# this module looks for zoltan library
# it will define the following values
#
# Needs environmental variables
#   SUPERLU_HOME
# Sets
#   SUPERLU_INCLUDE_DIRS
#   SUPERLU_LIBRARIES
#   CF3_HAVE_SUPERLU
#

option( CF3_SKIP_SUPERLU "Skip search for SuperLU library" OFF )

if( NOT CF3_SKIP_SUPERLU )

    coolfluid_set_trial_include_path("") # clear include search path
    coolfluid_set_trial_library_path("") # clear library search path

    coolfluid_add_trial_include_path( ${SUPERLU_HOME}/include )
    coolfluid_add_trial_include_path( $ENV{SUPERLU_HOME}/include )

    find_path( SUPERLU_INCLUDE_DIRS slu_ddefs.h PATH_SUFFIXES superlu PATHS ${TRIAL_INCLUDE_PATHS}  NO_DEFAULT_PATH )
    find_path( SUPERLU_INCLUDE_DIRS slu_ddefs.h PATH_SUFFIXES superlu )

    coolfluid_add_trial_library_path(${SUPERLU_HOME}/lib )
    coolfluid_add_trial_library_path($ENV{SUPERLU_HOME}/lib)

    find_library(SUPERLU_LIBRARIES superlu  PATHS  ${TRIAL_LIBRARY_PATHS}  NO_DEFAULT_PATH)
    find_library(SUPERLU_LIBRARIES superlu )

endif()

coolfluid_set_package( PACKAGE SuperLU
                       DESCRIPTION "direct large sparse linear system solver"
                       URL "http://crd.lbl.gov/~xiaoye/SuperLU"
                       TYPE OPTIONAL
                       VARS
                          SUPERLU_INCLUDE_DIRS
                          SUPERLU_LIBRARIES
                       QUIET )
