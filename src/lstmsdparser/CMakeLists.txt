include_directories (${SOURCE_DIR}/
    ${THIRDPARTY_DIR}/boost/include
    ${THIRDPARTY_DIR}/eigen-3.2.10/
    ${THIRDPARTY_DIR}/cnn/
    )

set (lstm_sdparser_VERSION "0.0.1")
set (lstm_sdparser_SRC
    lstm_sdparser.cpp
    c2.h
    listbased.cc
    )

# look for Boost
#if(DEFINED ENV{BOOST_ROOT})
#  set(Boost_NO_SYSTEM_PATHS ON)
#endif()
#set(Boost_REALPATH ON)
#find_package(Boost COMPONENTS program_options serialization REQUIRED)
#include_directories(${Boost_INCLUDE_DIR})
#set(LIBS ${LIBS} ${Boost_LIBRARIES})

set (Boost_LIBRARIES
    boost_program_options_static_lib
    boost_serialization_static_lib
    )


add_library(lstm_sdparser_static_lib STATIC lstm_sdparser_dll.cpp ${lstm_sdparser_SRC})
set_target_properties (lstm_sdparser_static_lib    PROPERTIES
    OUTPUT_NAME     lstm_sdparser)

add_library(lstm_sdparser_shared_lib SHARED lstm_sdparser_dll.cpp ${lstm_sdparser_SRC})
set_target_properties (lstm_sdparser_shared_lib    PROPERTIES
    VERSION         ${lstm_sdparser_VERSION}
    OUTPUT_NAME     lstm_sdparser)
target_link_libraries(lstm_sdparser_shared_lib cnn ${Boost_LIBRARIES})

# redirect output binary to tools/train
add_executable(lstm-parse main.cpp ${lstm_sdparser_SRC})
target_link_libraries(lstm-parse cnn ${Boost_LIBRARIES})
set_target_properties (lstm-parse PROPERTIES
    OUTPUT_NAME lstm-parse
    RUNTIME_OUTPUT_DIRECTORY ${TOOLS_DIR}/train/)
configure_file (
    lstm_sdparser_dll.h
    ${INCLUDE_OUTPUT_PATH}/ltp/lstm_sdparser_dll.h)


add_executable(lstm_par_cmdline lstm_par_cmdline.cpp)
target_link_libraries (lstm_par_cmdline lstm_sdparser_static_lib cnn ${Boost_LIBRARIES})
set_target_properties (lstm_par_cmdline PROPERTIES
    OUTPUT_NAME lstm_par_cmdline
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/examples/)


