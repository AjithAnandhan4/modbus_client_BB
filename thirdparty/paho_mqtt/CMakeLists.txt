add_library(paho-mqtt STATIC IMPORTED)

set_target_properties(paho-mqtt PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/thirdparty/paho-mqtt/lib/libpaho-mqtt.a
)

target_include_directories(paho-mqtt INTERFACE
    ${PROJECT_SOURCE_DIR}/thirdparty/paho-mqtt/inc
)

