<#
.SYNOPSIS
    Verifies the Remote Debug HTTP API of the Azahar emulator on localhost.

.DESCRIPTION
    Runs 6 connectivity and correctness checks against the Remote Debug API:
      1. Port listening check
      2. GET /api/v1/emulator/status → 200
      3. POST /api/v1/emulator/control (pause/resume) → 200
      4. POST /api/v1/emulator/speed → 200
      5. GET /api/v1/nonexistent → 404  (no _http_status leak)
      6. POST speed with string value  → 400  (no _http_status leak, no 500)

.PARAMETER Port
    TCP port the emulator's remote server is listening on (default 49355).

.PARAMETER Verbose
    Print raw response bodies for each check.

.EXAMPLE
    .\verify-remote-api.ps1

.EXAMPLE
    .\verify-remote-api.ps1 -Port 49355 -Verbose

.NOTES
    No Tailscale, no admin rights, no emulator config changes required.
    Target:  http://127.0.0.1:<Port>
#>

param(
    [int]$Port = 49355,
    [switch]$Verbose
)

$BaseUrl = "http://127.0.0.1:$Port"
$Passed = 0
$Failed = 0

function Write-Result {
    param($Name, $Result, $Detail)
    if ($Result) {
        Write-Host "  PASS $Name" -ForegroundColor Green
        $script:Passed++
    } else {
        Write-Host "  FAIL $Name" -ForegroundColor Red
        $script:Failed++
        if ($Detail) {
            Write-Host "       $Detail" -ForegroundColor DarkYellow
        }
    }
}

function Test-StatusCode {
    param($Expected, $Actual, $Name)
    if ($Expected -eq $Actual) { return $true }
    Write-Host "       Expected HTTP $Expected but got $Actual" -ForegroundColor Yellow
    return $false
}

Write-Host "=== Remote API Local Verification ===" -ForegroundColor Cyan
Write-Host "Target: $BaseUrl"
Write-Host ""

# ---------------------------------------------------------------------------
# Check 1: Port listening
# ---------------------------------------------------------------------------
Write-Host "[1/6] Port $Port listening check..." -ForegroundColor Cyan
$listening = netstat -an | Select-String ":$Port"
if ($listening -match "LISTENING") {
    Write-Result "Port $Port is LISTENING" $true
} else {
    Write-Result "Port $Port is NOT listening" $false @"
Is the emulator running with remote_server enabled?
Add to your config: [Debugging] enable_remote_server=true
"@
    Write-Host ""
    Write-Host "TIP: Start the emulator, then re-run this script." -ForegroundColor Yellow
    Write-Host "Total: $Passed passed, $Failed failed" -ForegroundColor $(if ($Failed -eq 0) { "Green" } else { "Red" })
    exit 1
}

try {
    # -----------------------------------------------------------------------
    # Check 2: Status endpoint
    # -----------------------------------------------------------------------
    Write-Host "[2/6] GET /api/v1/emulator/status..." -ForegroundColor Cyan
    $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/emulator/status" -UseBasicParsing
    $ok = Test-StatusCode 200 $resp.StatusCode "Status"
    if ($Verbose) {
        Write-Host "       Body: $($resp.Content)" -ForegroundColor Gray
    }
    Write-Result "GET /api/v1/emulator/status -> $($resp.StatusCode)" $ok

    # -----------------------------------------------------------------------
    # Check 3: Control endpoint (pause / resume)
    # -----------------------------------------------------------------------
    Write-Host "[3/6] POST /api/v1/emulator/control (pause/resume)..." -ForegroundColor Cyan
    $body = '{"action":"pause"}'
    $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/emulator/control" -Method Post -Body $body -ContentType "application/json" -UseBasicParsing
    $ok = Test-StatusCode 200 $resp.StatusCode "Control pause"
    if ($Verbose) {
        Write-Host "       Body: $($resp.Content)" -ForegroundColor Gray
    }

    $body = '{"action":"resume"}'
    $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/emulator/control" -Method Post -Body $body -ContentType "application/json" -UseBasicParsing
    $ok = ($ok -and (Test-StatusCode 200 $resp.StatusCode "Control resume"))
    if ($Verbose) {
        Write-Host "       Body: $($resp.Content)" -ForegroundColor Gray
    }
    Write-Result "POST /api/v1/emulator/control (pause+resume)" $ok

    # -----------------------------------------------------------------------
    # Check 4: Speed endpoint
    # -----------------------------------------------------------------------
    Write-Host "[4/6] POST /api/v1/emulator/speed..." -ForegroundColor Cyan
    $body = '{"speed_percent":200}'
    $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/emulator/speed" -Method Post -Body $body -ContentType "application/json" -UseBasicParsing
    $ok = Test-StatusCode 200 $resp.StatusCode "Speed"
    if ($Verbose) {
        Write-Host "       Body: $($resp.Content)" -ForegroundColor Gray
    }
    Write-Result "POST /api/v1/emulator/speed -> $($resp.StatusCode)" $ok

    # -----------------------------------------------------------------------
    # Check 5: 404 Not Found  +  _http_status leak check
    # -----------------------------------------------------------------------
    Write-Host "[5/6] GET /api/v1/nonexistent (expect 404)..." -ForegroundColor Cyan
    $ok = $false
    try {
        $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/nonexistent" -UseBasicParsing
        # Should have thrown -- if we get here, status is not an error code
        $ok = Test-StatusCode 404 $resp.StatusCode "404"
    } catch {
        $statusCode = $_.Exception.Response.StatusCode.value__
        $ok = Test-StatusCode 404 $statusCode "404"
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $responseBody = $reader.ReadToEnd()
        if ($Verbose) {
            Write-Host "       Body: $responseBody" -ForegroundColor Gray
        }
        if ($responseBody -match "_http_status") {
            Write-Host "       WARNING: _http_status leaked in response!" -ForegroundColor Red
        }
    }
    Write-Result "GET /api/v1/nonexistent -> 404" $ok

    # -----------------------------------------------------------------------
    # Check 6: JSON type mismatch -> 400  +  no _http_status leak
    # -----------------------------------------------------------------------
    Write-Host "[6/6] POST speed with string (expect 400, no _http_status leak)..." -ForegroundColor Cyan
    $ok = $false
    try {
        $body = '{"speed_percent":"not_a_number"}'
        $resp = Invoke-WebRequest -Uri "$BaseUrl/api/v1/emulator/speed" -Method Post -Body $body -ContentType "application/json" -UseBasicParsing
        # Should have thrown
        $ok = Test-StatusCode 400 $resp.StatusCode "400 for type_error"
    } catch {
        $statusCode = $_.Exception.Response.StatusCode.value__
        $ok = Test-StatusCode 400 $statusCode "400 for type_error"
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $responseBody = $reader.ReadToEnd()
        if ($Verbose) {
            Write-Host "       Body: $responseBody" -ForegroundColor Gray
        }
        if ($responseBody -match "_http_status") {
            Write-Host "       FAIL: _http_status leaked in error response!" -ForegroundColor Red
            $ok = $false
        }
        if ($responseBody -match "Internal server error") {
            Write-Host "       FAIL: Got 500-style error instead of 400!" -ForegroundColor Red
            $ok = $false
        }
        if ($responseBody -match "invalid_field_type") {
            Write-Host "       Correct error code 'invalid_field_type'" -ForegroundColor Green
        }
    }
    Write-Result "JSON type mismatch -> 400, no _http_status leak" $ok

} catch {
    Write-Host "Connection failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "TIP: Make sure the emulator is running and remote_server is enabled." -ForegroundColor Yellow
    Write-Host "     Config: [Debugging] enable_remote_server=true  remote_server_port=$Port" -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host ""
if ($Failed -eq 0) {
    Write-Host "All checks passed!" -ForegroundColor Green
} else {
    Write-Host "$Failed check(s) failed out of 6" -ForegroundColor Red
}
Write-Host "Total: $Passed passed, $Failed failed" -ForegroundColor $(if ($Failed -eq 0) { "Green" } else { "Red" })
