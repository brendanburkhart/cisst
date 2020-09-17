# initial CMake cache values for travis.ci builds

# Use external projects, mostly to get recent version of libcpp on Ubuntu 16.04
set (CISST_USE_EXTERNAL ON CACHE BOOL "")

# Force compilation for cisstMesh
set (CISST_cisstMesh ON CACHE BOOL "")

# XML parsing
set (CISST_cisstCommonXML ON CACHE BOOL "")

# JSON
set (CISST_HAS_JSON ON CACHE BOOL "")

# Unit tests
set (CISST_BUILD_TESTS ON CACHE BOOL "")

# cisstNetlib
set (CISST_HAS_CISSTNETLIB ON CACHE BOOL "")
set (CISSTNETLIB_DIR "/home/travis/cisstNetlib-install" CACHE PATH "")
