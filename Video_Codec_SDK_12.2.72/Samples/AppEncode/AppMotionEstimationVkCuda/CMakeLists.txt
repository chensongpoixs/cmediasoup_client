# Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(AppMotionEstimationVkCuda)

#find_package(VULKAN)

# AUTO_LOCATE_VULKAN - accepted value ON or OFF
# ON  - Use CMake to auto locate the Vulkan SDK.
# OFF - Vulkan SDK path can be specified manually. This is helpful to test the build on various Vulkan version.
option(AUTO_LOCATE_VULKAN "AUTO_LOCATE_VULKAN" ON)

if(AUTO_LOCATE_VULKAN)
    message(STATUS "Attempting auto locate Vulkan using CMake......")

    # Find Vulkan Path using CMake's Vulkan Module
    # This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
    # Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
    find_package(Vulkan)

    # Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
    if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
        set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
        STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
    endif()
endif()

if(WIN32)
    set(VULKAN_HDR_DIR ${VULKAN_PATH}/Include)
    if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        set(VULKAN_LIB_DIR ${VULKAN_PATH}/Lib32)
    else()
        set(VULKAN_LIB_DIR ${VULKAN_PATH}/Lib)
    endif()
else ()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PC_VULKAN REQUIRED IMPORTED_TARGET vulkan)
    set(VULKAN_HDR_DIR ${PC_VULKAN_INCLUDE_DIRS})
    find_library(VULKAN_LIBS vulkan)
endif()

message("VULKAN_PATH: ${VULKAN_PATH}")
message("VULKAN_SDK: $ENV{VULKAN_SDK}")
message("VULKAN_LIB_DIR: ${VULKAN_LIB_DIR}")
message("VULKAN_HDR_DIR: ${VULKAN_HDR_DIR}")

set(APP_SOURCES
 ${CMAKE_CURRENT_SOURCE_DIR}/AppMotionEstimationVkCuda.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/NvEnc.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/utility.cpp
)

set(APP_HDRS
 ${CMAKE_CURRENT_SOURCE_DIR}/NvEnc.h
 ${CMAKE_CURRENT_SOURCE_DIR}/utility.h
)

set(NV_ENC_SOURCES
 ${NV_ENC_DIR}/NvEncoder.cpp
)

set(NV_ENC_HDRS
 ${NV_ENC_DIR}/NvEncoder.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
)

source_group( "headers" FILES ${NV_ENC_HDRS} ${APP_HDRS} )
source_group( "sources" FILES ${APP_SOURCES} ${NV_ENC_SOURCES} )

find_package(CUDA)

set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})

list(APPEND CUDA_NVCC_FLAGS -I$(VULKAN_SDK)/include)

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")
if ( CMAKE_COMPILER_IS_GNUCC )
    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
    endif()
endif()

cuda_add_executable(${PROJECT_NAME}  ${APP_SOURCES} ${NV_ENC_SOURCES} ${APP_HDRS} ${NV_ENC_HDRS} )

set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(${PROJECT_NAME} PUBLIC ${CUDA_INCLUDE_DIRS}
 ${NVCODEC_PUBLIC_INTERFACE_DIR}
 ${NVCODEC_UTILS_DIR}
 ${NV_CODEC_DIR}
 ${VULKAN_HDR_DIR}
)

# Add any required preprocessor definitions here

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Include Vulkan header files from Vulkan SDK
    set(VULKAN_LIBS ${VULKAN_LIB_DIR}/vulkan-1.lib)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    include_directories(AFTER ${VULKAN_PATH}/Include)
    # Link directory for vulkan-1
    link_directories(${VULKAN_PATH}/Bin;${VULKAN_PATH}/Lib;)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    link_libraries(${XCB_LIBRARIES})
endif()

# Only enable validation layers in debug builds of the application
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:-DUSE_VALIDATION_LAYERS>")

target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} ${VULKAN_LIBS} )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/ )
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} CONFIGURATIONS Debug)
endif()

