include(GoogleTest)

file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSITests ${SRC})

sanatize("address,undefined")

target_common_warnings(ZUSITests PRIVATE)

cpmaddpackage("gh:google/googletest#main")

target_link_libraries(ZUSITests PRIVATE ZUSI::ZUSI GTest::gtest GTest::gmock)

gtest_discover_tests(ZUSITests)
