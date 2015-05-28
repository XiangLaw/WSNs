###################################################################
# - Find otcl
# The variables are defined by this module
#  
#  OTCL_INCLUDE_DIRS
#  OTCL_LIBRARIES   - List of libraries when using otcl.
#  OTCL_FOUND       - True if otcl found.

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} PATH)
SET(OTCL_ROOT_DIR ${PARENT_DIR}/otcl-1.14)
MESSAGE(STATUS "ROOT: ${OTCL_ROOT_DIR}")

FIND_PATH(OTCL_INCLUDE_DIR
    NAME
    otcl.h
    PATHS
    ${OTCL_ROOT_DIR}
  )

FIND_LIBRARY(OTCL_LIBRARY
    NAME
    libotcl.a
    PATHS
    ${OTCL_ROOT_DIR}
  )

SET(OTCL_INCLUDE_DIRS ${OTCL_INCLUDE_DIR})
SET(OTCL_LIBRARIES ${OTCL_LIBRARY})

IF(OTCL_INCLUDE_DIRS)
   MESSAGE(STATUS "OTCL include dirs set to ${OTCL_INCLUDE_DIRS}")
ELSE(OTCL_INCLUDE_DIRS)
    MESSAGE(FATAL "Otcl include dirs cannot be found")
ENDIF(OTCL_INCLUDE_DIRS)

IF(OTCL_INCLUDE_DIRS AND OTCL_LIBRARIES)
  SET( OTCL_FOUND "YES" )
ENDIF(OTCL_INCLUDE_DIRS AND OTCL_LIBRARIES)

MARK_AS_ADVANCED(
  OTCL_LIBRARIES
  OTCL_INCLUDE_DIRS
)
