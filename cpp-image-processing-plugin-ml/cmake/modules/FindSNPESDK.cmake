# FindSNPESDK.cmake
#
# Locates the Qualcomm SNPE SDK (libSNPE + headers).
#
# Search order:
#   1. SNPE_ROOT (CMake variable or environment variable)
#   2. Project-local third-party/snpe-sdk/<version>
#   3. System paths used in Docker / Yocto sysroots
#
# Provides:
#   SNPE::SNPE  - imported shared library target
#
# Inputs (optional):
#   SNPE_ROOT           - root of the SNPE SDK tree
#   SNPESDK_INCLUDE_DIR - override include search
#   SNPESDK_LIBRARY     - override library search

# Determine candidate lib subdirectory based on target architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(_SNPE_LIB_SUFFIXES
        lib/aarch64-ubuntu-gcc9.4
        lib/aarch64-oe-linux-gcc11.2
        lib/aarch64-oe-linux-gcc9.3
    )
else()
    set(_SNPE_LIB_SUFFIXES
        lib/x86_64-linux-clang
    )
endif()

# Build search paths
set(_SNPE_SEARCH_PATHS)

if(SNPE_ROOT)
    list(APPEND _SNPE_SEARCH_PATHS ${SNPE_ROOT})
elseif(DEFINED ENV{SNPE_ROOT})
    list(APPEND _SNPE_SEARCH_PATHS $ENV{SNPE_ROOT})
endif()

# Project-local SDK
file(GLOB _SNPE_LOCAL_VERSIONS
    "${CMAKE_SOURCE_DIR}/third-party/snpe-sdk/*"
)
list(APPEND _SNPE_SEARCH_PATHS ${_SNPE_LOCAL_VERSIONS})

# System / Docker paths
list(APPEND _SNPE_SEARCH_PATHS
    /opt/qcom/aistack/qairt/2.28.2.241116
    /usr
)

find_path(SNPESDK_INCLUDE_DIR
    NAMES SNPE/DlSystem/DlEnums.hpp
    PATHS ${_SNPE_SEARCH_PATHS}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
)

find_library(SNPESDK_LIBRARY
    NAMES SNPE
    PATHS ${_SNPE_SEARCH_PATHS}
    PATH_SUFFIXES ${_SNPE_LIB_SUFFIXES}
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNPESDK
    REQUIRED_VARS SNPESDK_LIBRARY SNPESDK_INCLUDE_DIR
)

if(SNPESDK_FOUND AND NOT TARGET SNPE::SNPE)
    add_library(SNPE::SNPE SHARED IMPORTED)
    # SNPE headers use relative includes (e.g. #include "Wrapper.hpp")
    # so both include/ and include/SNPE/ must be on the search path.
    set_target_properties(SNPE::SNPE PROPERTIES
        IMPORTED_LOCATION "${SNPESDK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SNPESDK_INCLUDE_DIR};${SNPESDK_INCLUDE_DIR}/SNPE"
    )
endif()

mark_as_advanced(SNPESDK_INCLUDE_DIR SNPESDK_LIBRARY)
