# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称 + 编程语言（纯c工程）
project(BN_LIB C)
# 设置C语言标准（C99，兼容所有C编译器）
set(CMAKE_C_STANDARD 99)
# 强制开启C标准检查，避免编译器兼容问题
set(CMAKE_C_STANDARD_REQUIRED ON)

# ========== 全集配置 ==========
# 定义build目录下的镜像目录，分别对应lib/test/src
set(BUILD_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
set(BUILD_TEST_DIR ${CMAKE_BINARY_DIR}/test)
set(BUILD_SRC_DIR ${CMAKE_BINARY_DIR}/src)
# 自动创建子目录，避免编译报错
file(MAKE_DIRECTORY ${BUILD_LIB_DIR} ${BUILD_TEST_DIR} ${BUILD_SRC_DIR})

# 全局配置：静态库输出到build/lib，当前先注释不更改输出目录
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${BUILD_LIB_DIR})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${BUILD_LIB_DIR})

# ========== 编译 静态库 ==========
# 自动收集 lib/ 目录下所有 .c 源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/lib BN_LIB_SRC)
# 生成静态库: bn是库名, STATIC指定为静态库, 最终生成libbn.a
add_library(bn STATIC ${BN_LIB_SRC})
# 为库指定头文件搜索路径
target_include_directories(bn
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
# 位lib库指定.o文件输出目录
set_target_properties(bn PROPERTIES
    # 核心：保留lib/编译的.o中间文件到build/lib/
    OBJECT_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
    OBJECT_OUTPUT_DIRECTORY_DEBUG ${BUILD_LIB_DIR}
    OBJECT_OUTPUT_DIRECTORY_RELEASE ${BUILD_LIB_DIR}
)

# ========== 编译 测试程序 ==========
# test/ 不生成可执行程序的文件
set(TEST_UTILS_SRC
    ${PROJECT_SOURCE_DIR}/test/test_utils.c
)
# 测试程序源文件
set(TEST_FILES
    "bn_unsigned_add_test.c"
    "bn_unsigned_sub_test.c"
    "bn_add_test.c"
    "bn_sub_test.c"
)
# 批量配置所有测试程序
foreach(TEST_FILE ${TEST_FILES})
    # 提取可执行程序名（去掉.c后缀）
    string(REPLACE ".c" "" TEST_EXEC_NAME ${TEST_FILE})
    # 拼接测试文件完整路径
    set(TEST_FILE_PATH ${PROJECT_SOURCE_DIR}/test/${TEST_FILE})
    # 生成测试可执行程序
    add_executable(${TEST_EXEC_NAME} ${TEST_FILE_PATH} ${TEST_UTILS_SRC})
    # 为测试程序指定头文件搜索路径
    target_include_directories(${TEST_EXEC_NAME}
        PUBLIC ${PROJECT_SOURCE_DIR}/include
        PUBLIC ${PROJECT_SOURCE_DIR}/test
    )
    # 链接静态库到测试程序
    target_link_libraries(${TEST_EXEC_NAME} PUBLIC bn)
    # 为测试程序指定输出目录
    set_target_properties(${TEST_EXEC_NAME} PROPERTIES
        # 可执行文件输出到build/test/
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_TEST_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BUILD_TEST_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BUILD_TEST_DIR}
        # 若需保存test/编译的.o文件，解开下面注释，输出到build/test/
        # OBJECT_OUTPUT_DIRECTORY ${BUILD_TEST_DIR}
        # OBJECT_OUTPUT_DIRECTORY_DEBUG ${BUILD_TEST_DIR}
        # OBJECT_OUTPUT_DIRECTORY_RELEASE ${BUILD_TEST_DIR}
    )
endforeach()

# ===== 编译优化 =====
# 开启所有警告 + 关闭无用警告 + O2优化
# 设置默认构建模式为Release（避免无指定时的编译混乱）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()
# 公共编译选项
set(BN_COMMON_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-implicit-fallthrough
    -Werror=implicit-function-declaration
)
# Release模式
set(BN_RELEASE_BASE_OPTIONS
    -O2
    -fomit-frame-pointer
)
# Debug模式
set(BN_DEBUG_BASE_OPTIONS
    -g3
    -gdwarf-2
    -O0
    -fno-omit-frame-pointer
)

# 编译器参数
# gcc
set(BN_GCC_DEBUG_EXTRA_OPTIONS -ggdb -fvar-tracking-assignments)
# clang
set(BN_CLANG_DEBUG_EXTRA_OPTIONS -ggdb -fvar-tracking-assignments)
# MSVC
set(BN_MSVC_DEBUG_EXTRA_OPTIONS /Zi /Od)

# 拼接编译参数
set(BN_RELEASE_COMPILE_OPTIONS ${BN_COMMON_COMPILE_OPTIONS} ${BN_RELEASE_BASE_OPTIONS})
set(BN_DEBUG_COMPILE_OPTIONS ${BN_COMMON_COMPILE_OPTIONS} ${BN_DEBUG_BASE_OPTIONS})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # gcc编译器，Debug参数，版本下限判断
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
        list(APPEND BN_DEBUG_COMPILE_OPTIONS ${BN_GCC_DEBUG_EXTRA_OPTIONS})
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang/LLVM编译器，Debug参数
    list(APPEND BN_DEBUG_COMPILE_OPTIONS ${BN_CLANG_DEBUG_EXTRA_OPTIONS})
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # MSVC编译器，MSVC专属参数
    set(BN_DEBUG_COMPILE_OPTIONS ${BN_MSVC_DEBUG_EXTRA_OPTIONS})
    set(BN_RELEASE_COMPILE_OPTIONS /O2 /Oi)
endif()

# 为目标设置编译选项
target_compile_options(bn PRIVATE
    $<$<CONFIG:Release>:${BN_RELEASE_COMPILE_OPTIONS}>
    $<$<CONFIG:Debug>:${BN_DEBUG_COMPILE_OPTIONS}>
)
foreach(TEST_FILE ${TEST_FILES})
    string(REPLACE ".c" "" TEST_EXEC_NAME ${TEST_FILE})
    target_compile_options(${TEST_EXEC_NAME} PRIVATE
    $<$<CONFIG:Release>:${BN_RELEASE_COMPILE_OPTIONS}>
    $<$<CONFIG:Debug>:${BN_DEBUG_COMPILE_OPTIONS}>
)
endforeach()