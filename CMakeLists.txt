cmake_minimum_required(VERSION 3.20)
project(marquee-console
    VERSION 1.0.0
    DESCRIPTION "Immediate-mode marquee console engine with rate calibration"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC-specific flags
if(MSVC)
    add_compile_options(/W4 /utf-8 /EHsc)
    add_link_options(/SUBSYSTEM:CONSOLE)
endif()

add_executable(marquee-console
    src/main.cpp
    src/engine.cpp
    src/calibrate.cpp
    src/marquee.cpp
)

target_link_libraries(marquee-console PRIVATE)

add_executable(test-marquee
    src/test_marquee.cpp
    src/engine.cpp
    src/calibrate.cpp
    src/marquee.cpp
)
target_link_libraries(test-marquee PRIVATE)
