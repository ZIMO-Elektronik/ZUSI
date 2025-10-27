file(GLOB_RECURSE SRC *.cpp)
add_executable(ZUSIZppLoad ${SRC})

sanitize(address,undefined)

target_common_warnings(ZUSIZppLoad PRIVATE)
target_common_errors(ZUSIZppLoad PRIVATE)

target_link_libraries(ZUSIZppLoad PRIVATE ZUSI::ZUSI)
