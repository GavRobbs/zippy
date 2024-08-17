add_subdirectory(gtest/)

add_executable(TestTrie test_trie.cpp)
target_link_libraries(TestTrie Zippy gtest gtest_main)

enable_testing()
add_test(NAME TestTrie COMMAND TestTrie)