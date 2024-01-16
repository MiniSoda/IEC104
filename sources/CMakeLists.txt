set(NAME iec104_client)


set(SOURCES protocol/iec104_class.cpp
            protocol/IEC104Extension.cpp)

# set(source_path ${CMAKE_CURRENT_LIST_DIR}/)
# list (TRANSFORM SOURCES PREPEND ${source_path})

add_executable(${NAME} ${SOURCES})