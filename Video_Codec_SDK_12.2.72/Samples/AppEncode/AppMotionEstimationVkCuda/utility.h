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

/**
*  This file provides wrapper classes over standard objects from the Vulkan and
*  CUDA APIs, so that these objects can be used in a manner similar to the
*  instances of the base/derived classes for the decoder and the encoder.
*/

#include <vulkan/vulkan.h>
#include <cuda.h>
#include <vector>
#include <array>

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

/**
*  @brief Wrapper class around VkInstance
*/
class Vkinst
{
    VkInstance m_instance;
    VkDebugReportCallbackEXT m_callback;
    std::vector<VkPhysicalDevice> m_physicalDevices;

public:
    /*
     * The 'layers' parameter is a list of choices for validation layers.
     * The first entry in this list that is supported by the implementation is
     * selected and used when creating the VkInstance.
     */
    Vkinst(
        const std::vector<const char*>& layers = std::vector<const char*>(),
        const std::vector<const char*>& extensions = std::vector<const char*>()
    );
    ~Vkinst();

    const std::vector<VkPhysicalDevice>& getPhysicalDevices(void) const
    {
        return m_physicalDevices;
    }

    VkInstance get(void) const
    {
        return m_instance;
    }
};

class Vkcmdbuffer;
class Vksema;

/**
*  @brief Wrapper class around VkQueue
*/
class Vkque
{
    VkQueue m_queue;

    VkResult submit(
        const std::vector<VkSemaphore>& waitSemaphores = std::vector<VkSemaphore>(),
        const std::vector<VkCommandBuffer>& commandBuffers = std::vector<VkCommandBuffer>(),
        const std::vector<VkSemaphore>& signalSemaphores = std::vector<VkSemaphore>()
    );

public:
    Vkque(const VkQueue queue)
    {
        m_queue = queue;
    }

    ~Vkque(){}

    /**
    *  @brief Wrappers around vkQueueSubmit()
    */
    VkResult submit(const Vkcmdbuffer *commandBuffer);

    VkResult submit(
        const Vkcmdbuffer *commandBuffer,
        const Vksema *signalSemaphore
    );

    VkResult submit(
        const Vksema *waitSemaphore,
        const Vkcmdbuffer *commandBuffer,
        const Vksema *signalSemaphore
    );

    /**
    *  @brief Wrapper around vkQueueWaitIdle()
    */
    VkResult waitIdle(void);

    VkQueue get() const
    {
        return m_queue;
    }
};

/**
*  @brief Wrapper class around VkDevice
*/
class Vkdev
{
    VkDevice m_device;

    uint32_t m_transferQueueFamilyIndex;
    VkQueue m_transferQueue;

    VkPhysicalDeviceMemoryProperties m_deviceMemProps;

    std::array<uint8_t, VK_UUID_SIZE> m_deviceUUID;

public:
    Vkdev(
        const Vkinst *instance,
        const std::vector<const char*>& deviceExtensions = std::vector<const char*>()
    );
    ~Vkdev();

    uint32_t getTransferQueueFamilyIndex(void) const
    {
        return m_transferQueueFamilyIndex;
    }

    const VkPhysicalDeviceMemoryProperties& getMemoryProperties(void) const
    {
        return m_deviceMemProps;
    }

    const Vkque getTransferQueue(void) const
    {
        const static Vkque transferQueue(m_transferQueue);
        return transferQueue;
    }

    const std::array<uint8_t, VK_UUID_SIZE> getUUID(void) const
    {
        return m_deviceUUID;
    }

    VkDevice get() const
    {
        return m_device;
    }
};

/**
*  @brief Wrapper class around VkCommandPool
*/
class Vkcmdpool
{
    VkCommandPool m_commandPool;
    VkDevice m_device;

public:
    Vkcmdpool(const Vkdev *device);
    ~Vkcmdpool();

    VkCommandPool get() const
    {
        return m_commandPool;
    }
};

class Vkdevicemem;

/**
*  @brief Wrapper class around VkBuffer
*/
class Vkbuf
{
    VkBuffer m_buffer;
    VkDevice m_device;

    VkDeviceSize m_size;
    VkDeviceSize m_alignment;
    uint32_t m_memoryTypeBits;

public:
    Vkbuf(
        const Vkdev *device, VkDeviceSize bufferSize,
        VkBufferUsageFlags bufferUsage, bool exportCapable = false
    );
    ~Vkbuf();

    /**
    *  @brief Wrapper around vkBindBufferMemory()
    */
    void bind(const Vkdevicemem *deviceMem, VkDeviceSize offset = 0);

    VkDeviceSize getSize(void)
    {
        return m_size;
    }

    uint32_t getMemoryTypeBits(void)
    {
        return m_memoryTypeBits;
    }

    VkBuffer get() const
    {
        return m_buffer;
    }
};

/**
*  @brief Wrapper class around VkImage (specifically, 2D images)
*/
class Vkimg2d
{
    VkImage m_image;
    VkDevice m_device;

    VkExtent2D m_extent;
    VkDeviceSize m_size;
    VkDeviceSize m_alignment;
    uint32_t m_memoryTypeBits;

public:
    Vkimg2d(
        const Vkdev *device, VkExtent2D extent, VkImageUsageFlags imageUsage,
        bool exportCapable = false
    );
    ~Vkimg2d();

    /**
    *  @brief Wrapper around vkBindImageMemory()
    */
    void bind(const Vkdevicemem *deviceMem, VkDeviceSize offset = 0);

    VkDeviceSize getSize(void) const
    {
        return m_size;
    }

    VkDeviceSize getAlignment(void) const
    {
        return m_alignment;
    }

    VkExtent2D getExtent(void) const
    {
        return m_extent;
    }

    uint32_t getMemoryTypeBits(void)
    {
        return m_memoryTypeBits;
    }

    VkImage get() const
    {
        return m_image;
    }
};

/**
*  @brief Wrapper class around VkDeviceMemory
*/
class Vkdevicemem
{
    VkDeviceMemory m_deviceMemory;
    VkDevice m_device;
    VkDeviceSize m_size;

public:
    Vkdevicemem(
        const Vkdev *device, VkDeviceSize size, uint32_t memoryTypeBits,
        VkMemoryPropertyFlags memoryProperties, bool exportCapable = false
    );
    ~Vkdevicemem();

    /**
    *  @brief Wrapper around vkMapMemory()
    */
    VkResult map(
        void **p, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0
    );

    /**
    *  @brief Wrapper around vkUnmapMemory()
    */
    void unmap(void);

    void *getExportHandle(void) const;

    const VkDeviceMemory getMemory(void) const
    {
        return m_deviceMemory;
    }

    VkDeviceSize getSize(void) const
    {
        return m_size;
    }

    VkDeviceMemory get() const
    {
        return m_deviceMemory;
    }
};

class Vkimgmembarrier;

/**
*  @brief Wrapper class around VkCommandBuffer
*/
class Vkcmdbuffer
{
    VkCommandBuffer m_commandBuffer;
    VkDevice m_device;
    VkCommandPool m_commandPool;

public:
    Vkcmdbuffer(const Vkdev *device, const Vkcmdpool *commandPool);
    ~Vkcmdbuffer();

    /**
    *  @brief Wrappers around vk{Begin,End}CommandBuffer()
    */
    VkResult begin(void);
    VkResult end(void);

    /**
    *  @brief Wrapper around vkCmdFillBuffer()
    */
    void fillBuffer(const Vkbuf *buffer, uint32_t data,
        VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    /**
    *  @brief Wrapper around vkCmdCopyBuffer()
    */
    void copyBuffer(const Vkbuf *dstBuffer, const Vkbuf *srcBuffer,
        VkDeviceSize size = VK_WHOLE_SIZE);

    /**
    *  @brief Insert a pipeline barrier (wrapper around vkCmdPipelineBarrier())
    */
    void pipelineBarrier(
        const Vkimgmembarrier *imageBarrier,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask
    );

    /**
    *  @brief Wrapper around vkCmdClearColorImage()
    */
    void clearImage(const Vkimg2d *image, VkClearColorValue value);

    /**
    *  @brief Wrapper around vkCmdCopyImageToBuffer()
    */
    void copyImageToBuffer(const Vkbuf *buffer, const Vkimg2d *image);

    /**
    *  @brief Wrapper around vkCmdCopyBufferToImage()
    */
    void copyBufferToImage(const Vkimg2d *image, const Vkbuf *buffer);

    VkCommandBuffer get() const
    {
        return m_commandBuffer;
    }
};

/**
*  @brief Wrapper class around VkSemaphore
*/
class Vksema
{
    VkSemaphore m_semaphore;
    VkDevice m_device;

public:
    Vksema(const Vkdev *device, bool exportCapable = false);
    ~Vksema();

    void *getExportHandle(void) const;

    VkSemaphore get() const
    {
        return m_semaphore;
    }
};

/**
*  @brief Wrapper class around VkImageMemoryBarrier
*/
class Vkimgmembarrier
{
    VkImageMemoryBarrier m_barrier;

public:
    Vkimgmembarrier(const Vkimg2d *image);
    ~Vkimgmembarrier(){};

    VkImageMemoryBarrier get() const
    {
        return m_barrier;
    }
};

/**
*  @brief Wrapper class around CUcontext
* This class can be used for creating CUDA contexts on the device referenced
* by the provided Vkdev instance. The methods provided in this class are
* wrappers over CUDA API calls and take care of pushing/popping the CUDA
* context as required.
*/
class Cudactx
{
    CUcontext m_context;

public:
    Cudactx(const Vkdev *device);
    ~Cudactx(){};

    CUresult memcpyDtoH(void *p, CUdeviceptr dptr, size_t size);

    CUresult memcpy2D(void *p, CUarray array, uint32_t width, uint32_t height);

    CUcontext get() const
    {
        return m_context;
    }
};

/**
*  @brief Wrapper class around CUdeviceptr
* This class can be used for mapping a CUdeviceptr allocation on the device
* memory object referred to by deviceMem. deviceMem should have been created
* with a device memory object backing a VkBuffer. This mapping makes use of
* Vulkan's export of device memory followed by import of this external memory
* by CUDA.
*/
class Cudabuffer
{
    CUdeviceptr m_deviceptr;
    CUexternalMemory m_extMem;

public:
    Cudabuffer(const Vkdevicemem *deviceMem);
    ~Cudabuffer();

    CUdeviceptr get() const
    {
        return m_deviceptr;
    }
};

/**
*  @brief Wrapper class around CUarray
* This class can be used for mapping a 2D CUDA array on the device memory
* object referred to by deviceMem. deviceMem should have been created with a
* device memory object backing a 2D VkImage. This mapping makes use of Vulkan's
* export of device memory followed by import of this external memory by CUDA.
*/
class Cudaimage
{
    CUarray m_array;
    CUmipmappedArray m_mipmapArray;
    CUexternalMemory m_extMem;

public:
    Cudaimage(const Vkimg2d *image, const Vkdevicemem *deviceMem);
    ~Cudaimage();

    CUarray get() const
    {
        return m_array;
    }
};

/**
*  @brief Wrapper class around CUexternalSemaphore
* This class can be used for creating a CUDA external semaphore from a
* VkSemaphore (referred to by the provided Vksema instance). This makes use of
* Vulkan's export of semaphores followed by their import by CUDA.
*/
class Cudasema
{
    CUexternalSemaphore m_extSema;

public:
    Cudasema(const Vksema *semaphore);
    ~Cudasema();

    CUresult wait(void);
    CUresult signal(void);
};
