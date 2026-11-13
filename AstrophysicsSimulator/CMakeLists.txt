cmake_minimum_required(VERSION 3.20)
project(AstrophysicsSimulator VERSION 0.8.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASTRO_BUILD_GUI "Build Qt 6 desktop simulator" ON)
option(ASTRO_BUILD_TESTS "Build headless numerical tests" ON)

add_library(astro_physics STATIC src/Physics.cpp)
target_include_directories(astro_physics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    target_link_libraries(astro_physics PUBLIC OpenMP::OpenMP_CXX)
endif()

if(ASTRO_BUILD_TESTS)
    enable_testing()
    add_executable(astro_physics_tests tests/physics_test.cpp)
    target_link_libraries(astro_physics_tests PRIVATE astro_physics)
    add_test(NAME physics_validation COMMAND astro_physics_tests)
endif()

if(ASTRO_BUILD_GUI)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
    add_executable(AstrophysicsSimulator MACOSX_BUNDLE
        src/main.cpp
        src/MainWindow.hpp src/MainWindow.cpp
        src/GLView.hpp src/GLView.cpp
    )
    set_target_properties(AstrophysicsSimulator PROPERTIES AUTOMOC ON)
    target_link_libraries(AstrophysicsSimulator PRIVATE
        astro_physics Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
    set_target_properties(AstrophysicsSimulator PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER org.example.astrophysics-simulator
        MACOSX_BUNDLE_BUNDLE_NAME "Astrophysics Simulator")
    if(APPLE)
        target_compile_options(AstrophysicsSimulator PRIVATE -Wno-deprecated-declarations)
    endif()
endif()
