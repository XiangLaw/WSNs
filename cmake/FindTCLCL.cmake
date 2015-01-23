###################################################################
# - Find TCLCL
# The variables are defined by this module
#  
#  TCLCL_INCLUDE_DIRS
#  TCLCL_LIBRARIES   - List of libraries when using TCLCL.
#  TCLCL_FOUND       - True if TCLCL found.

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} PATH)
SET(TCLCL_ROOT_DIR ${PARENT_DIR}/tclcl-1.20)

FIND_PATH(TCLCL_INCLUDE_DIR
    NAME
    config.h
    tclcl.h
    idlecallback.h
    tclcl-internal.h
    iohandler.h
    tclcl-mappings.h
    rate-variable.h
    timer.h
    tclcl-config.h
    tracedvar.h
    PATHS
    ${TCLCL_ROOT_DIR}
  )

FIND_LIBRARY(TCLCL_LIBRARY
    NAME
    libtclcl.a
    PATHS
    ${TCLCL_ROOT_DIR}
  )

SET(TCLCL_INCLUDE_DIRS ${TCLCL_ROOT_DIR})
SET(TCLCL_LIBRARIES ${TCLCL_LIBRARY})

IF(TCLCL_INCLUDE_DIRS)
   MESSAGE(STATUS "TCLCL include dirs set to ${TCLCL_INCLUDE_DIRS}")
ELSE(TCLCL_INCLUDE_DIRS)
    MESSAGE(FATAL "Tclcl include dirs cannot be found")
ENDIF(TCLCL_INCLUDE_DIRS)

IF(TCLCL_INCLUDE_DIRS AND TCLCL_LIBRARIES)
  SET( TCLCL_FOUND "YES" )
ENDIF(TCLCL_INCLUDE_DIRS AND TCLCL_LIBRARIES)

MARK_AS_ADVANCED(
  TCLCL_LIBRARIES
  TCLCL_INCLUDE_DIRS
)
