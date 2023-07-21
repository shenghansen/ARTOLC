PROJECT(ARTOLC)

add_definitions(-DNUMA_NODE=4)
add_compile_options("-g")
add_executable(artolc example.cpp)
add_executable(test main.cpp)
add_library(tree Tree.cpp)
target_link_libraries(artolc tbb tree numa)
target_link_libraries(test tbb tree numa)