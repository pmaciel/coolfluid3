# this module looks for MKL library
# it will define the following values
#
# Needs environmental variables
#   PARDISOMKL_HOME
# Sets
#   PARDISOMKL_INCLUDE_DIR
#   PARDISOMKL_LIBRARIES
#   CF3_HAVE_PARDISOMKL
#
option( CF3_SKIP_PARDISOMKL "Skip search for MKL Pardiso library" OFF )

if( NOT CF3_SKIP_PARDISOMKL )

  coolfluid_set_trial_include_path("") # clear include search path
  coolfluid_set_trial_library_path("") # clear library search path

  coolfluid_add_trial_include_path( ${PARDISOMKL_HOME}/include )
  coolfluid_add_trial_include_path( $ENV{PARDISOMKL_HOME}/include )
  coolfluid_add_trial_include_path( ${PARDISOMKL_HOME}/../include )
  coolfluid_add_trial_include_path( $ENV{PARDISOMKL_HOME}/../include )
  coolfluid_add_trial_include_path( ${PARDISOMKL_HOME}/../../include )
  coolfluid_add_trial_include_path( $ENV{PARDISOMKL_HOME}/../../include )

  find_path(PARDISOMKL_INCLUDE_DIR mkl_pardiso.h PATHS ${TRIAL_INCLUDE_PATHS}  NO_DEFAULT_PATH)
  find_path(PARDISOMKL_INCLUDE_DIR mkl_pardiso.h)
  list(APPEND PARDISOMKL_INCLUDE_DIRS ${PARDISOMKL_INCLUDE_DIR})

  if(DEFINED PARDISOMKL_HOME)
    coolfluid_add_trial_library_path(${PARDISOMKL_HOME})
    coolfluid_add_trial_library_path(${PARDISOMKL_HOME}/lib )
  endif()
  if(DEFINED ENV{PARDISOMKL_HOME})
    coolfluid_add_trial_library_path($ENV{PARDISOMKL_HOME})
    coolfluid_add_trial_library_path($ENV{PARDISOMKL_HOME}/lib)
  endif()
  if(DEFINED IOMP5_HOME)
    coolfluid_add_trial_library_path(${IOMP5_HOME})
    coolfluid_add_trial_library_path(${IOMP5_HOME}/lib )
  endif()
  if(DEFINED ENV{IOMP5_HOME})
    coolfluid_add_trial_library_path($ENV{IOMP5_HOME})
    coolfluid_add_trial_library_path($ENV{IOMP5_HOME}/lib )
  endif()

  find_library(PARDISOMKL_COMPUTATIONAL mkl_core ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH)
  if(WIN32 OR APPLE)
    # Windows MKL Libraries
    find_library(PARDISOMKL_INTERFACE NAMES mkl_intel_c mkl_intel_lp64 PATHS ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH) # 32/64-bit
    find_library(PARDISOMKL_THREADING NAMES mkl_intel_thread           PATHS ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH)
  else()
    # Linux MKL Libraries
    find_library(PARDISOMKL_INTERFACE NAMES mkl_gf mkl_gf_lp64     PATHS ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH) # 32/64-bit
    find_library(PARDISOMKL_THREADING NAMES mkl_gnu_thread         PATHS ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH)
  endif()
  list(APPEND PARDISOMKL_LIBRARIES ${PARDISOMKL_COMPUTATIONAL} ${PARDISOMKL_INTERFACE} ${PARDISOMKL_THREADING})

  # Extra library dependencies for static linking
  if(NOT BUILD_SHARED_LIBS OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    find_library(PARDISOMKL_RTL NAMES iomp5 libiomp5mt PATHS ${TRIAL_LIBRARY_PATHS} NO_DEFAULT_PATH) # Linux/Windows
    list(APPEND PARDISOMKL_LIBRARIES ${PARDISOMKL_RTL})
    if(UNIX)
      find_library(PTHREAD pthread ${TRIAL_LIBRARY_PATHS})
      list(APPEND PARDISOMKL_LIBRARIES ${PTHREAD})
    endif()
  endif()

endif( NOT CF3_SKIP_PARDISOMKL )

# mark_as_advanced(PARDISOMKL_THREADING PARDISOMKL_INTERFACE PARDISOMKL_COMPUTATIONAL PARDISOMKL_RTL PTHREAD)

coolfluid_set_package( PACKAGE PardisoMKL
                       DESCRIPTION "MKL implementation of Pardiso sparse direct solver"
                       URL "http://software.intel.com/en-us/intel-mkl"
                       TYPE OPTIONAL
                       VARS PARDISOMKL_LIBRARIES PARDISOMKL_INCLUDE_DIRS
                       QUIET )
