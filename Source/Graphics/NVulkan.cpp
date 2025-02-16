#include "NVulkan.h"

#include "../DataStructures/Arena.h"
#include "../Platform/Platform.h"

#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "../ThirdParty/stb_image.h"

struct VulkanState {
    Arena arena;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice pdevice;
    VkDevice ldevice;
    VkQueue graphics_queue;
    u32 graphics_queue_index;
    VmaAllocator allocator;
};

global VulkanState vulkan_state = {};

#if OS_WINDOWS
VkSurfaceKHR GetSurfaceForWindow(Window *win) {
    VkSurfaceKHR result = 0;

    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = GetModuleHandle(0);
    surface_info.hwnd = win->handle;

    VK_CHECK(vkCreateWin32SurfaceKHR(vulkan_state.instance, &surface_info, 0, &result));

    return result;
}
#endif

internal void InitPhysicalDevice(const char **device_extensions, u32 device_extensions_count) {
    VkInstance instance = vulkan_state.instance;

    u32 dcount;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dcount, 0));

    VkPhysicalDevice *devices = (VkPhysicalDevice *) HeapAlloc(dcount * sizeof(VkPhysicalDevice));
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dcount, devices));

    for (u32 i = 0; i < dcount; ++i) {
        VkPhysicalDevice pdevice = devices[i];

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(pdevice, &properties);

        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }

        u32 dextcnt;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(pdevice, 0, &dextcnt, 0));

        VkExtensionProperties *dextprops = (VkExtensionProperties *) HeapAlloc(dextcnt * sizeof(VkExtensionProperties));
        VK_CHECK(vkEnumerateDeviceExtensionProperties(pdevice, 0, &dextcnt, dextprops));

        b8 compatible = 1;
        for (u32 j = 0; j < device_extensions_count; ++j) {
            const char *device_extension = device_extensions[j];
            b8 found = 0;

            for (u32 k = 0; k < dextcnt; ++k) {
                VkExtensionProperties props = dextprops[k];
                if (strcmp(props.extensionName, device_extension) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                compatible = 0;
                break;
            }
        }

        HeapFree(dextprops);

        if (!compatible) {
            continue;
        }

        vulkan_state.pdevice = pdevice;
        break;
    }

    if (!vulkan_state.pdevice) {
        Print("Failed to find compatible device.");
        Exit(1);
    }

    HeapFree(devices);
}

internal void InitLogicalDevice(const char **device_extensions, u32 device_extensions_count) {
    VkPhysicalDevice pdev = vulkan_state.pdevice;

    VkPhysicalDeviceMemoryProperties memprops;
    vkGetPhysicalDeviceMemoryProperties(pdev, &memprops);

    u32 quefmlycnt;
    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &quefmlycnt, 0);

    VkQueueFamilyProperties *quefmlyprops = (VkQueueFamilyProperties *) HeapAlloc(quefmlycnt * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &quefmlycnt, quefmlyprops);

    VkDeviceQueueCreateInfo qinfos[1];
    float priority = 1.0f;
    u32 gfxidx = ~0u;
    for (u32 i = 0; i < quefmlycnt; ++i) {
        VkQueueFamilyProperties props = quefmlyprops[i];

        if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            VkBool32 present_supports = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, vulkan_state.surface, &present_supports);

            if (!present_supports) {
                continue;
            }

            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = i;
            info.queueCount = 1;
            info.pQueuePriorities = &priority;

            qinfos[0] = info;

            gfxidx = i;
            break;
        }
    }

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.features.multiDrawIndirect = true;
    features.features.pipelineStatisticsQuery = true;

    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.runtimeDescriptorArray = true;

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    features.pNext = &features11;
    features11.pNext = &features12;
    features12.pNext = &features13;

    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = ArrayCount(qinfos);
    info.pQueueCreateInfos = qinfos;
    info.enabledExtensionCount = device_extensions_count;
    info.ppEnabledExtensionNames = device_extensions;
    info.pNext = &features;

    VK_CHECK(vkCreateDevice(pdev, &info, 0, &vulkan_state.ldevice));

    vkGetDeviceQueue(vulkan_state.ldevice, gfxidx, 0, &vulkan_state.graphics_queue);

    vulkan_state.graphics_queue_index = gfxidx;

    HeapFree(quefmlyprops);
}

void InitVulkan(Window *win) {
	VK_CHECK(volkInitialize());

    vulkan_state.arena = CreateArena(GigaBytes(1));

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Application";
    app_info.pEngineName = "Engine";
    app_info.apiVersion = VK_API_VERSION_1_3;

    const char *extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    const char *layers[] = { "VK_LAYER_KHRONOS_validation" };

    const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // @Todo: check if extensions and layers are available

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = ArrayCount(extensions);
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledLayerCount = ArrayCount(layers);
    instance_info.ppEnabledLayerNames = layers;

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instance_info, 0, &instance));

    volkLoadInstance(instance);

    vulkan_state.instance = instance;
    vulkan_state.surface = GetSurfaceForWindow(win);

    InitPhysicalDevice(device_extensions, ArrayCount(device_extensions));
    InitLogicalDevice(device_extensions, ArrayCount(device_extensions));
    volkLoadDevice(vulkan_state.ldevice);

    VmaVulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
    vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
    vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
    vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
    vma_vulkan_func.vkCreateImage = vkCreateImage;
    vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
    vma_vulkan_func.vkDestroyImage = vkDestroyImage;
    vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_vulkan_func.vkFreeMemory = vkFreeMemory;
    vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_vulkan_func.vkMapMemory = vkMapMemory;
    vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
    vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = vulkan_state.pdevice;
    allocator_info.device = vulkan_state.ldevice;
    allocator_info.instance = vulkan_state.instance;
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.pVulkanFunctions = &vma_vulkan_func;

    VK_CHECK(vmaCreateAllocator(&allocator_info, &vulkan_state.allocator));
}

void ReleaseVulkan() {
    vmaDestroyAllocator(vulkan_state.allocator);
    vkDestroyDevice(vulkan_state.ldevice, 0);
    vkDestroySurfaceKHR(vulkan_state.instance, vulkan_state.surface, 0);
    vkDestroyInstance(vulkan_state.instance, 0);
    FreeArena(&vulkan_state.arena);
}

VkCommandPool CreateCommandPool() {
    VkCommandPool result = 0;

    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK(vkCreateCommandPool(vulkan_state.ldevice, &info, 0, &result));

    return result;
}

void DestroyCommandPool(VkCommandPool cmdpool) {
    vkDestroyCommandPool(vulkan_state.ldevice, cmdpool, 0);
}


void AllocateCommandBuffers(VkCommandPool cmdpool, VkCommandBuffer *buffers, u32 buffers_count) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = cmdpool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = buffers_count;

    VK_CHECK(vkAllocateCommandBuffers(vulkan_state.ldevice, &info, buffers));
}

void FreeCommandBuffers(VkCommandPool cmdpool, VkCommandBuffer *buffers, u32 buffers_count) {
    vkFreeCommandBuffers(vulkan_state.ldevice, cmdpool, buffers_count, buffers);
}

void BeginCommandBuffer(VkCommandBuffer cmdbuf) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(cmdbuf, &info));
}

void EndCommandBuffer(VkCommandBuffer cmdbuf) {
    VK_CHECK(vkEndCommandBuffer(cmdbuf));
}

void SubmitCommandBuffer(VkCommandBuffer cmdbuf) {
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmdbuf;

    vkQueueSubmit(vulkan_state.graphics_queue, 1, &info, 0);
    vkQueueWaitIdle(vulkan_state.graphics_queue);
}

VkCommandBuffer BeginTempCommandBuffer(VkCommandPool cmdpool) {
    VkCommandBuffer cmdbuf;
    AllocateCommandBuffers(cmdpool, &cmdbuf, 1);
    BeginCommandBuffer(cmdbuf);

    return cmdbuf;
}

void EndTempCommandBuffer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf) {
    EndCommandBuffer(cmdbuf);
    SubmitCommandBuffer(cmdbuf);
    FreeCommandBuffers(cmdpool, &cmdbuf, 1);
}

void CreateSwapchain(Swapchain *swapchain, VkCommandPool cmdpool) {
    VkPhysicalDevice pdev = vulkan_state.pdevice;
    VkDevice ldev = vulkan_state.ldevice;
    VkSurfaceKHR surface = vulkan_state.surface;

    Swapchain *sc = swapchain;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &caps);
    if (caps.maxImageCount < SWAPCHAIN_IMAGE_COUNT) {
        Print("Surface doesn't support enough images.\n");
        Exit(1);
    }

    u32 formats_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &formats_count, 0);

    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *) HeapAlloc(formats_count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &formats_count, formats);

    for (u32 i = 0; i < formats_count; ++i) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            sc->format = format;
            break;
        }
    }

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VK_CHECK(vkCreateSemaphore(ldev, &semaphore_info, 0, &sc->acquire_semaphore));
    VK_CHECK(vkCreateSemaphore(ldev, &semaphore_info, 0, &sc->release_semaphore));

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VK_CHECK(vkCreateFence(ldev, &fence_info, 0, &sc->frame_fence));

    UpdateSwapchain(sc, cmdpool, 1);

    sc->query_pool = CreateQueryPool(2, VK_QUERY_TYPE_TIMESTAMP);

    HeapFree(formats);
}

void DestroySwapchain(Swapchain *swapchain) {
    Swapchain *sc = swapchain;
    VkDevice ldev = vulkan_state.ldevice;

    DestroyQueryPool(sc->query_pool);

    vkDestroyFence(ldev, sc->frame_fence, 0);
    vkDestroySemaphore(ldev, sc->acquire_semaphore, 0);
    vkDestroySemaphore(ldev, sc->release_semaphore, 0);

    vkDestroySwapchainKHR(ldev, sc->handle, 0);
}

void UpdateSwapchain(Swapchain *swapchain, VkCommandPool cmdpool, b8 vsync) {
    VkPhysicalDevice pdev = vulkan_state.pdevice;
    VkDevice ldev = vulkan_state.ldevice;
    VkSurfaceKHR surface = vulkan_state.surface;
    Swapchain *sc = swapchain;

    VkSwapchainKHR old_swapchain = sc->handle;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &caps);

    Assert(caps.currentExtent.width != u32(-1));
    VkExtent2D extent = caps.currentExtent;

    u32 present_modes_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &present_modes_count, 0);

    VkPresentModeKHR *present_modes = (VkPresentModeKHR *) HeapAlloc(present_modes_count * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &present_modes_count, present_modes);

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync) {
        for (u32 i = 0; i < present_modes_count; ++i) {
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }
    HeapFree(present_modes);

    VkSurfaceTransformFlagBitsKHR transform;
    if (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        transform = caps.currentTransform;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = vulkan_state.surface;
    create_info.minImageCount = SWAPCHAIN_IMAGE_COUNT;
    create_info.imageFormat = sc->format.format;
    create_info.imageColorSpace = sc->format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &vulkan_state.graphics_queue_index;
    create_info.preTransform = transform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = old_swapchain;

    VK_CHECK(vkCreateSwapchainKHR(ldev, &create_info, 0, &sc->handle));

    sc->width = extent.width;
    sc->height = extent.height;

    u32 image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(ldev, sc->handle, &image_count, 0));

    Assert(image_count = SWAPCHAIN_IMAGE_COUNT);

    VK_CHECK(vkGetSwapchainImagesKHR(ldev, sc->handle, &image_count, sc->images));

    if (old_swapchain) {
        VK_CHECK(vkDeviceWaitIdle(ldev));
        vkDestroySwapchainKHR(ldev, old_swapchain, 0);
    }
}

b32 AcquireSwapchain(Swapchain *swapchain, VkCommandPool cmdpool, VkCommandBuffer cmdbuf, Image color_target, Image depth_target) {
    Swapchain *sc = swapchain;
    VkDevice ldev = vulkan_state.ldevice;

    VkResult result = vkAcquireNextImageKHR(ldev, sc->handle, 1000000, sc->acquire_semaphore, 0, &sc->current_image);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return 0;
    }

    VK_CHECK_SWAPCHAIN(result);

    VK_CHECK(vkResetCommandPool(ldev, cmdpool, 0));

    BeginCommandBuffer(cmdbuf);

    vkCmdResetQueryPool(cmdbuf, sc->query_pool, 0, 2);
    vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, sc->query_pool, 0);

    VkImageMemoryBarrier2 barriers[] = {
        CreateImageBarrier(color_target.handle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
        CreateImageBarrier(depth_target.handle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT),
    };

    PipelineImageBarriers(cmdbuf, VK_DEPENDENCY_BY_REGION_BIT, barriers, ArrayCount(barriers));

    return 1;
}

void PresentSwapchain(Swapchain *swapchain, VkCommandBuffer cmdbuf, Image color_target) {
    Swapchain *sc = swapchain;
    VkDevice ldev = vulkan_state.ldevice;

    VkImage current_image = sc->images[sc->current_image];

    VkImageMemoryBarrier2 copy_barriers[] = {
        CreateImageBarrier(color_target.handle, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
        CreateImageBarrier(current_image,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT)
    };

    PipelineImageBarriers(cmdbuf, VK_DEPENDENCY_BY_REGION_BIT, copy_barriers, ArrayCount(copy_barriers));

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent.width = sc->width;
    copy_region.extent.height = sc->height;
    copy_region.extent.depth = 1;

    vkCmdCopyImage(cmdbuf, color_target.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        current_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    VkImageMemoryBarrier2 present_barrier = CreateImageBarrier(current_image, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);

    PipelineImageBarriers(cmdbuf, VK_DEPENDENCY_BY_REGION_BIT, &present_barrier, 1);

    vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, sc->query_pool, 1);

    EndCommandBuffer(cmdbuf);

    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &sc->acquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdbuf;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &sc->release_semaphore;

    VK_CHECK(vkQueueSubmit(vulkan_state.graphics_queue, 1, &submit_info, sc->frame_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &sc->release_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &sc->handle;
    present_info.pImageIndices = &sc->current_image;

    VK_CHECK_SWAPCHAIN(vkQueuePresentKHR(vulkan_state.graphics_queue, &present_info));

    VK_CHECK(vkWaitForFences(ldev, 1, &sc->frame_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(ldev, 1, &sc->frame_fence));
}

Image CreateImage(u32 width, u32 height, VkFormat format, u32 mip_levels, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage) {
    Image result = {};

    result.format = format;

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = mip_levels;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(vulkan_state.allocator, &info, &alloc_info, &result.handle, &result.allocation, 0));

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = result.handle;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(vulkan_state.ldevice, &view_info, 0, &result.view));

    return result;
}

Image CreateDepthImage(Swapchain *swapchain, VkCommandPool cmdpool) {
    Swapchain *sc = swapchain;

    VkImageAspectFlags depth_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    Image depth_image = CreateImage(sc->width, sc->height, VK_FORMAT_D24_UNORM_S8_UINT, 1, depth_aspect,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    VkImageSubresourceRange range = {};
    range.aspectMask = depth_aspect;
    range.levelCount = 1;
    range.layerCount = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.image = depth_image.handle;
    barrier.subresourceRange = range;

    VkCommandBuffer cmdbuf = BeginTempCommandBuffer(cmdpool);
    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, 0, 0, 0, 1, &barrier);
    EndTempCommandBuffer(cmdpool, cmdbuf);

    return depth_image;
}

void DestroyImage(Image image) {
    vkDestroyImageView(vulkan_state.ldevice, image.view, 0);
    vmaDestroyImage(vulkan_state.allocator, image.handle, image.allocation);
}

Texture CreateTexture(u32 width, u32 height, VkFormat format, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage, VkSamplerCreateInfo sampler_info) {
    Texture result = {};

    result.image = CreateImage(width, height, format, 1, aspect_mask, usage);

    vkCreateSampler(vulkan_state.ldevice, &sampler_info, 0, &result.descriptor.sampler);

    result.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    result.descriptor.imageView = result.image.view;
    return result;
}

Texture CreateTexture(u32 width, u32 height, VkFormat format, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage) {
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    return CreateTexture(width, height, format, aspect_mask, usage, sampler_info);
}

internal u32 GetMipLevels(u32 width, u32 height) {
    return u32(floorf(log2f(float(Max(width, height))))) + 1;
}

Texture CreateTextureFromPixels(u32 width, u32 height, u32 channels, VkFormat format, u8 *pixels,
    VkSamplerCreateInfo sampler_info, VkCommandPool cmdpool) {
    Texture result = {};

    //Image img = CreateImage(state, width, height, format, GetMipLevels(width, height), VK_IMAGE_ASPECT_COLOR_BIT,
    Image img = CreateImage(width, height, format, 1, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkDeviceSize size = width * height * channels;
    StagingBuffer sbuf = CreateStagingBuffer(size, pixels);

    VkCommandBuffer cmdbuf = BeginTempCommandBuffer(cmdpool);

    VkImageMemoryBarrier2 before_barrier = CreateImageBarrier(img.handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    PipelineImageBarriers(cmdbuf, VK_DEPENDENCY_BY_REGION_BIT, &before_barrier, 1);

    VkBufferImageCopy2 region = {};
    region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    VkCopyBufferToImageInfo2 copy_info = {};
    copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
    copy_info.srcBuffer = sbuf.handle;
    copy_info.dstImage = img.handle;
    copy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_info.regionCount = 1;
    copy_info.pRegions = &region;

    vkCmdCopyBufferToImage2(cmdbuf, &copy_info);

    VkImageMemoryBarrier2 after_barrier = CreateImageBarrier(img.handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    PipelineImageBarriers(cmdbuf, VK_DEPENDENCY_BY_REGION_BIT, &after_barrier, 1);

    EndTempCommandBuffer(cmdpool, cmdbuf);

    DestroyStagingBuffer(sbuf);

    result.image = img;

    vkCreateSampler(vulkan_state.ldevice, &sampler_info, 0, &result.descriptor.sampler);
    result.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    result.descriptor.imageView = result.image.view;

    return result;
}

Texture LoadTextureFromFile(const char *path, VkCommandPool cmdpool) {
    Texture result = {};

	int w, h, channels;
	u8 *pixels = stbi_load(path, &w, &h, &channels, STBI_rgb);
	if (!pixels) {
		Print("Failed to load texture '%s'!\n", path);
		Exit(1);
	}

	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.maxLod = 1.0f;

	if (channels == 3) {
		u8 *rgba_pixels = (u8 *)HeapAlloc(w * h * 4);
		for (u32 i = 0; i < w * h; ++i) {
			u32 pixel_idx = i * 3;
			u32 rgba_idx = i * 4;

			rgba_pixels[rgba_idx] = pixels[pixel_idx];
			rgba_pixels[rgba_idx + 1] = pixels[pixel_idx + 1];
			rgba_pixels[rgba_idx + 2] = pixels[pixel_idx + 2];
			rgba_pixels[rgba_idx + 3] = 255;
		}

		result = CreateTextureFromPixels(w, h, 4, VK_FORMAT_R8G8B8A8_SRGB, rgba_pixels, sampler_info, cmdpool);

		HeapFree(rgba_pixels);
	} else {
		result = CreateTextureFromPixels(w, h, channels, VK_FORMAT_R8G8B8A8_SRGB, pixels, sampler_info, cmdpool);
	}
	
	free(pixels);

    return result;
}

void DestroyTexture(Texture tex) {
    vkDestroySampler(vulkan_state.ldevice, tex.descriptor.sampler, 0);
    DestroyImage(tex.image);
}

TextureArray CreateTextureArray(uint count) {
    TextureArray result = {};

    result.images = PushArray(&vulkan_state.arena, Image, count);
    result.descriptors = PushArray(&vulkan_state.arena, VkDescriptorImageInfo, count);

    result.capacity = count;
    result.count = 0;
    
    return result;
}

void AddTexture(TextureArray *ta, Texture tex) {
    Assert(ta->count < ta->capacity);

    ta->images[ta->count] = tex.image;
    ta->descriptors[ta->count] = tex.descriptor;
    ta->count++;
}

void LoadTextureAtSlot(TextureArray *ta, uint slot, const char *path, VkCommandPool cmdpool) {
    Texture tex = LoadTextureFromFile(path, cmdpool);

    ta->images[slot] = tex.image;
    ta->descriptors[slot] = tex.descriptor;
    ta->count++;
}

void DestroyTextureArray(TextureArray *ta) {
    for (int i = 0; i < ta->count; ++i) {
        vkDestroySampler(vulkan_state.ldevice, ta->descriptors[i].sampler, 0);
        DestroyImage(ta->images[i]);
    }
}

VkImageMemoryBarrier2 CreateImageBarrier(VkImage image, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkImageLayout old_layout,
    VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, VkImageLayout new_layout, VkImageAspectFlags aspect_mask) {
    VkImageMemoryBarrier2 result = {};

    VkImageSubresourceRange range = {};
    range.aspectMask = aspect_mask;
    range.levelCount = VK_REMAINING_MIP_LEVELS;
    range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    result.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    result.srcStageMask = src_stage;
    result.srcAccessMask = src_access;
    result.dstStageMask = dst_stage;
    result.dstAccessMask = dst_access;
    result.oldLayout = old_layout;
    result.newLayout = new_layout;
    result.image = image;
    result.subresourceRange = range;

    return result;
}

void PipelineImageBarriers(VkCommandBuffer cmdbuf, VkDependencyFlags flags, VkImageMemoryBarrier2 *barriers, u32 barriers_count) {
    VkDependencyInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.dependencyFlags = flags;
    info.imageMemoryBarrierCount = barriers_count;
    info.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmdbuf, &info);
}

VkBufferMemoryBarrier2 CreateBufferBarrier(VkBuffer buffer, VkDeviceSize size, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
    VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access) {
    VkBufferMemoryBarrier2 result = {};

    result.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    result.srcStageMask = src_stage;
    result.srcAccessMask = src_access;
    result.dstStageMask = dst_stage;
    result.dstAccessMask = dst_access;
    result.buffer = buffer;
    result.size = size;

    return result;
}

void PipelineBufferBarriers(VkCommandBuffer cmdbuf, VkDependencyFlags flags, VkBufferMemoryBarrier2 *barriers, u32 barriers_count) {
    VkDependencyInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.dependencyFlags = flags;
    info.bufferMemoryBarrierCount = barriers_count;
    info.pBufferMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmdbuf, &info);
}

void CreateDescriptorSet(Pipeline *p, VkDescriptorSetLayoutBinding *bindings, u32 bindings_count) {
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = bindings_count;
    layout_info.pBindings = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(vulkan_state.ldevice, &layout_info, 0, &p->desc_layout));

    VkDescriptorPoolSize *pool_sizes = (VkDescriptorPoolSize *) HeapAlloc(bindings_count * sizeof(VkDescriptorPoolSize));
    u32 pool_size_count = 0;
    for (u32 i = 0; i < bindings_count; ++i) {
        VkDescriptorSetLayoutBinding binding = bindings[i];
        if (binding.descriptorCount == 0) {
            continue;
        }

        b8 found = 0;
        for (u32 j = 0; j < pool_size_count; ++j) {
            if (pool_sizes[j].type == binding.descriptorType) {
                pool_sizes[j].descriptorCount += binding.descriptorCount;
                found = 1;
                continue;
            }
        }

        if (!found) {
            VkDescriptorPoolSize pool_size = {};
            pool_size.type = binding.descriptorType;
            pool_size.descriptorCount = binding.descriptorCount;

            Assert(pool_size_count < bindings_count);
            pool_sizes[pool_size_count++] = pool_size;
        }
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = pool_size_count;
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(vulkan_state.ldevice, &pool_info, 0, &p->desc_pool));

    VkDescriptorSetAllocateInfo desc_info = {};
    desc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_info.descriptorPool = p->desc_pool;
    desc_info.descriptorSetCount = 1;
    desc_info.pSetLayouts = &p->desc_layout;

    VK_CHECK(vkAllocateDescriptorSets(vulkan_state.ldevice, &desc_info, &p->desc_set));

    HeapFree(pool_sizes);
}

VkPipelineShaderStageCreateInfo LoadShader(Shader shader) {
	VkPipelineShaderStageCreateInfo result = {};

	String code = ReadFile(shader.path);
	if (!code.ptr) {
		Print("Failed to read shader '%s'!\n", shader.path);
		Exit(1);
	}

	VkShaderModuleCreateInfo module_info = {};
	module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = code.len;
	module_info.pCode = (u32 *) code.ptr;

	VkShaderModule module;
	VK_CHECK(vkCreateShaderModule(vulkan_state.ldevice, &module_info, 0, &module));

	result.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	result.stage = shader.stage;
	result.module = module;
	result.pName = "main";

    return result;
}

Pipeline CreatePipeline(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count, VkPipelineBindPoint bind_point) {
    Pipeline result = {};

    CreateDescriptorSet(&result, bindings, bindings_count);
    result.bind_point = bind_point;

    return result;
}

Pipeline CreateGraphicsPipeline(GraphicsPipelineOptions *options) {
    Pipeline result = CreatePipeline(options->bindings, options->bindings_count, VK_PIPELINE_BIND_POINT_GRAPHICS);

    VkPipelineShaderStageCreateInfo *shader_stages = (VkPipelineShaderStageCreateInfo *) HeapAlloc(options->shaders_count * sizeof(VkPipelineShaderStageCreateInfo));
    for (u32 i = 0; i < options->shaders_count; ++i) {
        shader_stages[i] = LoadShader(options->shaders[i]);
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {};
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.polygonMode = options->polygon_mode;
    rasterization_state.cullMode = options->cull_mode;
    rasterization_state.frontFace = options->front_face;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = options->depth_read;
    depth_stencil_state.depthWriteEnable = options->depth_write;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineViewportStateCreateInfo viewport_stage = {};
    viewport_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_stage.viewportCount = 1;
    viewport_stage.scissorCount = 1;

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_stage = {};
    dynamic_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_stage.dynamicStateCount = 2;
    dynamic_stage.pDynamicStates = dynamic_states;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_stage = {};
    color_blend_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_stage.attachmentCount = 1;
    color_blend_stage.pAttachments = &color_blend_attachment;

    VkPipelineVertexInputStateCreateInfo vertex_input_stage = {};
    vertex_input_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_stage.vertexBindingDescriptionCount = 0;
    vertex_input_stage.pVertexBindingDescriptions = 0;
    vertex_input_stage.vertexAttributeDescriptionCount = 0;
    vertex_input_stage.pVertexAttributeDescriptions = 0;

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &result.desc_layout;

    VK_CHECK(vkCreatePipelineLayout(vulkan_state.ldevice, &layout_info, 0, &result.layout));

	VkPipelineRenderingCreateInfo rendering_create_info = {};
	rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_create_info.colorAttachmentCount = options->color_formats_count;
    rendering_create_info.pColorAttachmentFormats = options->color_formats;
	rendering_create_info.depthAttachmentFormat = options->depth_format;

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = options->shaders_count;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pInputAssemblyState = &input_assembly_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pViewportState = &viewport_stage;
    pipeline_info.pDynamicState = &dynamic_stage;
    pipeline_info.pColorBlendState = &color_blend_stage;
    pipeline_info.pVertexInputState = &vertex_input_stage;
    pipeline_info.layout = result.layout;
    pipeline_info.pNext = &rendering_create_info;

    VK_CHECK(vkCreateGraphicsPipelines(vulkan_state.ldevice, VK_NULL_HANDLE, 1, &pipeline_info, 0, &result.handle));

    for (u32 i = 0; i < options->shaders_count; ++i) {
        vkDestroyShaderModule(vulkan_state.ldevice, shader_stages[i].module, 0);
    }

    HeapFree(shader_stages);

    return result;
}

Pipeline CreateComputePipeline(Shader shader, VkDescriptorSetLayoutBinding *bindings, u32 bindings_count) {
    Pipeline result = CreatePipeline(bindings, bindings_count, VK_PIPELINE_BIND_POINT_COMPUTE);

    VkPipelineShaderStageCreateInfo stage_info = LoadShader(shader);

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &result.desc_layout;

    VK_CHECK(vkCreatePipelineLayout(vulkan_state.ldevice, &layout_info, 0, &result.layout));

    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = stage_info;
    pipeline_info.layout = result.layout;

    VK_CHECK(vkCreateComputePipelines(vulkan_state.ldevice, VK_NULL_HANDLE, 1, &pipeline_info, 0, &result.handle));

    vkDestroyShaderModule(vulkan_state.ldevice, stage_info.module, 0);

    return result;
}

void DestroyPipeline(Pipeline pipeline) {
    vkDestroyDescriptorPool(vulkan_state.ldevice, pipeline.desc_pool, 0);
    vkDestroyDescriptorSetLayout(vulkan_state.ldevice, pipeline.desc_layout, 0);
    vkDestroyPipelineLayout(vulkan_state.ldevice, pipeline.layout, 0);
    vkDestroyPipeline(vulkan_state.ldevice, pipeline.handle, 0);
}

void BindPipeline(Pipeline *p, VkCommandBuffer cmdbuf) {
	vkCmdBindPipeline(cmdbuf, p->bind_point, p->handle);
	vkCmdBindDescriptorSets(cmdbuf, p->bind_point, p->layout, 0, 1, &p->desc_set, 0, 0);
}

void BindBuffer(Pipeline *p, u32 binding, Buffer *buffer, VkDeviceSize size, VkDescriptorType type) {
	VkDescriptorBufferInfo desc = { buffer->handle, 0, size };

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = p->desc_set;
	write.dstBinding = binding;
	write.descriptorCount = 1;
    write.descriptorType = type;
	write.pBufferInfo = &desc;

	vkUpdateDescriptorSets(vulkan_state.ldevice, 1, &write, 0, 0);
}

void BindTexture(Pipeline *p, u32 binding, Texture texture) {
	VkWriteDescriptorSet desc_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	desc_write.dstSet = p->desc_set;
	desc_write.dstBinding = binding;
    desc_write.descriptorCount = 1;
	desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_write.pImageInfo = &texture.descriptor;

	vkUpdateDescriptorSets(vulkan_state.ldevice, 1, &desc_write, 0, 0);
}

void BindTextureArray(Pipeline *p, u32 binding, TextureArray *textures) {
	VkWriteDescriptorSet desc_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	desc_write.dstSet = p->desc_set;
	desc_write.dstBinding = binding;
	desc_write.descriptorCount = textures->count;
	desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_write.pImageInfo = textures->descriptors;

	vkUpdateDescriptorSets(vulkan_state.ldevice, 1, &desc_write, 0, 0);
}

void CopyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkCommandPool cmdpool) {
    VkCommandBuffer cmdbuf = BeginTempCommandBuffer(cmdpool);

    VkBufferCopy copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copy);

    EndTempCommandBuffer(cmdpool, cmdbuf);
}

Buffer CreateBuffer(VkCommandPool cmdpool, VkBufferUsageFlags usage, VkDeviceSize size, void *data) {
    Buffer result = {};

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateBuffer(vulkan_state.allocator, &info, &alloc_create_info, &result.handle, &result.allocation, 0));

    if (data) {
        StagingBuffer staging = CreateStagingBuffer(size, data);
        CopyBuffer(result.handle, staging.handle, size, cmdpool);
        DestroyStagingBuffer(staging);
    }

    return result;
}

StagingBuffer CreateStagingBuffer(VkDeviceSize size, void *data) {
    StagingBuffer result = {};

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VK_CHECK(vmaCreateBuffer(vulkan_state.allocator, &info, &alloc_create_info, &result.handle, &result.allocation, &result.allocation_info));

    if (data) {
        CopyMemory(result.allocation_info.pMappedData, data, size);
    }

    return result;
}

void DestroyBuffer(Buffer buffer) {
    vmaDestroyBuffer(vulkan_state.allocator, buffer.handle, buffer.allocation);
}

void DestroyStagingBuffer(StagingBuffer buffer) {
    vmaDestroyBuffer(vulkan_state.allocator, buffer.handle, buffer.allocation);
}

void UpdateRendererBuffer(Buffer buffer, VkDeviceSize size, void *data, VkCommandBuffer cmdbuf) {
    vkCmdUpdateBuffer(cmdbuf, buffer.handle, 0, size, data);

    VkBufferMemoryBarrier2 after = CreateBufferBarrier(buffer.handle, size,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, &after, 1);
}

VkQueryPool CreateQueryPool(uint32_t count, VkQueryType type) {
    VkQueryPool result = 0;

    VkQueryPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = type;
    info.queryCount = count;

    if (type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
        info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
    }

    VK_CHECK(vkCreateQueryPool(vulkan_state.ldevice, &info, 0, &result));

    return result;
}

void DestroyQueryPool(VkQueryPool pool) {
    vkDestroyQueryPool(vulkan_state.ldevice, pool, 0);
}

void WaitForDeviceIdle() {
	VK_CHECK(vkDeviceWaitIdle(vulkan_state.ldevice));
}

VkPhysicalDevice GetPhysicalDevice() {
    return vulkan_state.pdevice;
}

VkDevice GetLogicalDevice() {
    return vulkan_state.ldevice;
}
