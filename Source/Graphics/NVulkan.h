#pragma once

#include "../General.h"
#include "../Window/Window.h"

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#define VK_CHECK(call)                                                                             \
    if (call != VK_SUCCESS) {                                                                      \
        NK_TRAP();                                                                                 \
    }

#define VK_CHECK_SWAPCHAIN(call)                                                                   \
    do {                                                                                           \
        VkResult cres = call;                                                                      \
        if (cres != VK_SUCCESS && cres != VK_SUBOPTIMAL_KHR && cres != VK_ERROR_OUT_OF_DATE_KHR) { \
            NK_TRAP();                                                                             \
        }                                                                                          \
    } while(0)

#define SWAPCHAIN_IMAGE_COUNT 2
struct Swapchain {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR format;
    VkQueryPool query_pool;

    u32 width;
    u32 height;
    u32 current_semaphore;
    u32 current_image;

    VkSemaphore acquire_semaphore;
    VkSemaphore release_semaphore;
    VkFence frame_fence;
    VkImage images[SWAPCHAIN_IMAGE_COUNT];
};

struct Image {
    VkImage handle;
    VkImageView view;
    VmaAllocation allocation;
    VkFormat format;
};

struct Texture {
    Image image;
    VkDescriptorImageInfo descriptor;
};

struct TextureArray {
    Image *images;
    VkDescriptorImageInfo *descriptors;
    uint capacity;
    uint count;
};

struct Shader {
    const char *path;
    VkShaderStageFlagBits stage;
};

struct GraphicsPipelineOptions {
    VkFormat *color_formats;
    u32 color_formats_count;
    VkFormat depth_format;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;
    VkBool32 depth_read;
    VkBool32 depth_write;
    VkBool32 blend;
    Shader *shaders;
    u32 shaders_count;
};

struct DescriptorSet {
    VkDescriptorSet handle;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
};

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkPipelineBindPoint bind_point;
};

struct Buffer {
    VkBuffer handle;
    VmaAllocation allocation;
};

struct StagingBuffer {
    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
};

void InitVulkan(Window *window);
void ReleaseVulkan();

VkCommandPool CreateCommandPool();
void DestroyCommandPool(VkCommandPool cmdpool);

void AllocateCommandBuffers(VkCommandPool cmdpool, VkCommandBuffer *buffers, u32 buffers_count);
void FreeCommandBuffers(VkCommandPool cmdpool, VkCommandBuffer *buffers, u32 buffers_count);

void BeginCommandBuffer(VkCommandBuffer cmdbuf);
void EndCommandBuffer(VkCommandBuffer cmdbuf);
void SubmitCommandBuffer(VkCommandBuffer cmdbuf);

VkCommandBuffer BeginTempCommandBuffer(VkCommandPool cmdpool);
void EndTempCommandBuffer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf);

void CreateSwapchain(Swapchain *swapchain, VkCommandPool cmdpool);
void DestroySwapchain(Swapchain *swapchain);
void UpdateSwapchain(Swapchain *swapchain, VkCommandPool cmdpool, b8 vsync);
b32 AcquireSwapchain(Swapchain *swapchain, VkCommandPool cmdpool, VkCommandBuffer cmdbuf, Image color_target, Image depth_target);
void PresentSwapchain(Swapchain *swapchain, VkCommandBuffer cmdbuf, Image color_target);

Image CreateImage(u32 width, u32 height, VkFormat format, u32 mip_levels, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage);
Image CreateDepthImage(Swapchain *swapchain, VkCommandPool cmdpool);
void DestroyImage(Image image);

Texture CreateTexture(u32 width, u32 height, VkFormat format, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage, VkSamplerCreateInfo sampler_info);
Texture CreateTexture(u32 width, u32 height, VkFormat format, VkImageAspectFlags aspect_mask, VkImageUsageFlags usage);
Texture CreateTextureFromPixels(u32 width, u32 height, u32 channels, VkFormat format, u8 *pixels,
    VkSamplerCreateInfo sampler_info, VkCommandPool cmdpool);
Texture LoadTextureFromFile(const char *path, VkCommandPool cmdpool);
void DestroyTexture(Texture tex);

TextureArray CreateTextureArray(uint count);
void AddTexture(TextureArray *ta, Texture tex);
void LoadTextureAtSlot(TextureArray *ta, uint slot, const char *path, VkCommandPool cmdpool);
void DestroyTextureArray(TextureArray *ta);

VkImageMemoryBarrier2 CreateImageBarrier(VkImage image, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access, VkImageLayout old_layout,
    VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, VkImageLayout new_layout, VkImageAspectFlags aspect_mask);
void PipelineImageBarriers(VkCommandBuffer cmdbuf, VkDependencyFlags flags, VkImageMemoryBarrier2 *barriers, u32 barriers_count);

VkBufferMemoryBarrier2 CreateBufferBarrier(VkBuffer buffer, VkDeviceSize size, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
    VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access);
void PipelineBufferBarriers(VkCommandBuffer cmdbuf, VkDependencyFlags flags, VkBufferMemoryBarrier2 *barriers, u32 barriers_count);

VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count);
DescriptorSet CreateDescriptorSet(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count);
DescriptorSet CreateDescriptorSet(VkDescriptorSetLayoutBinding *bindings, u32 bindings_count, VkDescriptorSetLayout layout);
void DestroyDescriptorSet(DescriptorSet *desc_set);
void BindDescriptorSet(DescriptorSet *desc_set, Pipeline *p, VkCommandBuffer cmdbuf);
void BindBuffer(DescriptorSet *desc_set, u32 binding, Buffer *buffer, VkDeviceSize size, VkDescriptorType type);
void BindTexture(DescriptorSet *desc_set, u32 binding, Texture texture);
void BindTextureArray(DescriptorSet *desc_set, u32 binding, TextureArray *textures);

Pipeline CreateGraphicsPipeline(GraphicsPipelineOptions *options, VkDescriptorSetLayout desc_layout);
Pipeline CreateComputePipeline(Shader shader, VkDescriptorSetLayout desc_layout);
void DestroyPipeline(Pipeline pipeline);

void BindPipeline(Pipeline *p, VkCommandBuffer cmdbuf);

void CopyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkCommandPool cmdpool);
Buffer CreateBuffer(VkCommandPool cmdpool, VkBufferUsageFlags usage, VkDeviceSize size, void *data);
StagingBuffer CreateStagingBuffer(VkDeviceSize size, void *data);
void DestroyBuffer(Buffer buffer);
void DestroyStagingBuffer(StagingBuffer buffer);
void UpdateRendererBuffer(Buffer buffer, VkDeviceSize size, void *data, VkCommandBuffer cmdbuf);

VkQueryPool CreateQueryPool(uint32_t count, VkQueryType type);
void DestroyQueryPool(VkQueryPool pool);

void WaitForDeviceIdle();

VkPhysicalDevice GetPhysicalDevice();
VkDevice GetLogicalDevice();
