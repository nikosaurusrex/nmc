#pragma once

#include "General.h"
#include "Graphics/NVulkan.h"
#include "Math/Mat.h"
#include "World.h"
#include "Player.h"

enum {
	MAX_INSTANCE_COUNT = 10000000
};

enum {
	SHADOW_MAP_WIDTH = 2048,
	SHADOW_MAP_HEIGHT = 2048,
};

struct FrustumInfo {
	mat4 view_matrix;
	vec4 planes[6];
	u32 instance_count;
};

struct SkyUniform {
	mat4 inv_proj;
	mat4 inv_view;
	vec3 camera_pos;
};

struct RenderPass {
	DescriptorSet desc_set;
	Pipeline pipeline;

	Buffer instance_buffer;
	StagingBuffer instance_staging_buffer;
	Buffer culled_instance_buffer;
	Buffer indirect_buffer;
};

struct ShadowPass {
	DescriptorSet desc_set;
	Pipeline pipeline;
	Buffer light_space_buffer;
	Texture shadow_map;
};

struct CullPass {
	DescriptorSet desc_sets[2];
	Pipeline pipeline;
	Buffer frustum_info_buffer;
};

struct CullCall {
	Buffer instance_buffer;
	Buffer culled_instance_buffer;
	Buffer indirect_buffer;
	mat4 proj_matrix;
	mat4 view_matrix;
	u32 desc_set_index;
	u32 instance_count;
};

struct Postprocess {
	DescriptorSet desc_set;
	Pipeline pipeline;
};

struct Renderer {
	RenderPass sky_pass;
	RenderPass solid_pass;
	RenderPass water_pass;
	ShadowPass shadow_pass;
	CullPass cull_pass;
	Postprocess post_process;

	TextureArray textures;
	Texture noise_texture;
	Texture water_texture1;
	Texture water_texture2;
	Buffer globals_buffer;
	Buffer sky_buffer;
};

struct BlockInstanceCounts {
	u32 solid;
	u32 water;
};

struct Globals {
	mat4 proj_matrix;
	mat4 view_matrix;
	mat4 light_space_matrix;
	vec3 camera_pos;
};

void InitRenderer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf,
	VkFormat color_format, VkFormat depth_format);
void DestroyRenderer();

void Cull(Player *p, BlockInstanceCounts instance_counts, VkCommandBuffer cmdbuf);
void RenderShadow(VkCommandBuffer cmdbuf, BlockInstanceCounts instance_counts);
void Render(Swapchain *swapchain, VkImageView color_view, VkImageView depth_view, VkCommandBuffer cmdbuf,
	BlockInstanceCounts instance_counts, float time);
void DoPostprocessing(Swapchain *swapchain, Texture render_target, Image swapchain_target, VkCommandBuffer cmdbuf);
void UploadTransformations(Player *p, VkCommandBuffer cmdbuf);

BlockInstanceCounts UpdateBlockInstances(VkCommandBuffer cmdbuf, BlockInstanceCounts prev_instance_counts);
