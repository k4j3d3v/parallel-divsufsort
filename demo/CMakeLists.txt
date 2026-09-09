add_executable(pardss main.cpp)
target_link_libraries (pardss divsufsort libprange)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")
