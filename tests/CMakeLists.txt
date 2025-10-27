include(GoogleTest)

file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSITests ${SRC})

sanitize(address,undefined)

target_common_warnings(ZUSITests PRIVATE)
target_common_errors(ZUSITests PRIVATE -Werror)

cpmaddpackage(URI "gh:google/googletest#main" OPTIONS "INSTALL_GTEST OFF")

target_link_libraries(ZUSITests PRIVATE GTest::gtest_main GTest::gmock
                                        ZUSI::ZUSI)

gtest_discover_tests(ZUSITests)
