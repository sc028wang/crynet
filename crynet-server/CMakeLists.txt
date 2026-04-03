cmake_minimum_required(VERSION 3.24)

project(crynet VERSION 0.1.0 LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/crynet.cmake")

message(STATUS "Configured crynet ${PROJECT_VERSION} with C++${CMAKE_CXX_STANDARD}")
