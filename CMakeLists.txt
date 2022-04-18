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

project( GPT_IMAGE_PROJ VERSION 0.0.1 LANGUAGES C )

set(
  GPT_IMAGE_SOURCES
  src/gpt.c
  src/main.c
)

add_executable( gpt_image  ${GPT_IMAGE_SOURCES} )
target_include_directories( gpt_image PUBLIC inc/ )
target_compile_options(
  gpt_image PRIVATE
  -O3
  -Wall
  -Wextra
  -Werror
)
