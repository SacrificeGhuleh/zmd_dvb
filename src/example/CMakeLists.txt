cmake_minimum_required(VERSION 3.14)

project(dvbInfoExample)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC dvbInfo_INTERFACE)

target_sources(${PROJECT_NAME}
               PRIVATE
               main.cpp
               $<TARGET_OBJECTS:dvbInfo_OBJECTS>
               )