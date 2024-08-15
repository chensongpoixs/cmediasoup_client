# Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(AppDecGL)

if(WIN32)
    set(APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AppDecGL.cpp
    )
    set(NV_FFMPEG_HDRS ${FFMPEG_DIR}/include)
else ()
    set(APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AppDecGL.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FramePresenterGLX.cpp
    )
endif()

if(WIN32)
    set(NV_GL_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../External/)
    set(NV_GLEW_HDRS ${GLEW_DIR}/include)
	set(NV_GLUT_HDRS ${GLUT_INC})
else ()
    set(NV_GL_HDRS "")
endif()

set(NV_DEC_SOURCES
 ${NV_DEC_DIR}/NvDecoder.cpp
)

set(NV_ENC_CUDA_UTILS
 ${NVCODEC_UTILS_DIR}/ColorSpace.cu
)

set(NV_DEC_HDRS
 ${NV_DEC_DIR}/NvDecoder.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/cuviddec.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvcuvid.h
 ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
 ${NVCODEC_UTILS_DIR}/FFmpegDemuxer.h
 ${NVCODEC_UTILS_DIR}/ColorSpace.h
 ${NV_APPDEC_COMMON_DIR}/AppDecUtils.h
 ${CMAKE_CURRENT_SOURCE_DIR}/FramePresenter.h
 ${CMAKE_CURRENT_SOURCE_DIR}/FramePresenterGLUT.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NV_DEC_HDRS ${NV_DEC_HDRS} ${CMAKE_CURRENT_SOURCE_DIR}/FramePresenterGLX.h)
endif()

source_group( "headers" FILES ${NV_DEC_HDRS})
source_group( "sources" FILES ${APP_SOURCES} ${NV_DEC_SOURCES} ${NV_ENC_CUDA_UTILS})

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

cuda_add_executable(${PROJECT_NAME}  ${APP_SOURCES} ${NV_DEC_SOURCES} ${NV_DEC_HDRS} ${NV_ENC_CUDA_UTILS})

if (WIN32 AND CUDA_VERSION_MAJOR GREATER_EQUAL 11 AND CUDA_VERSION_MINOR GREATER_EQUAL 7)
    target_compile_options(${PROJECT_NAME} PRIVATE "/MT")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(${PROJECT_NAME} PUBLIC ${CUDA_INCLUDE_DIRS}
 ${NVCODEC_PUBLIC_INTERFACE_DIR}
 ${NVCODEC_UTILS_DIR}
 ${NV_CODEC_DIR}
 ${NV_APPDEC_COMMON_DIR}
 ${NV_FFMPEG_HDRS}
 ${NV_GL_HDRS}
 ${NV_GLEW_HDRS}
 ${NV_GLUT_HDRS}
)

if(WIN32)
    Set(AVCODEC_LIB ${FFMPEG_DIR}/lib/avcodec.lib)
    Set(AVFORMAT_LIB ${FFMPEG_DIR}/lib/avformat.lib)
    Set(AVUTIL_LIB ${FFMPEG_DIR}/lib/avutil.lib)
    Set(SWRESAMPLE_LIB ${FFMPEG_DIR}/lib/swresample.lib)
    Set(FREEGLUT_LIB ${GLUT_DIR}/lib/Release/freeglut.lib)
    Set(FREEGLUT_STATIC_LIB ${GLUT_DIR}/lib/Release/freeglut_static.lib)
    Set(FREEGLUT_STATICD_LIB ${GLUT_DIR}/lib/Debug/freeglut_staticd.lib)
    Set(FREEGLUTD_LIB ${GLUT_DIR}/lib/Debug/freeglutd.lib)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    	Set(GLEW32_LIB ${GLEW_DIR}/lib/Release/x64/glew32.lib)
    	Set(GLEW32S_LIB ${GLEW_DIR}/lib/Release/x64/glew32s.lib)
        Set(GLEW_DLL_DIR ${GLEW_DIR}/bin/Release/x64/)
        Set(GLEW_LIB_DIR ${GLEW_DIR}/lib/Release/x64/)
    else()
    	Set(GLEW32_LIB ${GLEW_DIR}/lib/Release/Win32/glew32.lib)
    	Set(GLEW32S_LIB ${GLEW_DIR}/lib/Release/Win32/glew32s.lib)
        Set(GLEW_DLL_DIR ${GLEW_DIR}/bin/Release/Win32/)
        Set(GLEW_LIB_DIR ${GLEW_DIR}/lib/Release/Win32/)
    endif()

    Set(GLUT_LIB_DIR ${GLUT_DIR}/lib)
    Set(GLUT_DLL_DIR ${GLUT_DIR}/bin)
    
    #copy .dll and .lib files
    install(DIRECTORY ${FFMPEG_DLL_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY ${FFMPEG_LIB_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.lib")
    
    install(DIRECTORY ${GLUT_LIB_DIR}/Release/ DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.lib")
    install(DIRECTORY ${GLUT_DLL_DIR}/Release/ DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY ${GLUT_LIB_DIR}/Debug/ DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.lib")
    install(DIRECTORY ${GLUT_DLL_DIR}/Debug/ DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.dll")
    
    install(DIRECTORY ${GLEW_LIB_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.lib")
    install(DIRECTORY ${GLEW_DLL_DIR} DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} FILES_MATCHING PATTERN "*.dll")
    
    target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} ${AVCODEC_LIB}
        ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB} ${FREEGLUT_LIB} ${FREEGLUT_STATIC_LIB} ${FREEGLUT_STATICD_LIB}
        ${FREEGLUTD_LIB} ${GLEW32_LIB} ${GLEW32S_LIB})
else ()
    find_library(FREEGLUT_LIB glut)
    find_library(GLEW32_LIB GLEW)
    find_library(X11_LIB X11)
    find_library(GL_LIB GL)
    find_library(CUDART_LIB cudart HINTS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)
    target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CUDART_LIB} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} ${AVCODEC_LIB}
        ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB} ${FREEGLUT_LIB} ${GLEW32_LIB} ${X11_LIB} ${GL_LIB})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/ )
    add_dependencies(${PROJECT_NAME} copyFFMPEGFiles)
    
    # copy ffmpeg, GLUT and GLEW .dll and .lib files
    file(GLOB externalLibList 
        ${GLUT_LIB_DIR}/Release/*.lib
        ${GLUT_LIB_DIR}/Debug/*.lib
        ${GLUT_DLL_DIR}/Release/*.dll
        ${GLUT_DLL_DIR}/Debug/*.dll
        ${GLEW_LIB_DIR}/*.lib 
        ${GLEW_DLL_DIR}/*.dll
    )
    
    foreach(externalLib ${externalLibList})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD  COMMAND ${CMAKE_COMMAND} -E copy ${externalLib} ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/)
    endforeach()
    
    #copy pdb's
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR}  CONFIGURATIONS Debug)
endif()

