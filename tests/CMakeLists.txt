

add_executable(unit_tests
    test_ring_buffer.cpp
)


target_link_libraries(unit_tests PRIVATE 
    GTest::gtest
    GTest::gtest_main  # <-- This automatically provides the int main() for GTest!
    hpc_stdlib         # <-- Links your main library target
)

target_include_directories(
    unit_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/
)