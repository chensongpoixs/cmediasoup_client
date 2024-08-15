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

#include "utility.h"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sstream>

#ifndef _WIN32
#define EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
#define EXTERNAL_SEMAPHORE_HANDLE_SUPPORTED_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR
#else
#define EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
#define EXTERNAL_SEMAPHORE_HANDLE_SUPPORTED_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
#endif

static const char* getFirstSupportedValidationLayer(
    const std::vector<const char*>& layers
)
{
    uint32_t numProperties = 0;

    vkEnumerateInstanceLayerProperties(&numProperties, nullptr);

    std::vector<VkLayerProperties> availLayers(numProperties);
    vkEnumerateInstanceLayerProperties(&numProperties, availLayers.data());

    for (const char *req : layers) {
        for (const auto& layer : availLayers) {
            if (!std::strcmp(req, layer.layerName)) {
                return req;
            }
        }
    }

    return nullptr;
}

/*
 * Returns a list of instance extensions supported by the implementation,
 * which may be a subset of the extensions requested in the
 * requestedExtensions vector.
 */
static std::vector<const char*> getAvailableExtensions(
    const std::vector<const char*>& extensions
)
{
    uint32_t numExtensions = 0;
    std::vector<const char *> availableExts;

    vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

    std::vector<VkExtensionProperties> exts(numExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr,
        &numExtensions, exts.data());

    for (const char *req : extensions) {
        bool foundExt = false;

        for (const auto& ext : exts) {
            if (!std::strcmp(req, ext.extensionName)) {
                foundExt = true;
                break;
            }
        }

        if (foundExt) {
            availableExts.push_back(req);
        }
    }

    return availableExts;
}

/*
 * Returns a list of device extensions supported by the implementation, which
 * may be a subset of the extensions requested in the requestedDeviceExtensions
 * vector.
 */
static std::vector<const char*> getSupportedDeviceExtensions(
    VkPhysicalDevice phyDevice,
    const std::vector<const char*>& deviceExtensions
)
{
    uint32_t numExtensions = 0;
    std::vector<const char *> availableExts;

    vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &numExtensions,
        nullptr);

    std::vector<VkExtensionProperties> exts(numExtensions);
    vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &numExtensions,
        exts.data());

    for (const char *req : deviceExtensions) {
        bool foundExt = false;

        for (const auto& ext : exts) {
            if (!std::strcmp(req, ext.extensionName)) {
                foundExt = true;
                break;
            }
        }

        if (foundExt) {
            availableExts.push_back(req);
        }
    }

    return availableExts;
}

static uint32_t findMemoryType(
    const Vkdev& device, uint32_t memoryTypeBits, VkMemoryPropertyFlags memProps
)
{
    const VkPhysicalDeviceMemoryProperties& deviceMemProps = device.getMemoryProperties();

    for (uint32_t i = 0; i < deviceMemProps.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) &&
            ((memProps & deviceMemProps.memoryTypes[i].propertyFlags) == memProps)) {
            return i;
        }
    }

    return -1;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData
)
{
    std::cerr << "validation layer: " << msg << std::endl;

    // The return value informs the validation layer using this callback
    // about whether the API call triggering this callback should be
    // aborted (i.e. VK_TRUE => abort)
    return VK_FALSE;
}

static void getDeviceUUID(
    VkInstance instance, VkPhysicalDevice phyDevice,
    std::array<uint8_t, VK_UUID_SIZE>& deviceUUID
)
{
    /*
     * Query the physical device properties to obtain the device UUID.
     * Note that successfully loading vkGetPhysicalDeviceProperties2KHR()
     * requires the VK_KHR_get_physical_device_properties2 extension
     * (which is an instance-level extension) to be enabled.
     */
    VkPhysicalDeviceIDPropertiesKHR deviceIDProps = {};
    deviceIDProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2KHR props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    props.pNext = &deviceIDProps;

    auto func = (PFN_vkGetPhysicalDeviceProperties2KHR) \
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
    if (func == nullptr) {
        throw std::runtime_error("Failed to load vkGetPhysicalDeviceProperties2KHR");
    }

    func(phyDevice, &props);

    std::memcpy(deviceUUID.data(), deviceIDProps.deviceUUID, VK_UUID_SIZE);
}


/*
 * Definitions for the methods from class Vkinst.
 */
Vkinst::Vkinst(
    const std::vector<const char*>& layers,
    const std::vector<const char*>& extensions
)
{
    bool enableValidationLayers = layers.size() > 0;
    const char* layerToEnable = nullptr;

    if (enableValidationLayers) {
        layerToEnable = getFirstSupportedValidationLayer(layers);

        if (!layerToEnable) {
            throw std::runtime_error("Validation layers requested, "
                "but could not be enabled");
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vk_cu_interop";
    appInfo.applicationVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    if (enableValidationLayers) {
        instanceInfo.enabledLayerCount = 1;
        instanceInfo.ppEnabledLayerNames = &layerToEnable;
    }

    auto availableExts = getAvailableExtensions(extensions);

    instanceInfo.enabledExtensionCount = (uint32_t)availableExts.size();
    instanceInfo.ppEnabledExtensionNames = availableExts.data();

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a VK instance.");
    }

    uint32_t numPhyDevices = 0;

    result = vkEnumeratePhysicalDevices(m_instance, &numPhyDevices, nullptr);
    if (result != VK_SUCCESS) {
        std::ostringstream oss;
        oss << "vkEnumeratePhysicalDevices returned " << result;
        throw std::runtime_error(oss.str());
    } else if (numPhyDevices == 0) {
        throw std::runtime_error("No physical devices found");
    }

    m_physicalDevices.resize(numPhyDevices);

    vkEnumeratePhysicalDevices(m_instance, &numPhyDevices, m_physicalDevices.data());

    // If validation layers are not going to be used, don't attempt to register
    // the debug report callback.
    if (!enableValidationLayers) {
        return;
    }

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                       VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    auto func = (PFN_vkCreateDebugReportCallbackEXT) \
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
    if (func == nullptr) {
        throw std::runtime_error("Failed to load callback register extn");
    }

    result = func(m_instance, &createInfo, nullptr, &m_callback);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to register the callback");
    }
}

Vkinst::~Vkinst()
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) \
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr && m_callback != VK_NULL_HANDLE) {
        func(m_instance, m_callback, nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);
}


/*
 * Definitions for the methods from class Vkdev.
 */
Vkdev::Vkdev(
    const Vkinst *instance,
    const std::vector<const char*>& deviceExtensions
)
{
    VkResult result = VK_SUCCESS;
    VkPhysicalDevice phyDevice = VK_NULL_HANDLE;
    const auto& physicalDevices = instance->getPhysicalDevices();

    /*
     * Iterate over all available physical devices (queried as part of the
     * creation of the Vkinst instance) and identify a suitable one. Currently,
     * the only criteria for selecting a physical device are that it should be
     * a discrete GPU and must support at least one transfer queue instance.
     */
    for (auto& dev : physicalDevices) {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(dev, &props);

        if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }

        uint32_t numQueueFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &numQueueFamilies, nullptr);

        std::vector<VkQueueFamilyProperties> familyProps(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &numQueueFamilies,
            familyProps.data());

        uint32_t index = 0;

        for (const auto& prop : familyProps) {
            if ((prop.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                (prop.queueCount > 0)) {
                break;
            }

            index++;
        }

        if (index != familyProps.size()) {
            m_transferQueueFamilyIndex = index;
            phyDevice = dev;
            break;
        }
    }

    if (phyDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable physical device");
    }

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_transferQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    auto extensions = getSupportedDeviceExtensions(phyDevice, deviceExtensions);

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    result = vkCreateDevice(phyDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a device");
    }

    vkGetDeviceQueue(m_device, m_transferQueueFamilyIndex, 0, &m_transferQueue);

    // Fetch the memory properties associated with the physical device
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &m_deviceMemProps);

    // Fetch the device UUID for later use (when exporting resources to CUDA)
    getDeviceUUID(instance->get(), phyDevice, m_deviceUUID);

    /*
     * Query the implementation to check that exporting backing memory for a
     * buffer as an fd is supported.
     */
    VkPhysicalDeviceExternalBufferInfoKHR phyBufInfo = {};
    phyBufInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO_KHR;
    phyBufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    phyBufInfo.handleType = EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;

    VkExternalBufferPropertiesKHR bufProps = {};
    bufProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHR;

    auto func = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR) \
        vkGetInstanceProcAddr(instance->get(), "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
    if (!func) {
        throw std::runtime_error("Failed to load "
            "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
    }

    func(phyDevice, &phyBufInfo, &bufProps);

    const VkExternalMemoryPropertiesKHR& extMemProps = bufProps.externalMemoryProperties;
    if (!(extMemProps.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(extMemProps.compatibleHandleTypes & EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE)) {
        throw std::runtime_error("The buffer cannot be exported");
    }

    /*
     * Query the implementation to check that exporting the payload for a
     * semaphore as an fd is supported.
     */
    VkPhysicalDeviceExternalSemaphoreInfoKHR phySemaInfo = {};
    phySemaInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO_KHR;

    phySemaInfo.handleType = EXTERNAL_SEMAPHORE_HANDLE_SUPPORTED_TYPE;


    VkExternalSemaphorePropertiesKHR semaProps = {};
    semaProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES_KHR;

    auto func2 = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR) \
        vkGetInstanceProcAddr(instance->get(), "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
    if (!func2) {
        throw std::runtime_error("Failed to load "
            "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
    }

    func2(phyDevice, &phySemaInfo, &semaProps);

    if (!(semaProps.externalSemaphoreFeatures &
          VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR)) {
        throw std::runtime_error("The semaphore cannot be exported");
    }

    /*
     * Query the implementation to check that exporting backing memory for
     * an image as an fd is supported.
     */
    VkPhysicalDeviceExternalImageFormatInfoKHR extImageFormatInfo = {};
    extImageFormatInfo.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHR;

    extImageFormatInfo.handleType = EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;


    VkPhysicalDeviceImageFormatInfo2KHR imageFormatInfo = {};
    imageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR;
    imageFormatInfo.pNext = &extImageFormatInfo;
    imageFormatInfo.format = VK_FORMAT_R8_UINT;
    imageFormatInfo.type = VK_IMAGE_TYPE_2D;
    imageFormatInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageFormatInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkExternalImageFormatPropertiesKHR extImageFormatProperties = {};
    extImageFormatProperties.sType =
        VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES_KHR;

    VkImageFormatProperties2KHR imageFormatProperties = {};
    imageFormatProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR;
    imageFormatProperties.pNext = &extImageFormatProperties;

    auto func3 = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR) \
        vkGetInstanceProcAddr(instance->get(), "vkGetPhysicalDeviceImageFormatProperties2KHR");
    if (!func3) {
        throw std::runtime_error("Failed to load "
            "vkGetPhysicalDeviceImageFormatProperties2KHR");
    }

    result = func3(phyDevice, &imageFormatInfo, &imageFormatProperties);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to query image format properties");
    }

    VkExternalMemoryPropertiesKHR memProps =
        extImageFormatProperties.externalMemoryProperties;
    if (!(memProps.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(memProps.compatibleHandleTypes & EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE)) {
        throw std::runtime_error("The image cannot be exported");
    }
}

Vkdev::~Vkdev()
{
    vkDestroyDevice(m_device, nullptr);
}


/*
 * Definitions for the methods from class Vkcmdpool.
 */
Vkcmdpool::Vkcmdpool(
    const Vkdev *device
):
m_device(device->get())
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device->getTransferQueueFamilyIndex();

    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a command pool");
    }
}

Vkcmdpool::~Vkcmdpool()
{
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}


/*
 * Definitions for the methods from class Vkbuf.
 */
Vkbuf::Vkbuf(
    const Vkdev *device, VkDeviceSize bufferSize,
    VkBufferUsageFlags bufferUsage, bool exportCapable
):
m_device(device->get())
{
    VkResult result = VK_SUCCESS;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = bufferUsage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkExternalMemoryBufferCreateInfoKHR extBufferInfo = {};

    if (exportCapable) {
        /*
         * Indicate that the memory backing this buffer will be exported in an
         * fd. In some implementations, this may affect the call to
         * GetBufferMemoryRequirements() with this buffer.
         */
        extBufferInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR;
        extBufferInfo.handleTypes |= EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;

        bufferInfo.pNext = &extBufferInfo;
    }

    result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a buffer");
    }

    VkMemoryRequirements memReq = {};
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReq);

    m_size = memReq.size;
    m_alignment = memReq.alignment;
    m_memoryTypeBits = memReq.memoryTypeBits;
}

void Vkbuf::bind(const Vkdevicemem *deviceMem, VkDeviceSize offset)
{
    vkBindBufferMemory(m_device, m_buffer, deviceMem->getMemory(), offset);
}

Vkbuf::~Vkbuf()
{
    vkDestroyBuffer(m_device, m_buffer, nullptr);
}


/*
 * Definitions for the methods from class Vkimg2d.
 */
Vkimg2d::Vkimg2d(
    const Vkdev *device, VkExtent2D extent, VkImageUsageFlags imageUsage,
    bool exportCapable
):
m_device(device->get()),m_extent(extent)
{
    VkResult result = VK_SUCCESS;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8_UINT;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = imageUsage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkExternalMemoryImageCreateInfoKHR extImageCreateInfo = {};

    if (exportCapable) {
        /*
         * Indicate that the memory backing this image will be exported in an
         * fd. In some implementations, this may affect the call to
         * GetImageMemoryRequirements() with this image.
         */
        extImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;
        extImageCreateInfo.handleTypes |= EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;

        imageInfo.pNext = &extImageCreateInfo;
    }

    result = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create an image");
    }

    VkMemoryRequirements memReq = {};
    vkGetImageMemoryRequirements(m_device, m_image, &memReq);

    m_size = memReq.size;
    m_memoryTypeBits = memReq.memoryTypeBits;
    m_alignment = memReq.alignment;
}

void Vkimg2d::bind(const Vkdevicemem *deviceMem, VkDeviceSize offset)
{
    vkBindImageMemory(m_device, m_image, deviceMem->getMemory(), offset);
}

Vkimg2d::~Vkimg2d()
{
    vkDestroyImage(m_device, m_image, nullptr);
}


/*
 * Definitions for the methods from class Vkdevicemem.
 */
Vkdevicemem::Vkdevicemem(
    const Vkdev *device, VkDeviceSize size, uint32_t memoryTypeBits,
    VkMemoryPropertyFlags memoryProperties, bool exportCapable
):
m_device(device->get()),m_size(size)
{
    VkResult result = VK_SUCCESS;

    VkMemoryAllocateInfo memInfo = {};
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = m_size;
    memInfo.memoryTypeIndex = findMemoryType(*device, memoryTypeBits, memoryProperties);

    VkExportMemoryAllocateInfoKHR exportInfo = {};

    if (exportCapable) {
        /*
         * Indicate that the memory to be allocated now will be exported in an
         * fd.
         */
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
        exportInfo.handleTypes = EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;

        memInfo.pNext = &exportInfo;
    }

    result = vkAllocateMemory(m_device, &memInfo, nullptr, &m_deviceMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate backing memory for "
            "the buffer");
    }
}

Vkdevicemem::~Vkdevicemem()
{
    vkFreeMemory(m_device, m_deviceMemory, nullptr);
}

VkResult Vkdevicemem::map(void **p, VkDeviceSize size, VkDeviceSize offset)
{
    return vkMapMemory(m_device, m_deviceMemory, offset, size, 0, p);
}

void Vkdevicemem::unmap(void)
{
    vkUnmapMemory(m_device, m_deviceMemory);
}

#ifndef _WIN32
void *Vkdevicemem::getExportHandle(void) const
{
    int fd = -1;

    VkMemoryGetFdInfoKHR fdInfo = {};
    fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fdInfo.memory = m_deviceMemory;
    fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    auto func = (PFN_vkGetMemoryFdKHR) \
        vkGetDeviceProcAddr(m_device, "vkGetMemoryFdKHR");

    if (!func ||
        func(m_device, &fdInfo, &fd) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)(uintptr_t)fd;
}
#else
void *Vkdevicemem::getExportHandle(void) const
{
    HANDLE handle;

    VkMemoryGetWin32HandleInfoKHR handleInfo = {};
    handleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.memory = m_deviceMemory;
    handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

    auto func = (PFN_vkGetMemoryWin32HandleKHR) \
        vkGetDeviceProcAddr(m_device, "vkGetMemoryWin32HandleKHR");

    if (!func ||
        func(m_device, &handleInfo, &handle) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)handle;
}
#endif

/*
 * Definitions for the methods from class Vkcmdbuffer.
 */
Vkcmdbuffer::Vkcmdbuffer(const Vkdev *device, const Vkcmdpool *commandPool)
:m_device(device->get()),m_commandPool(commandPool->get())
{
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = m_commandPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &cmdBufInfo, &m_commandBuffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
}

Vkcmdbuffer::~Vkcmdbuffer()
{
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
}

VkResult Vkcmdbuffer::begin(void)
{
    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    return vkBeginCommandBuffer(m_commandBuffer, &cmdBeginInfo);
}

VkResult Vkcmdbuffer::end(void)
{
    return vkEndCommandBuffer(m_commandBuffer);
}

void Vkcmdbuffer::fillBuffer(
    const Vkbuf *buffer, uint32_t data, VkDeviceSize size, VkDeviceSize offset
)
{
    vkCmdFillBuffer(m_commandBuffer, buffer->get(), offset, size, data);
}

void Vkcmdbuffer::copyBuffer(
    const Vkbuf *dstBuffer, const Vkbuf *srcBuffer, VkDeviceSize size
)
{
    VkBufferCopy copy = {};
    copy.size = size;

    vkCmdCopyBuffer(m_commandBuffer, srcBuffer->get(), dstBuffer->get(), 1, &copy);
}

void Vkcmdbuffer::pipelineBarrier(
    const Vkimgmembarrier *imageBarrier,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
)
{
    VkImageMemoryBarrier barrier = imageBarrier->get();

    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(m_commandBuffer, srcStageMask, dstStageMask, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}

void Vkcmdbuffer::clearImage(const Vkimg2d *image, VkClearColorValue color)
{
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    vkCmdClearColorImage(m_commandBuffer, image->get(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
}

void Vkcmdbuffer::copyImageToBuffer(const Vkbuf *buffer, const Vkimg2d *image)
{
    VkExtent2D extent = image->getExtent();

    VkBufferImageCopy copy = {};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = { 0, 0, 0 };
    copy.imageExtent.width = extent.width;
    copy.imageExtent.height = extent.height;
    copy.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(m_commandBuffer, image->get(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->get(), 1, &copy);
}

void Vkcmdbuffer::copyBufferToImage(const Vkimg2d *image, const Vkbuf *buffer)
{
    VkExtent2D extent = image->getExtent();

    VkBufferImageCopy copy = {};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = { 0, 0, 0 };
    copy.imageExtent.width = extent.width;
    copy.imageExtent.height = extent.height;
    copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(m_commandBuffer, buffer->get(), image->get(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}


/*
 * Definitions for the methods from class Vkque.
 */
VkResult Vkque::submit(
    const std::vector<VkSemaphore>& waitSemaphores,
    const std::vector<VkCommandBuffer>& commandBuffers,
    const std::vector<VkSemaphore>& signalSemaphores
)
{
    std::vector<VkPipelineStageFlags> stageFlags(waitSemaphores.size());

    stageFlags.assign(stageFlags.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = stageFlags.data();
    submitInfo.commandBufferCount = (uint32_t)commandBuffers.size();
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    return vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkResult Vkque::submit(const Vkcmdbuffer *commandBuffer)
{
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkSemaphore> signalSemaphores;

    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.push_back(commandBuffer->get());

    return submit(waitSemaphores, commandBuffers, signalSemaphores);
}

VkResult Vkque::submit(
    const Vkcmdbuffer *commandBuffer,
    const Vksema *signalSemaphore
)
{
    std::vector<VkSemaphore> waitSemaphores;

    std::vector<VkSemaphore> signalSemaphores;
    signalSemaphores.push_back(signalSemaphore->get());

    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.push_back(commandBuffer->get());

    return submit(waitSemaphores, commandBuffers, signalSemaphores);
}

VkResult Vkque::waitIdle(void)
{
    return vkQueueWaitIdle(m_queue);
}


/*
 * Definitions for the methods from class Vksema.
 */
Vksema::Vksema(const Vkdev *device, bool exportCapable)
:m_device(device->get())
{
    VkSemaphoreCreateInfo semaInfo = {};
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkExportSemaphoreCreateInfoKHR exportSemaInfo = {};

    if (exportCapable) {
        exportSemaInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

        exportSemaInfo.handleTypes |= EXTERNAL_MEMORY_HANDLE_SUPPORTED_TYPE;
        semaInfo.pNext = &exportSemaInfo;
    }

    if (vkCreateSemaphore(m_device, &semaInfo, nullptr, &m_semaphore) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate a semaphore");
    }
}

Vksema::~Vksema()
{
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
}

#ifndef _WIN32
void *Vksema::getExportHandle(void) const
{
    int fd = -1;

    VkSemaphoreGetFdInfoKHR semaFdInfo = {};
    semaFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    semaFdInfo.semaphore = m_semaphore;
    semaFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

    auto func = (PFN_vkGetSemaphoreFdKHR) \
        vkGetDeviceProcAddr(m_device, "vkGetSemaphoreFdKHR");
    if (!func ||
        func(m_device, &semaFdInfo, &fd) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)(uintptr_t)fd;
}

#else

void *Vksema::getExportHandle(void) const
{
    HANDLE handle;

    VkSemaphoreGetWin32HandleInfoKHR semaWin32HandleInfo = {};
    semaWin32HandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    semaWin32HandleInfo.semaphore = m_semaphore;
    semaWin32HandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

    auto func = (PFN_vkGetSemaphoreWin32HandleKHR) \
        vkGetDeviceProcAddr(m_device, "vkGetSemaphoreWin32HandleKHR");
    if (!func ||
        func(m_device, &semaWin32HandleInfo, &handle) != VK_SUCCESS) {
        return nullptr;
    }

    return (void *)(handle);
}

#endif


/*
 * Definitions for the methods from class Vkimgmembarrier.
 */
Vkimgmembarrier::Vkimgmembarrier(const Vkimg2d *image)
{
    m_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    m_barrier.pNext = nullptr;
    m_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    m_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    m_barrier.image = image->get();
    m_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    m_barrier.subresourceRange.baseMipLevel = 0;
    m_barrier.subresourceRange.levelCount = 1;
    m_barrier.subresourceRange.baseArrayLayer = 0;
    m_barrier.subresourceRange.layerCount = 1;
}


/*
 * Definitions for the methods from class Cudactx.
 */
Cudactx::Cudactx(const Vkdev *device)
{
    CUdevice dev;
    CUresult result = CUDA_SUCCESS;
    bool foundDevice = true;

    result = cuInit(0);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to cuInit()");
    }

    int numDevices = 0;
    result = cuDeviceGetCount(&numDevices);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to get count of CUDA devices");
    }

    CUuuid id = {};
    const std::array<uint8_t, VK_UUID_SIZE> deviceUUID = device->getUUID();

    /*
     * Loop over the available devices and identify the CUdevice
     * corresponding to the physical device in use by this Vulkan instance.
     * This is required because there is no other way to match GPUs across
     * API boundaries.
     */
    for (int i = 0; i < numDevices; i++) {
        cuDeviceGet(&dev, i);

        cuDeviceGetUuid(&id, dev);

        if (!std::memcmp(static_cast<const void *>(&id),
                static_cast<const void *>(deviceUUID.data()),
                sizeof(CUuuid))) {
            foundDevice = true;
            break;
        }
    }

    if (!foundDevice) {
        throw std::runtime_error("Failed to get an appropriate CUDA device");
    }

    result = cuCtxCreate(&m_context, 0, dev);
    if (result != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to create a CUDA context");
    }
}

CUresult Cudactx::memcpyDtoH(void *p, CUdeviceptr dptr, size_t size)
{
    return cuMemcpyDtoH(p, dptr, size);
}

CUresult Cudactx::memcpy2D(
    void *p, CUarray array, uint32_t width, uint32_t height
)
{
    CUDA_MEMCPY2D copy = {};
    copy.srcMemoryType = CU_MEMORYTYPE_ARRAY;
    copy.srcArray = array;
    copy.dstMemoryType = CU_MEMORYTYPE_HOST;
    copy.dstHost = p;
    copy.dstPitch = width;
    copy.WidthInBytes = width;
    copy.Height = height;

    return cuMemcpy2D(&copy);
}


/*
 * Definitions for the methods from class Cudabuffer.
 */
Cudabuffer::Cudabuffer(const Vkdevicemem *deviceMem)
{
    int fd = -1;
    void *p = nullptr;

    if ((p = deviceMem->getExportHandle()) == nullptr) {
        throw std::runtime_error("Failed to get export handle for memory");
    }

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC memDesc = {};
    memDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    memDesc.handle.fd = (int)(uintptr_t)p;
    memDesc.size = deviceMem->getSize();

    if (cuImportExternalMemory(&m_extMem, &memDesc) != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to import buffer into CUDA");
    }

    CUDA_EXTERNAL_MEMORY_BUFFER_DESC bufDesc = {};
    bufDesc.size = memDesc.size;

    if (cuExternalMemoryGetMappedBuffer(&m_deviceptr, m_extMem, &bufDesc) !=
        CUDA_SUCCESS) {
        throw std::runtime_error("Failed to get CUdeviceptr");
    }
}

Cudabuffer::~Cudabuffer()
{
    cuMemFree(m_deviceptr);
    cuDestroyExternalMemory(m_extMem);
    m_deviceptr = 0;
}


/*
 * Definitions for the methods from class Cudaimage.
 */
Cudaimage::Cudaimage(const Vkimg2d *image, const Vkdevicemem *deviceMem)
{
    int fd = -1;
    void *p = nullptr;
    CUresult result = CUDA_SUCCESS;

    if ((p = deviceMem->getExportHandle()) == nullptr) {
        throw std::runtime_error("Failed to get export handle for memory");
    }

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC memDesc = {};
#ifndef _WIN32
    memDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
#else
    memDesc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32;
#endif
    memDesc.handle.fd = (int)(uintptr_t)p;
    memDesc.size = deviceMem->getSize();

    if (cuImportExternalMemory(&m_extMem, &memDesc) != CUDA_SUCCESS) {
        throw std::runtime_error("Failed to import buffer into CUDA");
    }

    VkExtent2D extent = image->getExtent();

    CUDA_ARRAY3D_DESCRIPTOR arrayDesc = {};
    arrayDesc.Width = extent.width;
    arrayDesc.Height = extent.height;
    arrayDesc.Depth = 0; /* CUDA 2D arrays are defined to have depth 0 */
    arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
    arrayDesc.NumChannels = 1;
    arrayDesc.Flags = CUDA_ARRAY3D_SURFACE_LDST |
                      CUDA_ARRAY3D_COLOR_ATTACHMENT;

    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC mipmapArrayDesc = {};
    mipmapArrayDesc.arrayDesc = arrayDesc;
    mipmapArrayDesc.numLevels = 1;

    result = cuExternalMemoryGetMappedMipmappedArray(&m_mipmapArray, m_extMem,
                 &mipmapArrayDesc);
    if (result != CUDA_SUCCESS) {
        std::ostringstream oss;
        oss << "Failed to get CUmipmappedArray; " << result;
        throw std::runtime_error(oss.str());
    }

    result = cuMipmappedArrayGetLevel(&m_array, m_mipmapArray, 0);
    if (result != CUDA_SUCCESS) {
        std::ostringstream oss;
        oss << "Failed to get CUarray; " << result;
        throw std::runtime_error(oss.str());
    }
}

Cudaimage::~Cudaimage()
{
    cuMipmappedArrayDestroy(m_mipmapArray);
    cuDestroyExternalMemory(m_extMem);
    m_array = 0;
    m_mipmapArray = 0;
}


/*
 * Definitions for the methods from class Cudasema.
 */
Cudasema::Cudasema(const Vksema *semaphore)
{
    int fd = -1;
    void *p = nullptr;

    if ((p = semaphore->getExportHandle()) == nullptr) {
        throw std::runtime_error("Failed to get export handle for semaphore");
    }

    CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC semDesc = {};
#ifndef _WIN32
    semDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD;
#else
    semDesc.type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32;
#endif
    semDesc.handle.fd = (int)(uintptr_t)p;

    if (cuImportExternalSemaphore(&m_extSema, &semDesc) !=
        CUDA_SUCCESS) {
        throw std::runtime_error("Failed to import semaphore into CUDA");
    }
}

Cudasema::~Cudasema()
{
    cuDestroyExternalSemaphore(m_extSema);
}

CUresult Cudasema::wait(void)
{
    CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS waitParams = {};

    return cuWaitExternalSemaphoresAsync(&m_extSema, &waitParams, 1, nullptr);
}

CUresult Cudasema::signal(void)
{
    CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS signalParams = {};

    return cuSignalExternalSemaphoresAsync(&m_extSema, &signalParams, 1, nullptr);
}
