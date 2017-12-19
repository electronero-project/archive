# CMake generated Testfile for 
# Source directory: /root/myntnote/tests/hash
# Build directory: /root/myntnote/build/release/tests/hash
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(hash-fast "/root/myntnote/build/release/tests/hash/hash-tests" "fast" "/root/myntnote/tests/hash/tests-fast.txt")
add_test(hash-slow "/root/myntnote/build/release/tests/hash/hash-tests" "slow" "/root/myntnote/tests/hash/tests-slow.txt")
add_test(hash-tree "/root/myntnote/build/release/tests/hash/hash-tests" "tree" "/root/myntnote/tests/hash/tests-tree.txt")
add_test(hash-extra-blake "/root/myntnote/build/release/tests/hash/hash-tests" "extra-blake" "/root/myntnote/tests/hash/tests-extra-blake.txt")
add_test(hash-extra-groestl "/root/myntnote/build/release/tests/hash/hash-tests" "extra-groestl" "/root/myntnote/tests/hash/tests-extra-groestl.txt")
add_test(hash-extra-jh "/root/myntnote/build/release/tests/hash/hash-tests" "extra-jh" "/root/myntnote/tests/hash/tests-extra-jh.txt")
add_test(hash-extra-skein "/root/myntnote/build/release/tests/hash/hash-tests" "extra-skein" "/root/myntnote/tests/hash/tests-extra-skein.txt")
