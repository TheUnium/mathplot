cmake_minimum_required(VERSION 3.10)
project(mathplot C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)

set(SOURCES
    main.c
    parser.c
    maths.c
    graph.c
    stb_image_write.c
)

add_executable(mathplot ${SOURCES})
target_link_libraries(mathplot PRIVATE m ${CURSES_LIBRARIES})
target_include_directories(mathplot PRIVATE
  ${CURSES_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
)
target_compile_options(mathplot PRIVATE -Wall -Wextra)


