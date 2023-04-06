cmake_minimum_required(VERSION 3.24)
project(altserver-linux VERSION 2.0)

# -------------------------------------------------------------------------------------------------------------------------------
# Patch upstream AltServer sources files
# -------------------------------------------------------------------------------------------------------------------------------

file(COPY "upstream_repo/" DESTINATION "${CMAKE_SOURCE_DIR}/upstream_patched/")
file(GLOB to_patch "upstream_repo/AltServer/*.c*" "upstream_repo/AltServer/*.h*")

foreach(file ${to_patch})
    execute_process(
            COMMAND python3 makefiles/rewrite_altserver_source.py "${file}" 
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE patched
        )

    string(REPLACE "upstream_repo" "upstream_patched" patched_file "${file}")
    file(WRITE "${patched_file}" "${patched}")

    message(STATUS "[altserver] Patched: ${file}")
endforeach()

set_source_files_properties(${to_patch} PROPERTIES LANGUAGE CXX)
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/AltServer.cpp")
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/WiredConnection.cpp")
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/AnisetteDataManager.cpp")

# -------------------------------------------------------------------------------------------------------------------------------
# Patch upstream AltSign sources files
# -------------------------------------------------------------------------------------------------------------------------------

file(GLOB to_patch "upstream_repo/AltSign/*.c*" "upstream_repo/AltSign/*.h*")

foreach(file ${to_patch})
    execute_process(
            COMMAND python3 makefiles/AltSign-build/rewrite_altsign_source.py "${file}" 
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE patched
        )

    string(REPLACE "upstream_repo" "upstream_patched" patched_file "${file}")
    file(WRITE "${patched_file}" "${patched}")

    message(STATUS "[altserver] Patched: ${file}")
endforeach()


set_source_files_properties(${to_patch} PROPERTIES LANGUAGE CXX)
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/AltServer.cpp")
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/WiredConnection.cpp")
file(REMOVE "${CMAKE_SOURCE_DIR}/upstream_patched/AltServer/AnisetteDataManager.cpp")

# -------------------------------------------------------------------------------------------------------------------------------
# Patch ldid source files
# -------------------------------------------------------------------------------------------------------------------------------

file(GLOB to_patch "upstream_patched/ldid/*.c*" "upstream_patched/ldid/*.h*")

foreach(file ${to_patch})
    execute_process(
            COMMAND python3 makefiles/AltSign-build/rewrite_ldid_source.py "${file}" 
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE patched
        )

    string(REPLACE "upstream_repo" "upstream_patched" patched_file "${file}")
    file(WRITE "${patched_file}" "${patched}")

    message(STATUS "[altserver] Patched: ${file}")
endforeach()

# -------------------------------------------------------------------------------------------------------------------------------
# Setup executable                          
# -------------------------------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME})
add_definitions(-w)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------------------------------------------------------------------
# Setup Third Party Libraries
# -------------------------------------------------------------------------------------------------------------------------------

# This is really bad
add_definitions(-include "${CMAKE_SOURCE_DIR}/makefiles/dnssd_loader-build/config.h" -include "${CMAKE_SOURCE_DIR}/makefiles/libimobiledevice-build/config.h")

file(GLOB dnssd_src "libraries/dnssd_loader/*.c*")
set_source_files_properties(${dnssd_src} PROPERTIES LANGUAGE CXX)
target_sources(${PROJECT_NAME} PRIVATE ${dnssd_src})
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/dnssd_loader")

file(GLOB libmobile_src "libraries/libimobiledevice/src/*.c*" "libraries/libimobiledevice/common/*.c*")
set_source_files_properties(${libmobile_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${libmobile_src})
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libimobiledevice/")
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libimobiledevice/include")

file(GLOB libmobile_glue_src "libraries/libimobiledevice-glue/src/*.c")
set_source_files_properties(${libmobile_glue_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${libmobile_glue_src})
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libimobiledevice-glue/include")

file(GLOB libplist_c_src "libraries/libplist/src/*.c" "libraries/libplist/libcnary/*.c")
list(FILTER libplist_c_src EXCLUDE REGEX "cnary\.c")
set_source_files_properties(${libplist_c_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${libplist_c_src})

file(GLOB libplist_src "libraries/libplist/src/*.cpp" "libraries/libplist/libcnary/*.cpp")
set_source_files_properties(${libplist_src} PROPERTIES LANGUAGE CXX)
target_sources(${PROJECT_NAME} PRIVATE ${libplist_src})

target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libplist/src")
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libplist/include")
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libplist/libcnary/include")

file(GLOB libusbmuxd_src "libraries/libusbmuxd/src/*.c")
set_source_files_properties(${libusbmuxd_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${libusbmuxd_src})
target_include_directories(${PROJECT_NAME} PRIVATE "libraries/libusbmuxd/include")
target_include_directories(${PROJECT_NAME} PRIVATE "upstream_patched/libusbmuxd/common")

file(GLOB ldid_cpp_src "upstream_patched/ldid/*.cpp") #"upstream_patched/ldid2/*.cpp")
set_source_files_properties(${ldid_cpp_src} PROPERTIES LANGUAGE CXX)
target_sources(${PROJECT_NAME} PRIVATE ${ldid_cpp_src})

file(GLOB ldid_src "upstream_patched/ldid/*.c") #"upstream_patched/ldid2/*.c")
set_source_files_properties(${ldid_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${ldid_src})
target_include_directories(${PROJECT_NAME} PRIVATE "upstream_patched/ldid") #"upstream_patched/ldid2")

file(GLOB minizip_src "upstream_patched/AltSign/Dependencies/minizip/*.c")
list(FILTER minizip_src EXCLUDE REGEX "win|miniunz|minizip\.c")
set_source_files_properties(${minizip_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${minizip_src})
target_include_directories(${PROJECT_NAME} PRIVATE "upstream_patched/AltSign/Dependencies/minizip")

file(GLOB corecrypto_src "upstream_patched/AltSign/Dependencies/corecrypto/*.c")
set_source_files_properties(${corecrypto_src} PROPERTIES LANGUAGE C)
target_sources(${PROJECT_NAME} PRIVATE ${corecrypto_src})
target_include_directories(${PROJECT_NAME} PRIVATE "upstream_patched/AltSign/Dependencies/")

# -------------------------------------------------------------------------------------------------------------------------------
# Setup Upstream Dependencies
# -------------------------------------------------------------------------------------------------------------------------------

find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ZLIB::ZLIB corecrypto_static uuid)

include(FetchContent)

FetchContent_Declare(cpprestsdk GIT_REPOSITORY "https://github.com/microsoft/cpprestsdk" GIT_TAG "2.10.18")

FetchContent_MakeAvailable(cpprestsdk)
target_link_libraries(${PROJECT_NAME} PRIVATE cpprest)

# -------------------------------------------------------------------------------------------------------------------------------
# Setup Shims
# -------------------------------------------------------------------------------------------------------------------------------

# This is really bad
add_definitions(-fpermissive -include "${CMAKE_SOURCE_DIR}/src/common.h" -include "${CMAKE_SOURCE_DIR}/shims/windows_shim.h")

target_sources(${PROJECT_NAME} PRIVATE "shims/old-linux-polyfill.c" "shims/windows_shim.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "src" "shims")

# -------------------------------------------------------------------------------------------------------------------------------
# Setup Upstream Project
# -------------------------------------------------------------------------------------------------------------------------------

file(GLOB upstream_src "upstream_patched/AltServer/*.c*" "upstream_patched/AltSign/*.c*")
set_source_files_properties(${upstream_src} PROPERTIES LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE "upstream_patched/AltServer" "upstream_patched/AltSign")
target_sources(${PROJECT_NAME} PRIVATE ${upstream_src})

# -------------------------------------------------------------------------------------------------------------------------------
# Setup Altserver-Linux
# -------------------------------------------------------------------------------------------------------------------------------

file(GLOB src "src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${src})