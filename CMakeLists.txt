cmake_minimum_required( VERSION 3.14 )

# Use `ccache` if it is installed in system's PATH.
find_program( CCACHE_PROGRAM ccache )
if ( CCACHE_PROGRAM )
  set_property(
    GLOBAL PROPERTY
    RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}"
  )
endif ()

# Export compilation database in JSON format.
set( CMAKE_EXPORT_COMPILE_COMMANDS on )

project(
  createGPT_PROJ
  VERSION 0.0.1
  LANGUAGES C
)

add_executable( createGPT src/main.c )
target_include_directories( createGPT PUBLIC inc/ )
target_compile_options(
  createGPT PRIVATE
  -Wall
  $<IF:$<C_COMPILER_ID:GNU>,-Wextra -Werror,>
)

if ( BUILD_STATIC )
  target_link_options(
    createGPT PRIVATE
    $<IF:$<C_COMPILER_ID:GNU>,-static -static-libgcc,>
  )
endif ()
