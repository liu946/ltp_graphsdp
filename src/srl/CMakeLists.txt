include_directories (.
#        ${THIRDPARTY_DIR}/boost/include
        ${THIRDPARTY_DIR}/eigen
        ${THIRDPARTY_DIR}/dynet
        )
# look for Boost
if(DEFINED ENV{BOOST_ROOT})
    set(Boost_NO_SYSTEM_PATHS ON)
endif()
set(Boost_REALPATH ON)
find_package(Boost COMPONENTS program_options serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
link_directories(${THIRDPARTY_DIR}/dynet/build/dynet) # -L

set(LIBS ${LIBS} ${Boost_LIBRARIES})

add_subdirectory(include)
include_directories(include)

set(LIBS ${LIBS} base_static_lib)

set (srl_VERSION "0.0.1")


include_directories(common)
add_subdirectory(common)

set(LIBS ${LIBS} common_static_lib)

add_subdirectory(Pi)
#set(LIBS ${LIBS} srl_pi_static_lib)

add_subdirectory(Srl)
set(LIBS ${LIBS} srl_srl_static_lib)

add_subdirectory(tool)

set (srl_SRC
        DepSRL.cpp
        DepSRL.h
        SRL_DLL.cpp
        SRL_DLL.h)

add_library (srl_static_lib STATIC ${srl_SRC})

set_target_properties (srl_static_lib   PROPERTIES
        OUTPUT_NAME     srl)

#add_library (ppsrl_shared_lib SHARED ${srl_SRC})
#
#set_target_properties (ppsrl_shared_lib   PROPERTIES
#    VERSION         ${srl_VERSION}
#    OUTPUT_NAME     srl)

target_link_libraries (srl_static_lib dynet ${LIBS})
#target_link_libraries (ppsrl_shared_lib maxent_shared_lib)

configure_file (
        SRL_DLL.h
        ${INCLUDE_OUTPUT_PATH}/ltp/SRL_DLL.h)

link_directories (${LIBRARY_OUTPUT_PATH})
