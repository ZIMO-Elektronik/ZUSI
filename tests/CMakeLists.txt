include(GoogleTest)

file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSITests ${SRC})

sanitize(address,undefined)

target_common_warnings(ZUSITests PRIVATE)

cpmaddpackage(
  NAME
  googletest
  GITHUB_REPOSITORY
  "google/googletest"
  GIT_TAG
  main
  SYSTEM
  YES
  OPTIONS
  "INSTALL_GTEST OFF")

target_link_libraries(ZUSITests PRIVATE GTest::gtest_main GTest::gmock
                                        ZUSI::ZUSI)

gtest_discover_tests(ZUSITests)
