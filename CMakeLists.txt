cmake_minimum_required(VERSION 2.8.4)
project(ns)
set(TARNAME ns)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# program name, version etc

set(PACKAGE_VERSION "2.35")
set(PACKAGE_NAME "ns")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://sourceforge.net/projects/nsnam")

find_package(PkgConfig REQUIRED)

##########################################################################
if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "~/workspace/thesis/ns-allinone-2.35/ns-2.35/")
endif(NOT DEFINED BIN_INSTALL_DIR)
if(NOT DEFINED DATA_INSTALL_DIR)
    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share")
endif(NOT DEFINED DATA_INSTALL_DIR)
if(NOT DEFINED MAN_INSTALL_DIR)
    set(MAN_INSTALL_DIR "${DATA_INSTALL_DIR}/man")
endif(NOT DEFINED MAN_INSTALL_DIR)

#########################################################################
# CMAKE settings

# set DEFINE var
add_definitions(-DTCP_DELAY_BIND_ALL)
add_definitions(-DNO_TK)
add_definitions(-DTCLCL_CLASSINSTVAR)
add_definitions(-DNDEBUG)
add_definitions(-DUSE_SHM)
add_definitions(-DHAVE_LIBTCLCL)
add_definitions(-DHAVE_TCLCL_H)
add_definitions(-DHAVE_LIBOTCL1_14)
add_definitions(-DHAVE_OTCL_H)
add_definitions(-DHAVE_LIBTK8_5)
add_definitions(-DHAVE_TK_H)
add_definitions(-DHAVE_LIBTCL8_5)
add_definitions(-DHAVE_TCLINT_H)
add_definitions(-DHAVE_TCL_H)
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DNS_DIFFUSION)
add_definitions(-DSMAC_NO_SYNC)
add_definitions(-DCPP_NAMESPACE=std)
add_definitions(-DUSE_SINGLE_ADDRESS_SPACE)
add_definitions(-Drng_test)
add_definitions(-DNDEBUG)
add_definitions(-DDEBUG)
INCLUDE(CheckModules)

# declare flags
set(CCOPT "-Wall -Wno-write-strings -g")

set(CFLAGS_MAC "-stdlib=libstdc++")

# set flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CCOPT} ${CFLAGS_MAC}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CCOPT} ${CFLAGS_MAC}")
set(LIB "-L/Users/huyvq/workspace/thesis/ns-allinone-2.35/tclcl-1.20 -ltclcl -L/Users/huyvq/workspace/thesis/ns-allinone-2.35/otcl-1.14 -lotcl -L/Users/huyvq/workspace/thesis/ns-allinone-2.35/lib -ltk8.5 -L/Users/huyvq/workspace/thesis/ns-allinone-2.35/lib -ltcl8.5 -L/usr/X11R6/lib -lXext -lX11 -lpcap -lm -lm -framework CoreFoundation -lstdc++.6")

#set include
include_directories("/usr/X11R6/include")
include_directories("/Users/huyvq/workspace/thesis/ns-allinone-2.35/include")
include_directories("/usr/include/pcap")

#######################################################################
## common/ptypes2tcl
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common/
)

set(ptypes2tcl_SRC
    common/ptypes2tcl.cc
)

add_executable(ptypes2tcl ${ptypes2tcl_SRC})
target_link_libraries(ptypes2tcl)

#######################################################################
### source code files
include(SourceFiles)
#######################################################################
get_filename_component(PARENT_DIR ${ns_SOURCE_DIR} PATH)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PARENT_DIR}/bin")
################################################################
### ns
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TCL_INCLUDE_PATH}
    ${TK_INCLUDE_PATH}
    ${OTCL_INCLUDE_DIRS}
    ${TCLCL_INCLUDE_DIRS}
    ${INCLUDE_SUB_DIR}
)

set(ns_SRC
    common/tclAppInit.cc
    common/main-monolithic.cc
    ${OBJ}
)

add_executable(ns ${ns_SRC})
target_link_libraries(ns ${LIB})
install(TARGETS ns RUNTIME DESTINATION ${BIN_INSTALL_DIR})

install(FILES ns.1 DESTINATION ${MAN_INSTALL_DIR}/man1)