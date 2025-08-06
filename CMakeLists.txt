cmake_minimum_required(VERSION 3.10)
project(mapper_framework_c C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML REQUIRED yaml-0.1)

# 手动指定 cJSON 头文件和库
find_package(CURL REQUIRED)

# 查找 FFmpeg 库（用于流处理）
pkg_check_modules(FFMPEG 
    libavformat 
    libavcodec 
    libavutil 
    libswscale
)

if(NOT FFMPEG_FOUND)
    message(WARNING "FFmpeg libraries not found. Stream processing will be disabled.")
    set(STREAM_FOUND FALSE)
else()
    message(STATUS "Found FFmpeg libraries")
    set(STREAM_FOUND TRUE)
endif()

# 查找 hiredis 库
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h
    PATHS /usr/include /usr/local/include)
find_library(HIREDIS_LIBRARY hiredis
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
    message(FATAL_ERROR "hiredis library not found. Please install libhiredis-dev")
endif()

# 查找 TDengine (taos) 库
find_path(TAOS_INCLUDE_DIR taos.h
    PATHS /usr/include /usr/local/include /usr/local/taos/include)
find_library(TAOS_LIBRARY taos
    PATHS /usr/lib /usr/local/lib /usr/local/taos/driver /usr/lib/x86_64-linux-gnu)

if(NOT TAOS_INCLUDE_DIR OR NOT TAOS_LIBRARY)
    message(WARNING "TDengine (taos) library not found. TDengine support will be disabled.")
    set(TAOS_FOUND FALSE)
else()
    message(STATUS "Found TDengine: ${TAOS_LIBRARY}")
    set(TAOS_FOUND TRUE)
endif()

include_directories(/usr/include/cjson)
include_directories(/usr/local/include/protobuf-c)

set(CJSON_LIBRARIES cjson)
set(PROTOBUF_C_LIBRARIES protobuf-c)

include_directories(
    ${YAML_INCLUDE_DIRS}
    ${PROTOBUF_C_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/log
    ${CMAKE_CURRENT_SOURCE_DIR}/util/parse
    ${CMAKE_CURRENT_SOURCE_DIR}/dmi/v1beta1
    ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf
    ${CMAKE_CURRENT_SOURCE_DIR}/grpcclient
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver
    ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/mysql
    ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/influxdb2
    ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/redis
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    /usr/include
    /usr/local/include
)

# 如果找到 TDengine，添加头文件目录
if(TAOS_FOUND)
    include_directories(${TAOS_INCLUDE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/tdengine)
endif()

# 如果找到 FFmpeg，添加头文件目录
if(STREAM_FOUND)
    include_directories(${FFMPEG_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/data/stream)
endif()

# absl 库列表（如有其它符号报错可补充）
set(ABSL_LIBS
    absl_log_internal_check_op
    absl_log_internal_message
    absl_log_internal_format
    absl_log_internal_globals
    absl_log_internal_proto
    absl_log_internal_nullguard
    absl_synchronization
    absl_time
    absl_strings
    absl_status
    absl_base
    absl_cord
    absl_cord_internal
    absl_cordz_info
    absl_cordz_handle
    absl_cordz_functions
    absl_cordz_sample_token
    absl_hash
)

# 设置源文件列表
set(COMMON_SOURCES
    config/config.c
    log/log.c
    common/dataconverter.c
    common/datamodel.c
    common/event.c
    util/parse/grpc.c
    dmi/v1beta1/api.pb-c.c
    dmi/v1beta1/api.pb.cc
    dmi/v1beta1/api.grpc.pb.cc
    google/protobuf/any.pb-c.c
    google/protobuf/wrappers.pb-c.c
    httpserver/httpserver.c
    global/global.c
    grpcclient/register.cc
    data/dbmethod/mysql/mysql_client.c
    data/dbmethod/mysql/handler.c 
    data/dbmethod/influxdb2/influxdb2_client.c
    data/dbmethod/influxdb2/handler.c 
    data/dbmethod/redis/redis_client.c
    data/dbmethod/redis/handler.c
    driver/driver.c
)

# 如果找到 TDengine，添加 TDengine 源文件
if(TAOS_FOUND)
    list(APPEND COMMON_SOURCES
        data/dbmethod/tdengine/tdengine_client.c
        data/dbmethod/tdengine/handler.c
    )
endif()

# 如果找到 FFmpeg，添加 Stream 源文件
if(STREAM_FOUND)
    list(APPEND COMMON_SOURCES
        data/stream/stream.c
    )
endif()

# 主程序
add_executable(main
    main.c
    ${COMMON_SOURCES}
)

# 设置链接库列表
set(COMMON_LIBRARIES
    ${CJSON_LIBRARIES}
    ${YAML_LIBRARIES}
    ${PROTOBUF_C_LIBRARIES}
    ${CURL_LIBRARIES}
    ${HIREDIS_LIBRARY}
    mysqlclient
    microhttpd
    grpc++
    grpc
    gpr
    protobuf
    yaml
    ${ABSL_LIBS}
    pthread
)

# 如果找到 TDengine，添加 taos 库
if(TAOS_FOUND)
    list(APPEND COMMON_LIBRARIES ${TAOS_LIBRARY})
endif()

# 如果找到 FFmpeg，添加 FFmpeg 库
if(STREAM_FOUND)
    list(APPEND COMMON_LIBRARIES ${FFMPEG_LIBRARIES})
endif()

target_link_libraries(main ${COMMON_LIBRARIES})

# 测试程序
file(GLOB TEST_SRC test/*.c)
foreach(testfile ${TEST_SRC})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile}
        ${COMMON_SOURCES}
    )
    target_link_libraries(${testname} ${COMMON_LIBRARIES})
endforeach()

# 打印配置信息
message(STATUS "=== Feature Support Summary ===")
message(STATUS "MySQL: YES")
message(STATUS "InfluxDB2: YES") 
message(STATUS "Redis: YES")
if(TAOS_FOUND)
    message(STATUS "TDengine: YES")
else()
    message(STATUS "TDengine: NO (library not found)")
endif()
if(STREAM_FOUND)
    message(STATUS "Stream Processing: YES")
else()
    message(STATUS "Stream Processing: NO (FFmpeg not found)")
endif()
message(STATUS "================================")