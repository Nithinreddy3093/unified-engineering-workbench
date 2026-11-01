cmake_minimum_required(VERSION 3.21)
project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

option(UEW_BUILD_TESTS "Build the dependency-free test executable" ON)
option(UEW_BUILD_DESKTOP "Build the native Windows desktop UI" WIN32)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(uew_core
    src/core/geometry/StlParser.cpp
    src/core/geometry/TemperatureCsvParser.cpp
    src/core/thermal/ThermalSolver.cpp
    src/core/performance/Benchmark.cpp
    src/core/hardware/MockDevices.cpp
    src/core/ml/Backprop.cpp
    src/core/export/CsvExport.cpp
    src/core/common/SystemInfo.cpp)
target_include_directories(uew_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(uew_core PUBLIC cxx_std_17)
if(WIN32)
  target_compile_definitions(uew_core PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601)
  target_link_libraries(uew_core PUBLIC advapi32)
endif()
if(MSVC)
  target_compile_options(uew_core PRIVATE /W4 /permissive- /EHsc /utf-8)
else()
  target_compile_options(uew_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
find_package(Threads REQUIRED)
target_link_libraries(uew_core PUBLIC Threads::Threads)

if(WIN32 AND UEW_BUILD_DESKTOP)
  add_executable(UnifiedEngineeringWorkbench WIN32
      src/frontend/windows/MainWindow.cpp
      src/frontend/windows/GeometryViewport.cpp
      src/main.cpp)
  set_target_properties(UnifiedEngineeringWorkbench PROPERTIES OUTPUT_NAME "Launch")
  target_link_libraries(UnifiedEngineeringWorkbench PRIVATE
      uew_core comctl32 comdlg32 user32 gdi32)
  target_compile_definitions(UnifiedEngineeringWorkbench PRIVATE UNICODE _UNICODE UEW_DESKTOP_APP)
  if(MSVC)
    target_compile_options(UnifiedEngineeringWorkbench PRIVATE /W4 /permissive- /EHsc /utf-8)
  endif()
else()
  add_executable(UnifiedEngineeringWorkbench src/main.cpp)
  target_link_libraries(UnifiedEngineeringWorkbench PRIVATE uew_core)
endif()

add_executable(uew_validation src/validation_main.cpp)
target_link_libraries(uew_validation PRIVATE uew_core)

if(UEW_BUILD_TESTS)
  enable_testing()
  add_executable(uew_tests tests/TestMain.cpp)
  target_link_libraries(uew_tests PRIVATE uew_core)
  add_test(NAME uew_tests COMMAND uew_tests)
endif()

install(TARGETS UnifiedEngineeringWorkbench RUNTIME DESTINATION .)
if(WIN32 AND UEW_BUILD_DESKTOP)
  install(DIRECTORY data/ DESTINATION data)
  install(FILES README.md LICENSES.md DESTINATION .)
  include(InstallRequiredSystemLibraries)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_PACKAGE_NAME "UnifiedEngineeringWorkbench")
  set(CPACK_PACKAGE_VENDOR "Unified Engineering Workbench")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_FILE_NAME "UnifiedEngineeringWorkbench-${PROJECT_VERSION}-windows-x64")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "Unified Engineering Workbench")
  include(CPack)
endif()
