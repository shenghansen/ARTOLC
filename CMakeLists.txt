PROJECT(ARTOLC)
add_definitions(-DNUMA_NODE=4)
add_compile_options("-g")

add_executable(artolc example.cpp)
add_library(tree Tree.cpp)
target_link_libraries(artolc tbb tree numa)

add_executable(test main.cpp)
target_link_libraries(test tbb tree numa)

find_package(OpenMP REQUIRED)
add_executable(test_omp main_omp.cpp)
target_link_libraries(test_omp OpenMP::OpenMP_CXX)
target_link_libraries(test_omp tree numa tbb pthread)
# target_compile_options(test_omp -fopenmp)