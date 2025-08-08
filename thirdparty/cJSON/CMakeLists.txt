add_library(cJSON STATIC
    src/cJSON.c
)

target_include_directories(cJSON PUBLIC
    ${PROJECT_SOURCE_DIR}/thirdparty/cJSON/inc
)

