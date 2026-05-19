if(NOT DEFINED REPO_ROOT)
    message(FATAL_ERROR "REPO_ROOT is required")
endif()

set(LAUNCHER_PATH "${REPO_ROOT}/dist/azahar-vulkan-validation.cmd")
set(BUNDLE_TARGET_PATH "${REPO_ROOT}/CMakeModules/BundleTarget.cmake")

if(NOT EXISTS "${LAUNCHER_PATH}")
    message(FATAL_ERROR "Missing validation launcher: ${LAUNCHER_PATH}")
endif()

file(READ "${BUNDLE_TARGET_PATH}" BUNDLE_TARGET_CONTENTS)
string(FIND "${BUNDLE_TARGET_CONTENTS}" "azahar-vulkan-validation.cmd" LAUNCHER_REF_INDEX)
if(LAUNCHER_REF_INDEX EQUAL -1)
    message(FATAL_ERROR "BundleTarget.cmake does not copy azahar-vulkan-validation.cmd")
endif()
