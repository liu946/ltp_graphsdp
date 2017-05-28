include_directories (./
    ${SOURCE_DIR}/
    ${THIRDPARTY_DIR}/boost/include/
    ${THIRDPARTY_DIR}/maxent/
    ${THIRDPARTY_DIR}/tinyxml/
    ${THIRDPARTY_DIR}/jsoncpp/include)

# look for Boost
#if(DEFINED ENV{BOOST_ROOT})
#  set(Boost_NO_SYSTEM_PATHS ON)
#endif()
#set(Boost_REALPATH ON)
#find_package(Boost COMPONENTS program_options serialization REQUIRED)
#include_directories(${Boost_INCLUDE_DIR})
#set(LIBS ${LIBS} ${Boost_LIBRARIES})

set (ltp_server_SRC ltp_server.cpp mongoose.c mongoose.h)
link_directories ( ${LIBRARY_OUTPUT_PATH} )
add_executable (ltp_server ${ltp_server_SRC})
target_link_libraries (ltp_server
    maxent_static_lib
    ltp
    splitsnt_static_lib
    segmentor_static_lib
    postagger_static_lib
    parser_static_lib
    lstm_sdparser_static_lib
    ner_static_lib
    srl_static_lib
    xml4nlp
    pthread
    boost_program_options_static_lib
    boost_regex_static_lib
    boost_serialization_static_lib
    dl
    dynet
    jsoncpp_lib_static)
