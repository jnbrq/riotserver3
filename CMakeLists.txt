cmake_minimum_required(VERSION 3.0)
project(riotserver3)

# find required libraries
# threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# boost
set(Boost_USE_DEBUG_RUNTIME OFF)
set(Boost_USE_STATIC_RUNTIME OFF)   # make ON, if you want on win32
find_package(Boost 1.58 REQUIRED
    # filesystem
    # locale
    # date_time
    system
    program_options
    )

find_package(OpenSSL REQUIRED)

add_executable(
    riotserver3
    main.cpp
    src/riot/server/basic_server.cpp
    src/riot/server/ssl_server.cpp
    src/riot/server/header_parser.cpp
    src/riot/server/command_parser.cpp
    src/riot/server/xeid_matcher.cpp
    )

target_link_libraries(
    riotserver3
    PUBLIC Threads::Threads
    PUBLIC ${Boost_LIBRARIES}
    PUBLIC ${OPENSSL_LIBRARIES}
    )

target_include_directories(
    riotserver3
    PUBLIC ${CMAKE_SOURCE_DIR}
    PUBLIC ${Boost_INCLUDE_DIRS}
    PUBLIC ${OPENSSL_INCLUDE_DIR}
    )

set_target_properties(
    riotserver3
    PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    )

if(WIN32)
    target_link_libraries(riotserver3 wsock32 ws2_32) # to avoid linker errors
endif()

install(TARGETS riotserver3 RUNTIME DESTINATION bin)
