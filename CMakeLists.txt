cmake_minimum_required(VERSION 3.10)
project(mapper_framework_c C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML REQUIRED yaml-0.1)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Protobuf / gRPC
# 优先用 CMake targets，找不到时走库名回退
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED)
endif()

find_package(gRPC CONFIG QUIET)

# FFmpeg（可选）
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

# hiredis
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h
  PATHS /usr/include /usr/local/include)
find_library(HIREDIS_LIBRARY hiredis
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
  message(FATAL_ERROR "hiredis library not found. Please install libhiredis-dev")
endif()

# TDengine（可选）
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

# 头文件目录
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
  ${CMAKE_CURRENT_SOURCE_DIR}/grpcserver
  ${CMAKE_CURRENT_SOURCE_DIR}/httpserver
  ${CMAKE_CURRENT_SOURCE_DIR}/device
  ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/mysql
  ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/influxdb2
  ${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/redis
  ${CMAKE_CURRENT_SOURCE_DIR}/driver
  /usr/include
  /usr/local/include
)

if(TAOS_FOUND)
  include_directories(${TAOS_INCLUDE_DIR})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/data/dbmethod/tdengine)
endif()

if(STREAM_FOUND)
  include_directories(${FFMPEG_INCLUDE_DIRS})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/data/stream)
endif()

# absl（仅在回退用库名链接时可能需要）
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

# 源文件
set(COMMON_SOURCES
  # 核心模块
  config/config.c
  log/log.c
  common/dataconverter.c
  common/datamodel.c
  common/event.c
  util/parse/grpc.c

  # Protobuf 生成
  dmi/v1beta1/api.pb-c.c
  dmi/v1beta1/api.pb.cc
  dmi/v1beta1/api.grpc.pb.cc
  google/protobuf/any.pb-c.c
  google/protobuf/wrappers.pb-c.c

  # 网络服务
  httpserver/httpserver.c
  grpcclient/register.cc
  grpcserver/server.cc

  # 设备管理
  device/device.c
  device/devicestatus.c
  device/devicetwin.c
  device/dev_panel.c

  # 驱动框架
  driver/driver.c

  # 数据库客户端
  data/dbmethod/mysql/mysql_client.c
  data/dbmethod/mysql/handler.c 
  data/dbmethod/influxdb2/influxdb2_client.c
  data/dbmethod/influxdb2/handler.c 
  data/dbmethod/redis/redis_client.c
  data/dbmethod/redis/handler.c
)

if(TAOS_FOUND)
  list(APPEND COMMON_SOURCES
    data/dbmethod/tdengine/tdengine_client.c
    data/dbmethod/tdengine/handler.c
  )
endif()

if(STREAM_FOUND)
  list(APPEND COMMON_SOURCES
    data/stream/stream.c
  )
endif()

# 条件编译宏
add_compile_definitions(
  $<$<BOOL:${TAOS_FOUND}>:TAOS_FOUND>
  $<$<BOOL:${STREAM_FOUND}>:ENABLE_STREAM>
)

# 主程序
add_executable(main
  main.c
  ${COMMON_SOURCES}
)

# gRPC/Protobuf 链接库（优先 targets）
set(GRPC_LINK_LIBS "")
set(PROTOBUF_LINK_LIB "")

if(TARGET gRPC::grpc++)
  list(APPEND GRPC_LINK_LIBS
    gRPC::grpc++
    gRPC::grpc
    gRPC::gpr
    gRPC::grpc++_reflection
  )
else()
  # 回退库名
  list(APPEND GRPC_LINK_LIBS
    grpc++
    grpc
    gpr
    grpc++_reflection
  )
  # 回退时 absl 可能需要
  list(APPEND GRPC_LINK_LIBS ${ABSL_LIBS})
endif()

if(TARGET protobuf::libprotobuf)
  list(APPEND PROTOBUF_LINK_LIB protobuf::libprotobuf)
else()
  list(APPEND PROTOBUF_LINK_LIB protobuf)
endif()

# 通用库
set(COMMON_LIBRARIES
  ${CJSON_LIBRARIES}
  ${YAML_LIBRARIES}
  ${PROTOBUF_C_LIBRARIES}
  ${CURL_LIBRARIES}
  ${HIREDIS_LIBRARY}
  mysqlclient
  microhttpd
  ${GRPC_LINK_LIBS}
  ${PROTOBUF_LINK_LIB}
  yaml
  Threads::Threads
  m
  dl
)

if(TAOS_FOUND)
  list(APPEND COMMON_LIBRARIES ${TAOS_LIBRARY})
endif()

if(STREAM_FOUND)
  list(APPEND COMMON_LIBRARIES ${FFMPEG_LIBRARIES})
endif()

target_link_libraries(main PRIVATE ${COMMON_LIBRARIES})

# 编译器选项
target_compile_options(main PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wno-unused-parameter>
  $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wno-unused-parameter>
)

# 构建后复制配置文件到构建目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
               ${CMAKE_CURRENT_BINARY_DIR}/config.yaml
               COPYONLY)

# 测试程序
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
  file(GLOB TEST_SRC test/*.c)
  foreach(testfile ${TEST_SRC})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile}
      ${COMMON_SOURCES}
    )
    target_link_libraries(${testname} PRIVATE ${COMMON_LIBRARIES})
    target_compile_options(${testname} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wno-unused-parameter>
      $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wno-unused-parameter>
    )
  endforeach()
endif()

# 示例程序
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  file(GLOB EXAMPLE_SRC examples/*.c)
  foreach(examplefile ${EXAMPLE_SRC})
    get_filename_component(examplename ${examplefile} NAME_WE)
    add_executable(example_${examplename} ${examplefile}
      ${COMMON_SOURCES}
    )
    target_link_libraries(example_${examplename} PRIVATE ${COMMON_LIBRARIES})
  endforeach()
endif()

# 安装规则
install(TARGETS main
  RUNTIME DESTINATION bin
)

install(DIRECTORY 
  common/
  config/
  log/
  device/
  driver/
  DESTINATION include/mapper-framework-c
  FILES_MATCHING PATTERN "*.h"
)

# 打印配置信息
message(STATUS "=== Build Configuration ===")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "=== Feature Support Summary ===")
message(STATUS "Core Framework: YES")
message(STATUS "Device Management: YES")
message(STATUS "HTTP Server: YES")
message(STATUS "GRPC Server: YES")
message(STATUS "MySQL: YES")
message(STATUS "InfluxDB2: YES") 
message(STATUS "Redis: YES")
if(TAOS_FOUND)
  message(STATUS "TDengine: YES")
else()
  message(STATUS "TDengine: NO (library not found)")
endif()
if(STREAM_FOUND)
  message(STATUS "Stream Processing: YES (FFmpeg)")
else()
  message(STATUS "Stream Processing: NO (FFmpeg not found)")
endif()
message(STATUS "")
message(STATUS "=== Source Files Summary ===")
list(LENGTH COMMON_SOURCES SOURCE_COUNT)
message(STATUS "Total source files: ${SOURCE_COUNT}")
message(STATUS "================================")

# Debug 信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "")
  message(STATUS "=== Debug Information ===")
  message(STATUS "Source files:")
  foreach(src ${COMMON_SOURCES})
    message(STATUS "  ${src}")
  endforeach()
  message(STATUS "Libraries:")
  foreach(lib ${COMMON_LIBRARIES})
    message(STATUS "  ${lib}")
  endforeach()
  message(STATUS "========================")
endif()