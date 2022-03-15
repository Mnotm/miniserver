# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required (VERSION 2.8.12.2)

# Set the project name
project (miniserver)

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    miniserver.c
)

add_compile_options(-Wno-implicit)
add_executable(miniserver ${SOURCES})

find_package (Threads REQUIRED)
target_link_libraries(miniserver
    ${CMAKE_THREAD_LIBS_INIT}
)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package (SQLite3 REQUIRED)
if (SQLITE3_FOUND)
  include_directories(${SQLITE3_INCLUDE_DIRS})
 target_link_libraries(miniserver sqlite3)
endif (SQLITE3_FOUND)







