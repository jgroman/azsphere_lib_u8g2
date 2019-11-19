cmake_minimum_required (VERSION 3.8)
project (lib_u8g2_example_u8x8 C)

add_subdirectory( ../../lib_u8g2 ./lib_u8g2)

add_executable(${PROJECT_NAME} 
	main.c 
	epoll_timerfd_utilities.c
)

target_link_libraries (${PROJECT_NAME} 
	applibs 
	pthread 
	gcc_s 
	c 
	u8g2 
	lib_u8g2
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/../../lib_u8g2
)

# Add MakeImage post-build command
include (${AZURE_SPHERE_MAKE_IMAGE_FILE})
