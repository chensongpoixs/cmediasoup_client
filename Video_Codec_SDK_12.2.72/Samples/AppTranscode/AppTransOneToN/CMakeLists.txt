# Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(AppTransOneToN)

set(APP_SOURCES
 ${CMAKE_CURRENT_SOURCE_DIR}/AppTransOneToN.cpp
)

if(WIN32)
    set(NV_FFMPEG_HDRS ${FFMPEG_DIR}/include)
endif()

set(NV_ENC_SOURCES
 ${NV_ENC_DIR}/NvEncoder.cpp
 ${NV_DEC_DIR}/NvDecoder.cpp
 ${NV_ENC_DIR}/NvEncoderCuda.cpp
)

set(NV_ENC_HDRS
 ${NV_ENC_DIR}/NvEncoder.h
 ${NV_DEC_DIR}/NvDecoder.h
 ${NV_ENC_DIR}/NvEncoderCuda.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
 ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
 ${NVCODEC_UTILS_DIR}/NvEncoderCLIOptions.h
 ${NVCODEC_UTILS_DIR}/FFmpegDemuxer.h
)

set(NV_ENC_CUDA_UTILS
 ${NVCODEC_UTILS_DIR}/Resize.cu
)

source_group( "headers" FILES ${NV_ENC_HDRS} )
source_group( "sources" FILES ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})

find_package(CUDA)

set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")
# CUDA 11.7 and later is compiled with /MT option (staticaly linked with C runtime), 
# since we are statically linking cudart_static.lib with the app, 
# we need to make sure that all .obj files linked are compiled with /MT to avoid linker warning LNK4098
if (WIN32 AND CUDA_VERSION_MAJOR GREATER_EQUAL 11 AND CUDA_VERSION_MINOR GREATER_EQUAL 7)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-Xcompiler /MT)
endif()
if ( CMAKE_COMPILER_IS_GNUCC )
    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
    endif()
endif()

cuda_add_executable(${PROJECT_NAME}  ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} ${NV_ENC_HDRS})

if (WIN32 AND CUDA_VERSION_MAJOR GREATER_EQUAL 11 AND CUDA_VERSION_MINOR GREATER_EQUAL 7)
    target_compile_options(${PROJECT_NAME} PRIVATE "/MT")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(${PROJECT_NAME} PUBLIC ${CUDA_INCLUDE_DIRS}
 ${NVCODEC_PUBLIC_INTERFACE_DIR}
 ${NVCODEC_UTILS_DIR}
 ${NV_CODEC_DIR}
 ${NV_FFMPEG_HDRS}
)

if(WIN32)
    Set(AVCODEC_LIB ${FFMPEG_DIR}/lib/avcodec.lib)
    Set(AVFORMAT_LIB ${FFMPEG_DIR}/lib/avformat.lib)
    Set(AVUTIL_LIB ${FFMPEG_DIR}/lib/avutil.lib)
    Set(SWRESAMPLE_LIB ${FFMPEG_DIR}/lib/swresample.lib)
    #copy dll's
    install(DIRECTORY ${FFMPEG_DLL_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} OPTIONAL FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY ${FFMPEG_LIB_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} OPTIONAL FILES_MATCHING PATTERN "*.lib")
endif ()

target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} ${AVCODEC_LIB}
 ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/ )
    add_dependencies(${PROJECT_NAME} copyFFMPEGFiles)

    #copy pdb's
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR}  CONFIGURATIONS Debug)
endif()

