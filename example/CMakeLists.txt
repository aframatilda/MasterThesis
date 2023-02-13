cmake_minimum_required(VERSION 2.8...3.13)
project(CameraSDKTest)

set(CMAKE_CXX_STANDARD 11) 

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../lib/*.lib)

file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp") 

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
if(MSVC)
	source_group(Src FILES ${SRCS})
endif(MSVC)

add_executable(${PROJECT_NAME} ${SRCS}) 

if(WIN32)
	target_link_libraries(${PROJECT_NAME} CameraSDK)
elseif(APPLE)
	find_library(CoreFoundation_LIB CoreFoundation)
	find_library(IOKIT_LIB IOKit)
	target_link_libraries(${PROJECT_NAME} CameraSDK ${CoreFoundation_LIB} ${IOKIT_LIB})
else()
	target_link_libraries(${PROJECT_NAME} CameraSDK pthread udev)
endif(WIN32)

