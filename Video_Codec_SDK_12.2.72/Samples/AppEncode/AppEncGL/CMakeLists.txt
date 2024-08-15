# Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(AppEncGL)

set(APP_SOURCES
 ${CMAKE_CURRENT_SOURCE_DIR}/AppEncGL.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/GraphicsUtils.cpp
)

set(NV_GL_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../External/)

set(NV_ENC_SOURCES
 ${NV_ENC_DIR}/NvEncoder.cpp
 ${NV_ENC_DIR}/NvEncoderGL.cpp
)

set(NV_ENC_HDRS
 ${NV_ENC_DIR}/NvEncoder.h
 ${NV_ENC_DIR}/NvEncoderGL.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
 ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
 ${NVCODEC_UTILS_DIR}/NvEncoderCLIOptions.h
 ${NVCODEC_UTILS_DIR}/Logger.h
 ${CMAKE_CURRENT_SOURCE_DIR}/GraphicsUtils.h
)

source_group( "headers" FILES ${NV_ENC_HDRS} )
source_group( "sources" FILES ${APP_SOURCES} ${NV_ENC_SOURCES})

add_executable(${PROJECT_NAME}  ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_HDRS})

target_include_directories(${PROJECT_NAME} PUBLIC
 ${NVCODEC_PUBLIC_INTERFACE_DIR}
 ${NVCODEC_UTILS_DIR}
 ${NV_CODEC_DIR}
 ${NV_ENC_DIR}
 ${NV_FFMPEG_HDRS}
 ${NV_GL_HDRS}
)

find_library(FREEGLUT_LIB glut)
find_library(GLEW32_LIB GLEW)
find_library(X11_LIB X11)
find_library(GL_LIB GL)
find_library(EGL_LIB EGL)

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} ${AVCODEC_LIB}
 ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB} ${FREEGLUT_LIB} ${FREEGLUT_STATIC_LIB} ${FREEGLUT_STATICD_LIB}
 ${FREEGLUTD_LIB} ${GLEW32_LIB} ${GLEW32S_LIB} ${X11_LIB} ${GL_LIB} ${EGL_LIB})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})
if (MSVC)
    set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NVCODEC_SAMPLES_INSTALL_DIR}/$<0:> )
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} CONFIGURATIONS Debug)
endif()

