add_library(bt)

generate_export_header(bt EXPORT_MACRO_NAME BT_API EXPORT_FILE_NAME
    ${CMAKE_CURRENT_BINARY_DIR}/public/bt/api.h)

target_include_directories(bt PUBLIC include
    ${CMAKE_CURRENT_BINARY_DIR}/public)

add_subdirectory(src)

if(BT_BUILD_TESTS)
    add_subdirectory(test)
endif()
