/*
* Copyright 2017-2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

#include "NvEnc.h"

#define CUDA_DRVAPI_CALL( call )                                                                                                 \
    do                                                                                                                           \
    {                                                                                                                            \
        CUresult err__ = call;                                                                                                   \
        if (err__ != CUDA_SUCCESS)                                                                                               \
        {                                                                                                                        \
            const char *szErrName = NULL;                                                                                        \
            cuGetErrorName(err__, &szErrName);                                                                                   \
            std::ostringstream errorLog;                                                                                         \
            errorLog << "CUDA driver API error " << szErrName ;                                                                  \
            throw NVENCException::makeNVENCException(errorLog.str(), NV_ENC_ERR_GENERIC, __FUNCTION__, __FILE__, __LINE__);      \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

NvEnc::NvEnc(CUcontext cuContext, uint32_t nWidth, uint32_t nHeight, NV_ENC_BUFFER_FORMAT eBufferFormat,
    uint32_t nExtraOutputDelay, bool bMotionEstimationOnly):
    NvEncoder(NV_ENC_DEVICE_TYPE_CUDA, cuContext, nWidth, nHeight, eBufferFormat, nExtraOutputDelay, bMotionEstimationOnly),
    m_cuContext(cuContext)
{
    if (!m_hEncoder) 
    {
        NVENC_THROW_ERROR("Encoder Initialization failed", NV_ENC_ERR_INVALID_DEVICE);
    }

    if (!m_cuContext)
    {
        NVENC_THROW_ERROR("Invalid Cuda Context", NV_ENC_ERR_INVALID_DEVICE);
    }
}

NvEnc::~NvEnc()
{
    ReleaseCudaResources();
}

void NvEnc::AllocateInputBuffers(int32_t numInputBuffers)
{
    // We're not going to actually allocate buffers here because it is the
    // caller's responsibility to do that. This method has only been defined
    // because NvEncoder::AllocateInputBuffers() is a pure virtual function.
}

void NvEnc::ReleaseInputBuffers()
{
    // Since no input buffers were allocated, we're not going to free them.
}

void NvEnc::RegisterInputResources(std::vector<void*> inputframes,
    NV_ENC_INPUT_RESOURCE_TYPE eResourceType, int width, int height, int pitch,
    NV_ENC_BUFFER_FORMAT bufferFormat, bool bRegisterAsReferences
)
{
    NvEncoder::RegisterInputResources(inputframes, eResourceType, width, height,
        pitch, bufferFormat, bRegisterAsReferences);
}

void NvEnc::UnregisterInputResources()
{
    NvEncoder::UnregisterInputResources();
}

void NvEnc::ReleaseCudaResources()
{
    if (!m_hEncoder)
    {
        return;
    }

    if (!m_cuContext)
    {
        return;
    }

    UnregisterInputResources();

    m_cuContext = nullptr;
}
