file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSIExamples ${SRC})

sanatize("address,undefined")

target_common_warnings(ZUSIExamples PRIVATE)

target_link_libraries(ZUSIExamples PRIVATE ZUSI::ZUSI)
