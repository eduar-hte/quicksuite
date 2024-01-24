cmake_minimum_required(VERSION 3.22)

project(tests)

add_executable(client client.cpp)

add_executable(server server.cpp)

set(BOOST_ROOT /opt/boost)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.84.0 COMPONENTS system filesystem unit_test_framework REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
add_executable(cipher_tests cipher_tests.cpp)
target_link_libraries(cipher_tests ${Boost_LIBRARIES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET client server cipher_tests PROPERTY CXX_STANDARD 20)
endif()