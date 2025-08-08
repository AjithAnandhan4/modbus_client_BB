add_library(libmodbus STATIC IMPORTED)

set_target_properties(libmodbus PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/thirdparty/libmodbus/lib/libmodbus.a
)

target_include_directories(libmodbus INTERFACE
    ${PROJECT_SOURCE_DIR}/thirdparty/libmodbus/inc
)

