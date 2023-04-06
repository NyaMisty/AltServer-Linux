cmake_minimum_required(VERSION 3.1)
project(imobiledevice LANGUAGES C)

# -------------------------------------------------------------------------------------------------------------------------------
# Setup library                          
# -------------------------------------------------------------------------------------------------------------------------------

set(ROOT "${ALTSERVER_DIR}/libraries/libimobiledevice")
file(GLOB src "${ROOT}/src/*.c" "${ROOT}/common/*.c")

add_library(${PROJECT_NAME} ${src})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# --------------------------------------------------------------------------------------------------------
# Include directories
# --------------------------------------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE "${ROOT}")
target_include_directories(${PROJECT_NAME} PUBLIC "${ROOT}/include" "${ROOT}/common")

# --------------------------------------------------------------------------------------------------------
# Link required libraries
# --------------------------------------------------------------------------------------------------------

add_subdirectory(../libimobiledevice-glue "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")
target_link_libraries(${PROJECT_NAME} PUBLIC imobiledevice::glue)