cmake_minimum_required(VERSION 3.20)

project(AviWxRisk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimized build so the SXF map renderer and the 60 fps render
# clock run at full speed unless the developer explicitly selects Debug.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

add_library(aviwx_core
    src/Geo.cpp
    src/GisMap.cpp
    src/Json.cpp
    src/RiskEngine.cpp
    src/Scenario.cpp
)

target_include_directories(aviwx_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(aviwx_core PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

add_executable(aviwx src/main.cpp)
target_link_libraries(aviwx PRIVATE aviwx_core)

enable_testing()

add_executable(aviwx_tests tests/risk_engine_tests.cpp)
target_link_libraries(aviwx_tests PRIVATE aviwx_core)

add_test(NAME aviwx_tests COMMAND aviwx_tests)

find_package(GDAL QUIET)
if(GDAL_FOUND)
    add_library(aviwx_gdal src/GdalSxfReader.cpp)
    target_link_libraries(aviwx_gdal PRIVATE aviwx_core GDAL::GDAL)
    target_include_directories(aviwx_gdal PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_options(aviwx_gdal PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
    )
else()
    message(STATUS "GDAL was not found; skipping SXF loader support.")
endif()

find_package(Qt6 QUIET COMPONENTS Widgets Network)

if(Qt6_FOUND)
    add_executable(aviwx_app
        src/gui/AppMain.cpp
        src/gui/MainWindow.cpp
        src/gui/ModeBar.cpp
        src/gui/RouteProfileWidget.cpp
        src/gui/WeatherTimelineWidget.cpp
        src/gui/WeatherMapWidget.cpp
    )

    target_link_libraries(aviwx_app PRIVATE
        Qt6::Widgets
        Qt6::Network
    )

    if(GDAL_FOUND)
        target_link_libraries(aviwx_app PRIVATE aviwx_gdal)
        target_compile_definitions(aviwx_app PRIVATE AVIWX_WITH_GDAL=1)
    else()
        target_link_libraries(aviwx_app PRIVATE aviwx_core)
    endif()

    target_compile_options(aviwx_app PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
    )
else()
    message(STATUS "Qt6 Widgets was not found; skipping aviwx_app GUI target.")
endif()
