# CMakeLists.txt for HMC5883L standalone host-side unit testing and verification.
# Sourced from Section 7 of DRIVER_STANDARD.md and aligned with the ANSA directory structure.
#
# NOTE (integration fix): the include/source paths below were updated to
# match Drivers/HMC5883L (this repo drops the extra mag/ nesting so all
# three sensor drivers sit at the same Drivers/<SENSOR>/ depth per
# DRIVER_STANDARD.md Section 1), and the source file names were corrected
# to main.c / mock_hmc5883l_bus.c to match what actually ships in this
# folder — the previous test_hmc5883l-v2.c / mock_hmc5883l_bus-v2.c /
# lowercase drivers/mag/hmc5883l paths did not exist, so this file did not
# build as originally submitted. A plain Makefile (matching DRIVER_STANDARD.md's
# literal ask, and consistent with tests/qemu_icm42688 and tests/qemu_bmp388)
# is also provided alongside this file; either builds the same test.

cmake_minimum_required(VERSION 3.10)
project(test_hmc5883l C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure output build directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories: driver interfaces and registration headers
include_directories(
    ../../Drivers/HMC5883L/Inc
    .
)

# Test executable definitions
add_executable(test_hmc5883l
    main.c
    mock_hmc5883l_bus.c
    ../../Drivers/HMC5883L/Src/hmc5883l.c
)

# Link standard math library
target_link_libraries(test_hmc5883l m)

# Enable compiler warnings to maintain strict ANSA coding standards
if(MSVC)
    target_compile_options(test_hmc5883l PRIVATE /W4)
else()
    target_compile_options(test_hmc5883l PRIVATE -Wall -Wextra -Werror)
endif()
