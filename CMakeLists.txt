cmake_minimum_required(VERSION 3.28)

project(graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(graph_core)

target_sources(graph_core
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_core/graph_core.ixx
)

target_compile_features(graph_core PUBLIC cxx_std_20)

add_library(graph_layout)

target_sources(graph_layout
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_layout/graph_layout.ixx
)

target_compile_features(graph_layout PUBLIC cxx_std_20)

add_library(graph_editor)

target_sources(graph_editor
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_editor/graph_editor.ixx
)

target_link_libraries(graph_editor PUBLIC graph_core graph_layout)
target_compile_features(graph_editor PUBLIC cxx_std_20)

add_library(graph_ui)

target_sources(graph_ui
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_ui/graph_ui.ixx
)

target_compile_features(graph_ui PUBLIC cxx_std_20)

add_library(graph_ui_svg)

target_sources(graph_ui_svg
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_ui_svg/graph_ui_svg.ixx
)

target_link_libraries(graph_ui_svg PUBLIC graph_ui graph_layout)
target_compile_features(graph_ui_svg PUBLIC cxx_std_20)

add_library(graph_ui_imgui)

target_sources(graph_ui_imgui
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      graph_ui_imgui/graph_ui_imgui.ixx
)

target_link_libraries(graph_ui_imgui PUBLIC graph_ui graph_layout)
target_compile_features(graph_ui_imgui PUBLIC cxx_std_20)

include(CTest)

if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(graph_core_tests)
  target_sources(graph_core_tests
    PRIVATE
      tests/graph_core_tests.cpp
  )
  target_link_libraries(graph_core_tests PRIVATE graph_core GTest::gtest_main)
  target_compile_features(graph_core_tests PRIVATE cxx_std_20)

  include(GoogleTest)
  gtest_discover_tests(graph_core_tests)
endif()
