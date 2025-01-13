file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSIExamples ${SRC})

sanitize(address,undefined)

target_common_warnings(ZUSIExamples PRIVATE)

target_link_libraries(ZUSIExamples PRIVATE ZUSI::ZUSI)
