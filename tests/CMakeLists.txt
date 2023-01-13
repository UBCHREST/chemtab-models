cmake_minimum_required(VERSION 3.18.4)

# FetContent will allow downloading of other libraries
include(FetchContent)

# ABLATE must be available for the project.  If the ABLATE_PATH is not specified it will download the version specified in FetchContent_Declare
set(ABLATE_PATH "" CACHE PATH "Custom path to local ABLATE project.  If specified this will be used instead of downloading ABLATE.")

if("${ABLATE_PATH}" STREQUAL "")
    message(STATUS "ABLATE_PATH was not specified.  ABLATE is being downloaded and built.")

    # Download and make ablate available to the client
    FetchContent_Declare(
            ablate
            GIT_REPOSITORY https://github.com/UBCHREST/ablate.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(ablate)

else()
    message(STATUS "Configuring with ABLATE_PATH: ${ABLATE_PATH}")
    add_subdirectory(${ABLATE_PATH})
endif()

# Set the project details
project(chemTabModelTests VERSION 0.0.1)

# Set the c/c++ Standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# define the two sample programs and link to ablate
add_executable(chemTabModelTests tests.cpp)
target_link_libraries(chemTabModelTests PUBLIC ablateLibrary testingResources GTest::gtest_main)

gtest_discover_tests(chemTabModelTests
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES "VS_DEBUGGER_WORKING_DIRECTORY;\"${PROJECT_DIR}\";LABELS;unitTests"
        )

# define the chemtab-model root directory so that the tests can find it
get_filename_component(CHEMTAB_MODEL_PATH ${CMAKE_SOURCE_DIR} DIRECTORY)
configure_file(modelPath.hpp.in modelPath.hpp @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})