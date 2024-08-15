# Copyright 2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(AppEncD3D12)

set(APP_SOURCES
 ${CMAKE_CURRENT_SOURCE_DIR}/AppEncD3D12.cpp
)

set(NV_ENC_SOURCES
 ${NV_ENC_DIR}/NvEncoder.cpp
 ${NV_ENC_DIR}/NvEncoderD3D12.cpp
)

set(NV_ENC_HDRS
 ${NV_ENC_DIR}/NvEncoder.h
 ${NV_ENC_DIR}/NvEncoderD3D12.h
 ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
 ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
 ${NVCODEC_UTILS_DIR}/NvEncoderCLIOptions.h
 ${NV_APPENC_COMMON_DIR}/AppEncUtils.h
)

source_group( "headers" FILES ${NV_ENC_HDRS} )
source_group( "sources" FILES ${APP_SOURCES} ${NV_ENC_SOURCES} )

add_compile_definitions(AGILITY_SDK_VER=${AGILITY_SDK_VER})
add_executable(${PROJECT_NAME}  ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_HDRS})

target_include_directories(${PROJECT_NAME} PUBLIC
 ${NVCODEC_PUBLIC_INTERFACE_DIR}
 ${NVCODEC_UTILS_DIR}
 ${NV_CODEC_DIR}
 ${NV_ENC_COMMON_DIR}
)

set(D3D12_LIBS d3d12.lib dxgi.lib d3dcompiler.lib)

target_link_libraries(${PROJECT_NAME} ${D3D12_LIBS} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})

# create directory and copy D3D12Core.dll 
install(FILES ${AGILITY_SDK_BIN}/D3D12Core.dll DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR}/D3D12)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/ )    
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR} CONFIGURATIONS Debug) 
    # create directory and copy D3D12Core.dll 
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD  COMMAND ${CMAKE_COMMAND} -E make_directory ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/D3D12)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD  COMMAND ${CMAKE_COMMAND} -E copy ${AGILITY_SDK_BIN}/D3D12Core.dll ${NVCODEC_SAMPLES_INSTALL_DIR}/$<CONFIG>/D3D12/D3D12Core.dll)
endif()



