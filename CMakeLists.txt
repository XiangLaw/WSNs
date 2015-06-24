cmake_minimum_required(VERSION 2.8.4)
project(ns)
set(TARNAME ns)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

#set ( CMAKE_VERBOSE_MAKEFILE true )

# program name, version etc

set(PACKAGE_VERSION "2.35")
set(PACKAGE_NAME "ns")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://sourceforge.net/projects/nsnam")

find_package(PkgConfig REQUIRED)

##########################################################################
if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "/home/eleven/")
endif(NOT DEFINED BIN_INSTALL_DIR)
if(NOT DEFINED DATA_INSTALL_DIR)
    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share")
endif(NOT DEFINED DATA_INSTALL_DIR)
if(NOT DEFINED MAN_INSTALL_DIR)
    set(MAN_INSTALL_DIR "${DATA_INSTALL_DIR}/man")
endif(NOT DEFINED MAN_INSTALL_DIR)

add_definitions(-DTCP_DELAY_BIND_ALL)
add_definitions(-DNO_TK -DTCLCL_CLASSINSTVAR )
add_definitions(-DNDEBUG -DLINUX_TCP_HEADER)
add_definitions(-DUSE_SHM -DHAVE_LIBTCLCL)
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
add_definitions(-DSTL_NAMESPACE=std)
add_definitions(-DUSE_SINGLE_ADDRESS_SPACE)
add_definitions(-Drng_test)
add_definitions(-DNDEBUG)
add_definitions(-DDEBUG)
INCLUDE(CheckModules)
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
target_link_libraries(ns -lnsl -ldl -lm ${X11_LIBRARIES} ${X11_Xext_LIB} ${TCL_LIBRARY} ${TCL_STUB_LIBRARY} ${TK_LIBRARY} ${TK_STUB_LIBRARY} ${OTCL_LIBRARIES} ${TCLCL_LIBRARIES})
install(TARGETS ns RUNTIME DESTINATION ${BIN_INSTALL_DIR})

install(FILES ns.1 DESTINATION ${MAN_INSTALL_DIR}/man1)


#######################################################################################
### nse
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TCL_INCLUDE_PATH}
    ${TK_INCLUDE_PATH}
    ${OTCL_INCLUDE_DIRS}
    ${TCLCL_INCLUDE_DIRS}
    ${INCLUDE_SUB_DIR}
    ${PCAP_INCLUDE_DIRS}
)
set(nse_SRC
    common/tclAppInit.cc
    common/main-monolithic.cc
    ${OBJ}
    ${OBJ_EMULATE_CC}
    ${OBJ_EMULATE_C}
)
add_executable(nse ${nse_SRC})
target_link_libraries(nse -lnsl -ldl -lm ${X11_LIBRARIES} ${X11_Xext_LIB} ${PCAP_LIBRARIES} ${TCL_LIBRARY} ${TCL_STUB_LIBRARY} ${TK_LIBRARY} ${TK_STUB_LIBRARY} ${OTCL_LIBRARIES} ${TCLCL_LIBRARIES})
install(TARGETS nse RUNTIME DESTINATION ${BIN_INSTALL_DIR})

#######################################################################################
### nstk
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TCL_INCLUDE_PATH}
    ${TK_INCLUDE_PATH}
    ${OTCL_INCLUDE_DIRS}
    ${TCLCL_INCLUDE_DIRS}
    ${INCLUDE_SUB_DIR}
)
set(nstk_SRC
    common/tkAppInit.cc
    ${OBJ}
)
add_executable(nstk ${nstk_SRC})
target_link_libraries(nstk ${TCL_LIBRARY} ${TCL_STUB_LIBRARY} ${TK_LIBRARY} ${TK_STUB_LIBRARY} ${OTCL_LIBRARIES} ${TCLCL_LIBRARIES})
install(TARGETS nstk RUNTIME DESTINATION ${BIN_INSTALL_DIR})

#########################################################################################
####  indep-utils
add_subdirectory(indep-utils/cmu-scen-gen/setdest)
add_subdirectory(indep-utils/webtrace-conv/dec)
add_subdirectory(indep-utils/webtrace-conv/epa)
add_subdirectory(indep-utils/webtrace-conv/nlanr)
add_subdirectory(indep-utils/webtrace-conv/ucb)
#=========================================================================================
### test
enable_testing()
add_test(NAME ns_TEST COMMAND ./validate)