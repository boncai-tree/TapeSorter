cmake_minimum_required(VERSION 3.30)
project(YADRO_TATLIN LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tatlin_core STATIC
        src/app_config.cpp
        src/file_tape.cpp
        src/file_tape_factory.cpp
        src/tape_sorter.cpp
)
target_include_directories(tatlin_core PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if (MSVC)
  target_compile_options(tatlin_core PRIVATE /W4 /permissive-)
else ()
  target_compile_options(tatlin_core PRIVATE -Wall -Wextra -Wpedantic)
endif ()

add_executable(YADRO_TATLIN main.cpp)
target_link_libraries(YADRO_TATLIN PRIVATE tatlin_core)

if (MSVC)
  target_compile_options(YADRO_TATLIN PRIVATE /W4 /permissive-)
else ()
  target_compile_options(YADRO_TATLIN PRIVATE -Wall -Wextra -Wpedantic)
endif ()

include(CTest)
option(BUILD_TESTING "Build unit tests" ON)
if (BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG v1.15.2
  )
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(yadro_tatlin_tests
          tests/test_file_tape.cpp
          tests/test_app_config.cpp
          tests/test_tape_sorter.cpp
  )
  target_include_directories(yadro_tatlin_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_link_libraries(yadro_tatlin_tests PRIVATE tatlin_core GTest::gtest_main)

  if (MSVC)
    target_compile_options(yadro_tatlin_tests PRIVATE /W4 /permissive-)
  else ()
    target_compile_options(yadro_tatlin_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif ()

  include(GoogleTest)
  gtest_discover_tests(yadro_tatlin_tests)
endif ()
