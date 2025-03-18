cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gateway LANGUAGES C)

# This is a workaround for a bug in the Zephyr build system
set(ignoreMe "${NCS_TOOLCHAIN_VERSION}")

# Include directories
include_directories(
    include
)

# Add source files
file(GLOB_RECURSE APP_SOURCES
    "src/*.c"
    "drivers/*.c"
    "lib/*.c"
)


target_include_directories(app PRIVATE include)
target_sources(app PRIVATE
    ${APP_SOURCES}
)
