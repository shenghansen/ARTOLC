PROJECT(ARTOLC)

add_compile_options("-g")
add_executable(artolc example.cpp)
add_library(tree Tree.cpp)
target_link_libraries(artolc tbb tree numa)