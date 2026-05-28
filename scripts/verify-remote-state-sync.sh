#!/usr/bin/env bash
set -euo pipefail

state_handler="src/core/remote/handlers/state_handler.cpp"
core_header="src/core/core.h"
core_source="src/core/core.cpp"
rasterizer_cache="src/video_core/rasterizer_cache/rasterizer_cache.h"
remote_handler="src/core/remote/remote_handler.cpp"
remote_docs="docs/remote_api.md"

require_pattern() {
    local file="$1"
    local pattern="$2"
    local message="$3"

    if ! rg -n "$pattern" "$file" >/dev/null; then
        echo "FAIL: $message"
        exit 1
    fi
}

if rg -n "SendSignal\\(Core::System::Signal::(Save|Load)" "$state_handler" >/dev/null; then
    echo "FAIL: remote state handlers still enqueue save/load directly"
    exit 1
fi

require_pattern "$core_header" "RequestStateOperation" \
    "core header does not expose synchronous state operations"
require_pattern "$core_source" "System::RequestStateOperation" \
    "core source does not implement synchronous state operations"
require_pattern "$core_header" "CancelStateOperation" \
    "core header does not expose state operation cancellation"
require_pattern "$core_source" "System::CancelStateOperation" \
    "core source does not cancel timed-out queued state operations"
require_pattern "$core_source" "System::CancelPendingStateOperation" \
    "core source does not cancel pending state operations during lifecycle changes"
require_pattern "$core_source" "state_operation_cv\\.notify_all" \
    "state operations do not notify waiters"
if ! sed -n '/case Signal::Reset:/,/case Signal::Load:/p' "$core_source" |
    rg -n "CancelPendingStateOperation" >/dev/null; then
    echo "FAIL: reset/shutdown signals do not cancel pending state operations"
    exit 1
fi

if ! sed -n '/void System::Shutdown/,/^}/p' "$core_source" |
    rg -n "CancelPendingStateOperation" >/dev/null; then
    echo "FAIL: System::Shutdown does not cancel pending state operations"
    exit 1
fi

if ! sed -n '/void System::Shutdown/,/^}/p' "$core_source" |
    rg -n "if \\(!is_deserializing\\)" >/dev/null; then
    echo "FAIL: savestate load shutdown does not preserve remote server while deserializing"
    exit 1
fi

if ! awk '
    /System::RequestStateOperation\(/ { in_function = 1 }
    in_function && /current_signal = signal/ { queued_line = NR }
    in_function && /frame_limiter\.AdvanceFrame\(\)/ { advance_line = NR }
    in_function && /state_operation_cv\.wait_for/ { wait_line = NR }
    in_function && /^}/ {
        ok = queued_line > 0 && advance_line > queued_line && wait_line > advance_line
        exit !ok
    }
' "$core_source"; then
    echo "FAIL: remote save/load does not advance a paused frame after queueing and before waiting"
    exit 1
fi

if ! awk '
    /void System::Shutdown/ { in_function = 1 }
    in_function && /if \(!is_deserializing\)/ { guard_line = NR }
    in_function && /remote_server\.reset\(\)/ {
        if (guard_line == 0 || NR - guard_line > 3) {
            exit 1
        }
        found = 1
    }
    in_function && /^}/ { exit !found }
' "$core_source"; then
    echo "FAIL: System::Shutdown can reset remote server while deserializing savestate load"
    exit 1
fi

if ! sed -n '/#ifdef ENABLE_REMOTE_SERVER/,/#endif/p' "$core_source" |
    rg -n "enable_remote_server\\.GetValue\\(\\) && !remote_server" >/dev/null; then
    echo "FAIL: System::Init can recreate remote server during savestate load"
    exit 1
fi

if ! awk '
    /Shutdown, but persist a few things between loads/ { in_block = 1 }
    in_block && /const u64 current_title_id = title_id\.load\(\)/ { saved_line = NR }
    in_block && /Shutdown\(true\)/ { shutdown_line = NR }
    in_block && /Init\(\*m_emu_window/ { init_line = NR }
    in_block && /title_id\.store\(current_title_id\)/ { restore_line = NR }
    in_block && /^    }/ {
        ok = saved_line > 0 && shutdown_line > saved_line &&
             init_line > shutdown_line && restore_line > init_line
        exit !ok
    }
' "$core_source"; then
    echo "FAIL: savestate load does not restore title_id after deserialization reinit"
    exit 1
fi

require_pattern "$state_handler" "MakeErrorResponse\\(409, .*signal_pending" \
    "state handler does not map pending signals to HTTP 409"
require_pattern "$state_handler" "MakeErrorResponse\\(504" \
    "state handler does not map timeouts to HTTP 504"
require_pattern "$state_handler" "state_operation_timeout" \
    "state handler does not return state operation timeout error code"
require_pattern "$state_handler" "MakeErrorResponse\\(500" \
    "state handler does not map failed state operations to HTTP 500"
require_pattern "$state_handler" "state_operation_failed" \
    "state handler does not return state operation failure error code"
require_pattern "$remote_handler" "_http_status" \
    "dispatcher does not propagate per-response HTTP status"

if rg -n -F 'stop` / `reset` / `save` / `load` は `SendSignal`' "$remote_docs" >/dev/null; then
    echo "FAIL: remote API docs still describe save/load as enqueue-only"
    exit 1
fi

check_complete_before_wait() {
    local marker="$1"
    local label="$2"

    if ! awk -v marker="$marker" '
        $0 ~ marker {
            in_block = 1
            complete_line = 0
            wait_line = 0
        }
        in_block && /CompleteStateOperation\(operation_id, ResultStatus::Success/ {
            if (complete_line == 0) {
                complete_line = NR
            }
        }
        in_block && /frame_limiter\.WaitOnce\(\)/ {
            if (wait_line == 0) {
                wait_line = NR
            }
        }
        in_block && /return ResultStatus::Success;/ {
            if (complete_line == 0 || (wait_line != 0 && complete_line > wait_line)) {
                exit 1
            }
            found = 1
            in_block = 0
        }
        END {
            if (!found) {
                exit 1
            }
        }
    ' "$core_source"; then
        echo "FAIL: $label reports success after frame limiter wait"
        exit 1
    fi
}

check_complete_before_wait "save_state_request_status == SaveStateStatus::LOADING" "state load"
check_complete_before_wait "save_state_request_status == SaveStateStatus::SAVING" "state save"

check_wait_guarded_for_remote() {
    local marker="$1"
    local label="$2"

    if ! awk -v marker="$marker" '
        $0 ~ marker {
            in_block = 1
            complete_line = 0
            guard_line = 0
            wait_line = 0
        }
        in_block && /CompleteStateOperation\(operation_id, ResultStatus::Success/ {
            complete_line = NR
        }
        in_block && /if \(operation_id == 0\)/ {
            guard_line = NR
        }
        in_block && /frame_limiter\.WaitOnce\(\)/ {
            wait_line = NR
        }
        in_block && /return ResultStatus::Success;/ {
            if (wait_line == 0 || complete_line == 0 || guard_line == 0 ||
                !(complete_line < guard_line && guard_line < wait_line)) {
                exit 1
            }
            found = 1
            in_block = 0
        }
        END {
            if (!found) {
                exit 1
            }
        }
    ' "$core_source"; then
        echo "FAIL: $label does not restrict frame limiter wait to non-remote state operations"
        exit 1
    fi
}

check_wait_guarded_for_remote "save_state_request_status == SaveStateStatus::LOADING" "state load"
check_wait_guarded_for_remote "save_state_request_status == SaveStateStatus::SAVING" "state save"

if ! sed -n '/void RasterizerCache<T>::ClearAll/,/^}/p' "$rasterizer_cache" |
    rg -n "runtime\\.Finish\\(\\)" >/dev/null; then
    echo "FAIL: savestate ClearAll(true) does not wait for renderer work completion"
    exit 1
fi

echo "PASS: remote save/load waits for completion and savestate flush waits for renderer work"
