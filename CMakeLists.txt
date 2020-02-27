cmake_minimum_required(VERSION 3.5)
project(auto-complete)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

include_directories(${PROJECT_SOURCE_DIR}/include/ /usr/local/include/ /usr/ )
include_directories(/home/c.servan/Tools/symspell/include/)

link_directories(/usr/local/lib/ /usr/lib/ /home/c.servan/Tools/symspell/lib/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -lpthread")


SET(auto-complete
  ${CMAKE_SOURCE_DIR}/src/suggest.cpp
  ${CMAKE_SOURCE_DIR}/src/rest_server.cpp
  ${CMAKE_SOURCE_DIR}/src/utils.cpp
  ${CMAKE_SOURCE_DIR}/src/auto-complete.cpp
  ${CMAKE_SOURCE_DIR}/src/benderrmq.cpp
  ${CMAKE_SOURCE_DIR}/src/phrase_map.cpp
  ${CMAKE_SOURCE_DIR}/src/segtree.cpp
  ${CMAKE_SOURCE_DIR}/src/sparsetable.cpp
  ${CMAKE_SOURCE_DIR}/src/lookuptables.cpp
  ${CMAKE_SOURCE_DIR}/src/embeddings.cpp
)
add_executable(auto-complete ${auto-complete})

target_link_libraries(auto-complete pistache pthread yaml-cpp symspell fasttext)

install(TARGETS auto-complete DESTINATION bin)
