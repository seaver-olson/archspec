cmake_minimum_required(VERSION 3.16)

project(
  archspec_inspect
  VERSION 0.2.0
  DESCRIPTION "Dependency-free, structured local system introspection"
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(ARCHSPEC_BUILD_CLI "Build the archspec-inspect command" ON)
option(ARCHSPEC_BUILD_EXAMPLES "Build examples" OFF)
option(ARCHSPEC_BUILD_PYTHON "Build the CPython extension" ON)
set(
  ARCHSPEC_PYTHON_INSTALL_DIR
  "${CMAKE_INSTALL_LIBDIR}/python/archspec_inspect"
  CACHE PATH
  "Install directory for the archspec_inspect Python package"
)

set(ARCHSPEC_SOURCES
  src/block.cc
  src/cache.cc
  src/core.cc
  src/cpu.cc
  src/gpu.cc
  src/isa.cc
  src/json.cc
  src/memory.cc
  src/net.cc
  src/os.cc
  src/pci.cc
  src/perf.cc
  src/platform.cc
  src/power.cc
  src/thermal.cc
  src/virtualization.cc
)

add_library(archspec STATIC ${ARCHSPEC_SOURCES})
add_library(archspec::archspec ALIAS archspec)
target_compile_features(archspec PUBLIC cxx_std_17)
target_include_directories(
  archspec
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(
  archspec
  PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
  target_compile_options(archspec PRIVATE /W4 /permissive-)
else()
  target_compile_options(archspec PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ARCHSPEC_BUILD_CLI)
  add_executable(archspec-inspect tools/archspec-inspect.cc)
  target_link_libraries(archspec-inspect PRIVATE archspec::archspec)
  target_compile_features(archspec-inspect PRIVATE cxx_std_17)
endif()

if(ARCHSPEC_BUILD_EXAMPLES)
  add_executable(archspec-summary examples/summary.cc)
  target_link_libraries(archspec-summary PRIVATE archspec::archspec)
endif()

if(ARCHSPEC_BUILD_PYTHON)
  # Development.Module was introduced after this project's CMake minimum;
  # Development keeps the optional binding available on CMake 3.16 as well.
  find_package(Python3 COMPONENTS Interpreter Development QUIET)
  if(Python3_Development_FOUND)
    set(ARCHSPEC_PYTHON_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/python/archspec_inspect")
    file(MAKE_DIRECTORY "${ARCHSPEC_PYTHON_BUILD_DIR}")
    configure_file(
      python/archspec_inspect/__init__.py
      "${ARCHSPEC_PYTHON_BUILD_DIR}/__init__.py"
      COPYONLY
    )

    add_library(archspec_python MODULE python/archspec_inspect/_native.cc)
    target_link_libraries(archspec_python PRIVATE archspec::archspec Python3::Python)
    target_compile_features(archspec_python PRIVATE cxx_std_17)
    set_target_properties(
      archspec_python
      PROPERTIES
        PREFIX ""
        OUTPUT_NAME "_native"
        LIBRARY_OUTPUT_DIRECTORY "${ARCHSPEC_PYTHON_BUILD_DIR}"
    )

    install(
      TARGETS archspec_python
      LIBRARY DESTINATION "${ARCHSPEC_PYTHON_INSTALL_DIR}"
      RUNTIME DESTINATION "${ARCHSPEC_PYTHON_INSTALL_DIR}"
    )
    install(
      FILES python/archspec_inspect/__init__.py
      DESTINATION "${ARCHSPEC_PYTHON_INSTALL_DIR}"
    )
  else()
    message(STATUS "Python development headers not found; CPython extension will not be built")
  endif()
endif()

if(BUILD_TESTING)
  add_executable(archspec-tests tests/core_tests.cc)
  target_link_libraries(archspec-tests PRIVATE archspec::archspec)
  add_test(NAME archspec-core COMMAND archspec-tests)
  if(ARCHSPEC_BUILD_CLI)
    add_test(NAME archspec-cli-version COMMAND archspec-inspect --version)
    add_test(
      NAME archspec-cli-json
      COMMAND archspec-inspect --format json --categories os,cpu --redact
    )
  endif()
  if(TARGET archspec_python)
    add_test(
      NAME archspec-python
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/python_tests.py
    )
    set_tests_properties(
      archspec-python
      PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/python"
    )
  endif()
endif()

install(
  TARGETS archspec
  EXPORT archspecTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
if(ARCHSPEC_BUILD_CLI)
  install(TARGETS archspec-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES schema/archspec-report-v1.schema.json DESTINATION ${CMAKE_INSTALL_DATADIR}/archspec)

configure_package_config_file(
  cmake/archspecConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/archspecConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/archspec
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/archspecConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  EXPORT archspecTargets
  FILE archspecTargets.cmake
  NAMESPACE archspec::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/archspec
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/archspecConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/archspecConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/archspec
)

configure_file(cmake/archspec.pc.in archspec.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/archspec.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(CPACK_PACKAGE_NAME "archspec-inspect")
set(CPACK_PACKAGE_VENDOR "ArchSpec contributors")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
