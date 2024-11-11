cmake_minimum_required(VERSION 3.10)
project(BrightnessControl)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(iniparser
  GIT_REPOSITORY https://github.com/Rookfighter/inifile-cpp
  GIT_TAG 7bb1ec3534768e0d1fd9893d01027468b72af5ec
)
FetchContent_MakeAvailable(iniparser)

add_library(RCOBJ OBJECT src/resource.rc)
add_executable(BrightnessControl WIN32 src/app.cpp $<TARGET_OBJECTS:RCOBJ>)
target_link_libraries(BrightnessControl PRIVATE comctl32 uuid inicpp)