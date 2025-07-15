cmake_minimum_required(VERSION 3.10)
project(mapper_framework_c C )

set(CMAKE_C_STANDARD 99)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML REQUIRED yaml-0.1)

# 手动指定 cJSON 头文件和库
include_directories(/usr/include/cjson)
include_directories(/usr/local/include/protobuf-c)

set(CJSON_LIBRARIES cjson)
set(PROTOBUF_C_LIBRARIES protobuf-c)

include_directories(
    ${YAML_INCLUDE_DIRS}
    ${PROTOBUF_C_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/log
    ${CMAKE_CURRENT_SOURCE_DIR}/util/parse
    ${CMAKE_CURRENT_SOURCE_DIR}/dmi/v1beta1
    ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf
)

# 源文件
set(SRC
    main.c
    config/config.c
    log/log.c
    common/dataconverter.c
    common/datamodel.c
    common/event.c
    util/parse/grpc.c
    dmi/v1beta1/api.pb-c.c
    google/protobuf/any.pb-c.c
    google/protobuf/wrappers.pb-c.c
)

# 主程序
add_executable(main ${SRC})

target_link_libraries(main
    ${CJSON_LIBRARIES}
    ${YAML_LIBRARIES}
    ${PROTOBUF_C_LIBRARIES}
    pthread
)

# 测试程序
file(GLOB TEST_SRC test/*.c)
foreach(testfile ${TEST_SRC})
    get_filename_component(testname ${testfile} NAME_WE)
    add_executable(${testname} ${testfile}
        config/config.c
        log/log.c
        common/dataconverter.c
        common/datamodel.c
        common/event.c
        util/parse/grpc.c
        dmi/v1beta1/api.pb-c.c
        google/protobuf/any.pb-c.c
        google/protobuf/wrappers.pb-c.c
    )
    target_link_libraries(${testname}
        ${CJSON_LIBRARIES}
        ${YAML_LIBRARIES}
        ${PROTOBUF_C_LIBRARIES}
        pthread
    )
endforeach()