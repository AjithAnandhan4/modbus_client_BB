cmake_minimum_required(VERSION 3.10)

project(modbus_client_BB)

set(CMAKE_C_STANDARD 11)

add_subdirectory(thirdparty/cJSON)
#add_subdirectory(thirdparty/libmodbus)

include_directories(${PROJECT_SOURCE_DIR}/inc)

set(SOURCES
	src/main.c
	src/json.c
	src/mqtt.c
    src/config.c
    src/modbus.c
)

add_executable(modbus_client_BB ${SOURCES})

target_link_libraries(modbus_client_BB 
	cJSON         
    #    libmodbus     
	paho-mqtt3a
    pthread	
)

set(CMAKE_EXE_LINKER_FLAGS "-static")


