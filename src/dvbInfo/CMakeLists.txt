cmake_minimum_required(VERSION 3.14)

project(dvbInfo)

add_library(${PROJECT_NAME}_INTERFACE INTERFACE)
target_include_directories(${PROJECT_NAME}_INTERFACE INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_sources(${PROJECT_NAME}_INTERFACE
               INTERFACE
               dvbinfo.h
               program.h
               sdtdescriptor.h
               mpegvideodescriptor.h
               mpegaudiodescriptor.h
               ac3audiodescriptor.h
               networkinfodescriptor.h
               helpers.h)

find_library(ucsi NAMES ucsi)
target_link_libraries(${PROJECT_NAME}_INTERFACE INTERFACE ucsi)

add_library(${PROJECT_NAME}_OBJECTS OBJECT)
target_sources(${PROJECT_NAME}_OBJECTS
               PUBLIC
               dvbinfo.cpp
               program.cpp
               sdtdescriptor.cpp
               mpegvideodescriptor.cpp
               mpegaudiodescriptor.cpp
               ac3audiodescriptor.cpp
               networkinfodescriptor.cpp
               helpers.cpp)

target_link_libraries(${PROJECT_NAME}_OBJECTS PUBLIC ${PROJECT_NAME}_INTERFACE)
