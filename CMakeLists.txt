cmake_minimum_required( VERSION 3.14 )
project( PROJ_GPT_IMAGE VERSION 0.0.1 LANGUAGES C )

set(
  GPT_IMAGE_SOURCES
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
