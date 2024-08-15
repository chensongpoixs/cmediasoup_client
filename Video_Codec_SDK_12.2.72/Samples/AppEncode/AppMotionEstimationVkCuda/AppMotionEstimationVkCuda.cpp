/*
* Copyright 2018-2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/


/*
 * This sample application demonstrates feeding of CUarrays to EncodeAPI
 * for the purposes of motion estimation between pairs of frames, using the
 * H.264 motion estimation-only mode. The CUarrays registered with EncodeAPI
 * have not been created by the application but have been obtained through the
 * interop of CUDA with the Vulkan graphics API.
 */

#include "utility.h"
#include "NvEnc.h"
#include "../Utils/NvEncoderCLIOptions.h"
#include "../Utils/NvCodecUtils.h"

#include <iostream>
#include <memory>
#include <cstring>
#include <map>

#ifdef _WIN32
#define strcasecmp _stricmp 
#endif

#define NUM_BUFFERS 2

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

/*
 * A structure for tying together the following pieces of information:
 * - a VkImage and its backing device memory allocation
 * - the CUarray obtained via Vulkan export + CUDA external memory import of
 *   the image's backing memory allocation
 * - a Vulkan semaphore object (for synchronizing accesses to the VkImage) and
 *   the equivalent CUDA external semaphore object
 * - image memory barriers associated with operations on the VkImage
 */
struct DeviceAlloc
{
    Vkimg2d *vulkanImage;
    Vkdevicemem *vulkanImageDeviceMemory;
    Vksema *vulkanSemaphore;
    Vkimgmembarrier *preOpBarrier;
    Vkimgmembarrier *postOpBarrier;
    Cudaimage *cudaImage;;
    Cudasema *cudaSemaphore;
};

/*
 * A structure for tying together a VkBuffer and its backing memory.
 */
struct DeviceBuffer
{
    Vkbuf *vulkanBuffer;
    Vkdevicemem *vulkanBufferDeviceMemory;
};

const std::vector<const char*> requestedLayers = {
#if defined(USE_VALIDATION_LAYERS)
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_standard_validation"
#endif
};

const std::vector<const char*> requestedExtensions = {
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
};


const std::vector<const char*> requestedDeviceExtensions = {
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
#ifndef _WIN32
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME
#else
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
    VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
#endif
};

static void ShowHelpAndExit(const char *szBadOption = NULL)
{
    bool bThrowError = false;
    std::ostringstream oss;
    if (szBadOption)
    {
        bThrowError = true;
        oss << "Error parsing \"" << szBadOption << "\"" << std::endl;
    }
    oss << "Options:" << std::endl
        << "-i           Input file path" << std::endl
        << "-o           Output file path" << std::endl
        << "-s           Input resolution in this form: WxH" << std::endl
        << "-h           Print this help message" << std::endl
        ;
    if (bThrowError)
    {
        throw std::invalid_argument(oss.str());
    }
    else
    {
        std::cout << oss.str();
        exit(0);
    }
}

static void ParseCommandLine(int argc, char *argv[], char *szInputFileName,
    int &nWidth, int &nHeight, char *szOutputFileName, NvEncoderInitParam &initParam
)
{
    std::ostringstream oss;
    int i;
    for (i = 1; i < argc; i++)
    {
        if (!strcasecmp(argv[i], "-h"))
        {
            ShowHelpAndExit();
        }
        if (!strcasecmp(argv[i], "-i"))
        {
            if (++i == argc)
            {
                ShowHelpAndExit("-i");
            }
            sprintf(szInputFileName, "%s", argv[i]);
            continue;
        }
        if (!strcasecmp(argv[i], "-o"))
        {
            if (++i == argc)
            {
                ShowHelpAndExit("-o");
            }
            sprintf(szOutputFileName, "%s", argv[i]);
            continue;
        }
        if (!strcasecmp(argv[i], "-s"))
        {
            if (++i == argc || 2 != sscanf(argv[i], "%dx%d", &nWidth, &nHeight))
            {
                ShowHelpAndExit("-s");
            }
            continue;
        }

        // Regard as encoder parameter
        if (argv[i][0] != '-')
        {
            ShowHelpAndExit(argv[i]);
        }
        oss << argv[i] << " ";
        while (i + 1 < argc && argv[i + 1][0] != '-')
        {
            oss << argv[++i] << " ";
        }
    }
    initParam = NvEncoderInitParam(oss.str().c_str());
}

/*
 * Uploads data from the supplied VkBuffer to the supplied VkImage.
 * `queue` must support transfer operations and `commandBuffer` must be a
 * command buffer from a command pool associated with the provided queue.
 */
static void UploadData(Vkcmdbuffer *commandBuffer, Vkque *queue,
    const DeviceAlloc *surf, const Vkbuf *buffer)
{
    VkResult result = VK_SUCCESS;

    result = commandBuffer->begin();
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to start recording commands");
    }

    /*
     * Transition the image layout from UNDEFINED to DST_OPTIMAL for a copy
     * operation.
     */
    commandBuffer->pipelineBarrier(surf->preOpBarrier,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, VK_ACCESS_TRANSFER_WRITE_BIT);

    commandBuffer->copyBufferToImage(surf->vulkanImage, buffer);

    /*
     * Transition the image layout from DST_OPTIMAL to GENERAL for other
     * uses of the image.
     */
    commandBuffer->pipelineBarrier(surf->postOpBarrier,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);

    result = commandBuffer->end();
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record commands");
    }

    queue->submit(commandBuffer, surf->vulkanSemaphore);
}

void RunMotionEstimation(
    char *szInFilePath, int nWidth, int nHeight, char *szOutFilePath,
    NvEncoderInitParam *pEncodeCLIOptions
)
{
    VkResult result = VK_SUCCESS;
    CUresult res = CUDA_SUCCESS;

    std::map<CUarray, DeviceAlloc*> mapCUarrayToDeviceAlloc;
    DeviceAlloc surfaces[NUM_BUFFERS] = {};
    DeviceBuffer buffers[NUM_BUFFERS] = {};

    /*
     * Consider only YUV 4:2:0 frames for now.
     */
    VkExtent2D extent = { (uint32_t)nWidth, (uint32_t)(nHeight + (nHeight + 1) / 2) };
    VkDeviceSize imageSize = extent.width * extent.height;
    VkDeviceSize bufferSize = imageSize;

    NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV;

    std::ifstream fpIn(szInFilePath, std::ios::in | std::ios::binary);
    if (!fpIn)
    {
        std::ostringstream err;
        err << "Unable to open input file: " << szInFilePath << std::endl;
        throw std::invalid_argument(err.str());
    }

    std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
    if (!fpOut)
    {
        std::ostringstream err;
        err << "Unable to open output file: " << szOutFilePath << std::endl;
        throw std::invalid_argument(err.str());
    }

    Vkinst instance(requestedLayers, requestedExtensions);

    Vkdev device(&instance, requestedDeviceExtensions);

    Vkque queue = device.getTransferQueue();

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    /*
     * Create Vulkan images, allocate the device memory backing them and
     * associated staging buffers.
     */
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        Vkimg2d *image = new Vkimg2d(&device, extent, usageFlags, true);

        Vkdevicemem *imgMem = new Vkdevicemem(&device, image->getSize(),
                                      image->getMemoryTypeBits(),
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      true);

        image->bind(imgMem);

        Vksema *sema = new Vksema(&device, true);

        surfaces[i].vulkanImage = image;
        surfaces[i].vulkanImageDeviceMemory = imgMem;
        surfaces[i].vulkanSemaphore = sema;

        Vkbuf *buffer = new Vkbuf(&device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        Vkdevicemem *bufMem = new Vkdevicemem(&device, buffer->getSize(),
                                      buffer->getMemoryTypeBits(),
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        buffer->bind(bufMem);

        buffers[i].vulkanBuffer = buffer;
        buffers[i].vulkanBufferDeviceMemory = bufMem;
    }

    Vkcmdpool commandPool(&device);
    Vkcmdbuffer commandBuffer(&device, &commandPool);

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        Vkimg2d *image = surfaces[i].vulkanImage;

        surfaces[i].preOpBarrier = new Vkimgmembarrier(image);
        surfaces[i].postOpBarrier = new Vkimgmembarrier(image);
    }

    Cudactx context(&device);

    /*
     * Obtain CUDA-side objects equivalent to the Vulkan images and semaphores
     * created earlier.
     */
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        Cudaimage *cuImage = new Cudaimage(surfaces[i].vulkanImage,
                                     surfaces[i].vulkanImageDeviceMemory);

        Cudasema *cuSema = new Cudasema(surfaces[i].vulkanSemaphore);

        surfaces[i].cudaImage = cuImage;
        surfaces[i].cudaSemaphore = cuSema;

        mapCUarrayToDeviceAlloc[cuImage->get()] = &surfaces[i];
    }

    NvEnc enc(context.get(), nWidth, nHeight, eFormat, 0, true);

    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    initializeParams.encodeConfig = &encodeConfig;
    enc.CreateDefaultEncoderParams(&initializeParams,
        NV_ENC_CODEC_H264_GUID,
        NV_ENC_PRESET_P6_GUID);

    pEncodeCLIOptions->SetInitParams(&initializeParams, eFormat);

    enc.CreateEncoder(&initializeParams);

    assert(imageSize == enc.GetFrameSize());

    struct stat st;

    if (stat(szInFilePath, &st) != 0)
    {
        std::ostringstream err;
        err << "Failed to stat file \"" << szInFilePath << "\"" << std::endl;
        throw std::invalid_argument(err.str());
    }

    uint32_t numFrames = static_cast<uint32_t>(st.st_size / imageSize);

    if (numFrames < 2)
    {
        std::ostringstream err;
        err << "At least 2 frames are needed for motion estimation." << std::endl;
        throw std::invalid_argument(err.str());
    }

    std::vector<void *> inputFrames, refFrames;

    inputFrames.push_back((void *)surfaces[0].cudaImage->get());
    enc.RegisterInputResources(inputFrames, NV_ENC_INPUT_RESOURCE_TYPE_CUDAARRAY,
         nWidth, nHeight, nWidth, eFormat);

    refFrames.push_back((void *)surfaces[1].cudaImage->get());
    enc.RegisterInputResources(refFrames, NV_ENC_INPUT_RESOURCE_TYPE_CUDAARRAY,
         nWidth, nHeight, nWidth, eFormat, true);

    char *ptr = nullptr;
    std::vector<uint8_t> vPacket;
    int inputBufferIdx = 1, refBufferIdx = 0;

    /*
     * Load the first frame (frame idx 0) for later upload to the reference
     * image.
     */
    result = buffers[refBufferIdx].vulkanBufferDeviceMemory->map(
                 reinterpret_cast<void **>(&ptr), bufferSize);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map host buffer");
    }

    fpIn.read(ptr, bufferSize);

    buffers[refBufferIdx].vulkanBufferDeviceMemory->unmap();

    for (uint32_t idx = 0; idx < numFrames - 1; idx++)
    {
        CUarray refArray = (CUarray)enc.GetNextReferenceFrame()->inputPtr;
        const DeviceAlloc *refSurf = mapCUarrayToDeviceAlloc[refArray];

        /*
         * The input frame for the previous motion estimation call is the
         * reference frame for the current motion estimation call. Upload
         * data from the previously-mapped buffer to the reference image
         */
        UploadData(&commandBuffer, &queue, refSurf, buffers[refBufferIdx].vulkanBuffer);


        CUarray inputArray = (CUarray)enc.GetNextInputFrame()->inputPtr;
        const DeviceAlloc *inputSurf = mapCUarrayToDeviceAlloc[inputArray];

        // Upload data to current surface
        result = buffers[inputBufferIdx].vulkanBufferDeviceMemory->map(
                     reinterpret_cast<void **>(&ptr), bufferSize);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map host buffer");
        }

        fpIn.read(ptr, bufferSize);

        buffers[inputBufferIdx].vulkanBufferDeviceMemory->unmap();

        UploadData(&commandBuffer, &queue, inputSurf, buffers[inputBufferIdx].vulkanBuffer);

        /*
         * We don't need a call to queue.waitIdle() here to ensure that CUDA work
         * will start only after submission of commands from the Vulkan side.
         * This is because semaphores are in the unsignaled state by default when
         * they are created, and a wait() will block until somebody/something
         * calls signal(). In this case, the signal() comes from Vulkan's side
         * after the completion of the submitted commands, so CUDA is guaranteed
         * to wait() for it.
         */

        refSurf->cudaSemaphore->wait();
        inputSurf->cudaSemaphore->wait();

        enc.RunMotionEstimation(vPacket);

        fpOut << "Motion Vectors for input frame = " << idx + 1 << ", reference frame = " << idx << std::endl;

        int numMBs = ((nWidth + 15) / 16) * ((nHeight + 15) / 16);
        fpOut << "block, mb_type, partitionType, "
            << "MV[0].x, MV[0].y, MV[1].x, MV[1].y, MV[2].x, MV[2].y, MV[3].x, MV[3].y, cost" << std::endl;

        // Parse the output from the API to obtain human-readable motion vectors
        NV_ENC_H264_MV_DATA *outputMV = (NV_ENC_H264_MV_DATA *)vPacket.data();
        for (int l = 0; l < numMBs; l++)
        {
            fpOut << l << ", " << static_cast<int>(outputMV[l].mbType) << ", " << static_cast<int>(outputMV[l].partitionType) << ", " <<
                outputMV[l].mv[0].mvx << ", " << outputMV[l].mv[0].mvy << ", " << outputMV[l].mv[1].mvx << ", " << outputMV[l].mv[1].mvy << ", " <<
                outputMV[l].mv[2].mvx << ", " << outputMV[l].mv[2].mvy << ", " << outputMV[l].mv[3].mvx << ", " << outputMV[l].mv[3].mvy << ", " << outputMV[l].mbCost;
            fpOut << std::endl;
        }

        vPacket.clear();

        refBufferIdx = inputBufferIdx;
        inputBufferIdx = refBufferIdx ^ 1;
    }

    enc.UnregisterInputResources();
    enc.DestroyEncoder();

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        delete surfaces[i].cudaSemaphore;
        delete surfaces[i].cudaImage;

        delete surfaces[i].preOpBarrier;
        delete surfaces[i].postOpBarrier;

        delete surfaces[i].vulkanSemaphore;

        delete surfaces[i].vulkanImageDeviceMemory;
        delete surfaces[i].vulkanImage;

        delete buffers[i].vulkanBufferDeviceMemory;
        delete buffers[i].vulkanBuffer;

    }
}

int main(int argc, char **argv)
{
    char szInFilePath[256] = "",
        szOutFilePath[256] = "";
    int nWidth = 0, nHeight = 0;

    try
    {
        NvEncoderInitParam encodeCLIOptions;
        ParseCommandLine(argc, argv, szInFilePath, nWidth, nHeight,
            szOutFilePath, encodeCLIOptions);

        CheckInputFile(szInFilePath);
        ValidateResolution(nWidth, nHeight);

        if (!*szOutFilePath)
        {
            sprintf(szOutFilePath, "out.txt");
        }

        RunMotionEstimation(szInFilePath, nWidth, nHeight, szOutFilePath, &encodeCLIOptions);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
