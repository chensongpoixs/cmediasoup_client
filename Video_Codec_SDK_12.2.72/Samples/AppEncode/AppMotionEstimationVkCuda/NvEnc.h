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

#pragma once

#include <vector>
#include <stdint.h>
#include <mutex>
#include <cuda.h>
#include "NvEncoder/NvEncoder.h"


class NvEnc : public NvEncoder
{
public:
    NvEnc(CUcontext cuContext, uint32_t nWidth, uint32_t nHeight, NV_ENC_BUFFER_FORMAT eBufferFormat,
        uint32_t nExtraOutputDelay = 3, bool bMotionEstimationOnly = false);
    virtual ~NvEnc();

    void RegisterInputResources(std::vector<void*> inputframes, NV_ENC_INPUT_RESOURCE_TYPE eResourceType,
        int width, int height, int pitch, NV_ENC_BUFFER_FORMAT bufferFormat,
        bool bRegisterAsReferences = false);

    void UnregisterInputResources();

private:
    /**
    *  @brief This function is used to allocate input buffers for encoding.
    *  This function is an override of virtual function NvEncoder::AllocateInputBuffers().
    */
    virtual void AllocateInputBuffers(int32_t numInputBuffers) override;

    /**
    *  @brief This function is used to release the input buffers allocated for encoding.
    *  This function is an override of virtual function NvEncoder::ReleaseInputBuffers().
    */
    virtual void ReleaseInputBuffers() override;
private:
    /**
    *  @brief This is a private function to release CUDA device memory used for encoding.
    */
    void ReleaseCudaResources();
private:
    size_t m_cudaPitch = 0;
    CUcontext m_cuContext;
};
