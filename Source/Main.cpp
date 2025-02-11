#include <stdio.h>

#include "ThirdParty/stb_image.h"

#include "General.h"
#include "Window/Window.h"
#include "Graphics/NVulkan.h"
#include "Math/Vec.h"
#include "Math/Mat.h"
#include "Math/NMath.h"

enum {
	CHUNK_X = 16,
	CHUNK_Y = 16,
	CHUNK_Z = 16,
};

enum {
	WORLD_CHUNK_COUNT_X = 8,
	WORLD_CHUNK_COUNT_Z = 8,
};

enum {
	BLOCK_AIR,
	BLOCK_DIRT,
	BLOCK_GRASS,
	BLOCK_OAK_LOG,
	BLOCK_STONE,
	BLOCK_COBBLE_STONE,
	BLOCK_STONE_BRICKS
};

enum {
	TEXTURE_DIRT,
	TEXTURE_GRASS_SIDE,
	TEXTURE_GRASS_TOP,
	TEXTURE_OAK_LOG_SIDE,
	TEXTURE_OAK_LOG_TOP,
	TEXTURE_STONE,
	TEXTURE_COBBLE_STONE,
	TEXTURE_STONE_BRICKS,

	TEXTURE_COUNT
};

enum {
	MAX_INSTANCE_COUNT = 1000000
};

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec2 uv;
};

struct FrustumInfo {
	mat4 view_matrix;
	vec4 planes[6];
	u32 instance_count;
};

struct BlockTexture {
	uint sides;
	uint top;
	uint bot;
};

struct InstanceData {
	vec3 pos;
	BlockTexture texture;
};

struct Block {
	int type;
};

struct Chunk {
	Block blocks[CHUNK_X][CHUNK_Z][CHUNK_Y];
	vec3 world_pos;
};

struct Renderer {
	DescriptorSet render_desc_set;
	DescriptorSet cull_desc_set;
	DescriptorSet cull_pass_desc_set;
	Pipeline render_pipeline;
	Pipeline cull_pipeline;
	Pipeline cull_pass_pipeline;
	VkCommandBuffer cmdbuf;
	Buffer vertex_buffer;
	Buffer index_buffer;
	Buffer instance_buffer;
	StagingBuffer instance_staging_buffer;
	Buffer culled_instance_buffer;
	Buffer culled_counter_buffer;
	Buffer indirect_buffer;
	Buffer frustum_info_buffer;
	Buffer globals_buffer;
};

struct Globals {
	mat4 proj_matrix;
	mat4 view_matrix;
};

struct OrbitCamera {
    vec3 position;
    vec3 target;

    float radius;
    float yaw;
    float pitch;

    mat4 proj_matrix;
    mat4 view_matrix;

    float width;
    float height;
};

struct Textures {
	Image images[TEXTURE_COUNT];
	VkDescriptorImageInfo descriptors[TEXTURE_COUNT];
};

global const Vertex cube_vertices[] = {
	// back
	{vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec2(0.0f, 0.0f)},
	{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec2(0.0f, 1.0f)},
	{vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec2(1.0f, 1.0f)},
	{vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec2(1.0f, 0.0f)},

	// bot
	{vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f)},
	{vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec2(0.0f, 1.0f)},
	{vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec2(1.0f, 1.0f)},
	{vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f)},

	// top
	{vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 0.0f)},
	{vec3(0.0f, 1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f)},
	{vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f)},
	{vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec2(1.0f, 0.0f)},

	// right
	{vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f)},
	{vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec2(0.0f, 1.0f)},
	{vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 1.0f)},
	{vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f)},

	// left
	{vec3(0.0f, 1.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec2(0.0f, 0.0f)},
	{vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec2(0.0f, 1.0f)},
	{vec3(0.0f, 0.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f), vec2(1.0f, 1.0f)},
	{vec3(0.0f, 1.0f, 1.0f), vec3(-1.0f, 0.0f, 0.0f), vec2(1.0f, 0.0f)},

	// front
	{vec3(0.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 0.0f)},
	{vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec2(0.0f, 1.0f)},
	{vec3(1.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 1.0f)},
	{vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f), vec2(1.0f, 0.0f)},
};

global const u32 cube_indices[] = {
	// back
	0, 1, 2,
	0, 2, 3,

	// bottom
	4, 5, 6,
	4, 6, 7,

	// top
	8, 9, 10,
	8, 10, 11,

	// right
	12, 13, 14,
	12, 14, 15,

	// left
	16, 17, 18,
	16, 18, 19,

	// front
	20, 21, 22,
	20, 22, 23
};

global Chunk chunks[WORLD_CHUNK_COUNT_X][WORLD_CHUNK_COUNT_Z];

global BlockTexture block_textures_map[] = {
	{},
	{TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT},
	{TEXTURE_GRASS_SIDE, TEXTURE_GRASS_TOP, TEXTURE_DIRT},
	{TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_TOP, TEXTURE_OAK_LOG_TOP},
	{TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE},
	{TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE},
	{TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS},
};

OrbitCamera CreateOrbitCamera() {
    OrbitCamera result = {};

    result.position = vec3(0.f, 0.f, -10.f);
    result.target = vec3(0.f);
    result.radius = 10.f;
    result.yaw = 0.f;
    result.pitch = 0.f;

    return result;
}

void ResizeOrbitCamera(OrbitCamera *camera, float width, float height) {
    camera->width = width;
    camera->height = height;
}

void UploadOrbitCameraMatrices(OrbitCamera *camera, Renderer *r, VkCommandBuffer cmdbuf) {
    mat4 proj_matrix = Perspective(PI32 / 3.0f, camera->width / camera->height, 0.01f, 1000.f);
    mat4 view_matrix = LookAt(camera->position, camera->target, vec3(0.f, 1.f, 0.f));

	camera->proj_matrix = proj_matrix;
	camera->view_matrix = view_matrix;

	Globals globals = { proj_matrix, view_matrix };
    UpdateRendererBuffer(r->globals_buffer, sizeof(globals), &globals, cmdbuf);
}

bool UpdateOrbitCamera(OrbitCamera *camera) {
    bool result = false;

	int scroll = GetMouseScrollDelta();
    Int2 mouse_pos = GetMousePosition();
    Int2 mouse_delta = GetMouseDeltaPosition();

    float border_threshold = 20.0f;
    float strafe_speed = 0.2f;

    if (scroll != 0) {
		camera->radius -= float(scroll) * 2.0f;

        camera->radius = Clamp(camera->radius, 1.f, 100.f);

        result = true;
    }

    if (IsButtonDown(MOUSE_BUTTON_LEFT)) {
        camera->yaw += mouse_delta.x * 0.01f;
        camera->pitch += mouse_delta.y * 0.01f;

        camera->yaw = FMod(camera->yaw, 2.f * PI32);

        const float eps = 0.01f;

        if (camera->pitch >= PI32 / 2.f - eps) {
            camera->pitch = PI32 / 2.f - eps;
        }

        if (camera->pitch <= -(PI32 / 2.f - eps)) {
            camera->pitch = -(PI32 / 2.f - eps);
        }

        result = true;
    }

    if (IsButtonDown(MOUSE_BUTTON_RIGHT)) {
        vec3 up = vec3(0.0f, 1.0f, 0.0f);
        vec3 forward = Normalize(camera->target - camera->position);

        vec3 axis_x = Normalize(Cross(forward, up));
        vec3 axis_y = Normalize(Cross(axis_x, forward));

        camera->target = camera->target - axis_x * mouse_delta.x * 0.02f;
        camera->target = camera->target + axis_y * mouse_delta.y * 0.02f;

        result = true;
    }

    if (result) {
        float y = camera->target.y + Sin(camera->pitch) * camera->radius;
        float h = Cos(camera->pitch) * camera->radius;

        float x = camera->target.x + Sin(-camera->yaw) * h;
        float z = camera->target.z + Cos(-camera->yaw) * h;

        vec3 pos = vec3(x, y, z);

        camera->position = pos;
    }

    return result;
}

Renderer CreateRenderer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf,
	VkFormat color_format, VkFormat depth_format) {
	Renderer result = {};

	// Graphics pipeline
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, 0}
	};
	result.render_desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings));

	Shader shaders[] = {
		{"Assets/Shaders/Core-Vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Core-Frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	VkPipelineRenderingCreateInfo rendering_create_info = {};
	rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	rendering_create_info.colorAttachmentCount = 1;
	rendering_create_info.pColorAttachmentFormats = &color_format;
	rendering_create_info.depthAttachmentFormat = depth_format;

	result.render_pipeline = CreateGraphicsPipeline(result.render_desc_set.layout, rendering_create_info, VK_CULL_MODE_BACK_BIT, VK_TRUE, shaders, ArrayCount(shaders));

	// Culling pipeline
	VkDescriptorSetLayoutBinding culling_bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0}
	};
	result.cull_desc_set = CreateDescriptorSet(culling_bindings, ArrayCount(culling_bindings));

	Shader culling_shader = {
		"Assets/Shaders/Culling-Comp.spv", VK_SHADER_STAGE_COMPUTE_BIT
	};

	result.cull_pipeline = CreateComputePipeline(result.cull_desc_set.layout, culling_shader);

	// Culling pass pipeline
	VkDescriptorSetLayoutBinding culling_pass_bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
	};
	result.cull_pass_desc_set = CreateDescriptorSet(culling_pass_bindings, ArrayCount(culling_pass_bindings));

	Shader culling_pass_shader = {
		"Assets/Shaders/CullingPass-Comp.spv", VK_SHADER_STAGE_COMPUTE_BIT
	};

	result.cull_pass_pipeline = CreateComputePipeline(result.cull_pass_desc_set.layout, culling_pass_shader);

	result.cmdbuf = cmdbuf;

	result.vertex_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(Vertex) * ArrayCount(cube_vertices), (void *)cube_vertices);
	result.index_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(u32) * ArrayCount(cube_indices), (void *) cube_indices);
	result.instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.instance_staging_buffer = CreateStagingBuffer(sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.culled_instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.culled_counter_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(u32), 0);

	result.frustum_info_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrustumInfo), 0);

	VkDrawIndexedIndirectCommand indirect_cmd = {ArrayCount(cube_indices), 0, 0, 0, 0};
	result.indirect_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, sizeof(VkDrawIndirectCommand) + sizeof(u32), &indirect_cmd);

	Globals globals = {};
	result.globals_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(globals), &globals);

	return result;
}

void DestroyRenderer(Renderer *r) {
	DestroyBuffer(r->vertex_buffer);
	DestroyBuffer(r->index_buffer);
	DestroyBuffer(r->instance_buffer);
	DestroyStagingBuffer(r->instance_staging_buffer);
	DestroyBuffer(r->culled_instance_buffer);
	DestroyBuffer(r->culled_counter_buffer);
	DestroyBuffer(r->frustum_info_buffer);
	DestroyBuffer(r->indirect_buffer);
	DestroyBuffer(r->globals_buffer);

	DestroyPipeline(r->render_pipeline);
	FreeDescriptorSet(r->render_desc_set);
	DestroyPipeline(r->cull_pipeline);
	FreeDescriptorSet(r->cull_desc_set);
	DestroyPipeline(r->cull_pass_pipeline);
	FreeDescriptorSet(r->cull_pass_desc_set);
}

u32 UpdateBlockInstances(Renderer *r, OrbitCamera *camera, VkCommandBuffer cmdbuf) {
	InstanceData *instance_data = (InstanceData *) r->instance_staging_buffer.allocation_info.pMappedData;

	u32 instance_count = 0;
	
	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			Chunk c = chunks[cx][cz];

			for (int x = 0; x < CHUNK_X; ++x) {
				for (int z = 0; z < CHUNK_Z; ++z) {
					for (int y = 0; y < CHUNK_Y; ++y) {
						Block b = c.blocks[x][z][y];
						if (b.type != BLOCK_AIR) {
							InstanceData *id = instance_data + instance_count;
							id->pos = c.world_pos + vec3(x, y, z);
							id->texture = block_textures_map[b.type];
							instance_count++;
						}
					}
				}
			}
		}
	}

    VkBufferCopy copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = instance_count * sizeof(InstanceData);
    vkCmdCopyBuffer(cmdbuf, r->instance_staging_buffer.handle, r->instance_buffer.handle, 1, &copy);

	vkCmdFillBuffer(cmdbuf, r->culled_counter_buffer.handle, 0, sizeof(u32), 0);

	FrustumInfo frustum_info = {};
	mat4 proj_t = Transpose(camera->proj_matrix);
	vec4 row0 = proj_t[0];
	vec4 row1 = proj_t[1];
	vec4 row2 = proj_t[2];
	vec4 row3 = proj_t[3];

	frustum_info.view_matrix = camera->view_matrix;
	frustum_info.planes[0] = Normalize(row3 + row0); // left
	frustum_info.planes[1] = Normalize(row3 - row0); // right
	frustum_info.planes[2] = Normalize(row3 + row1); // top
	frustum_info.planes[3] = Normalize(row3 - row1); // bottom
	frustum_info.planes[4] = Normalize(row3 + row2); // near
	frustum_info.planes[5] = Normalize(row3 - row2); // far
	frustum_info.instance_count = instance_count;

	UpdateRendererBuffer(r->frustum_info_buffer, sizeof(frustum_info), &frustum_info, cmdbuf);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, r->cull_pipeline.handle);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, r->cull_pipeline.layout, 0, 1, &r->cull_desc_set.handle, 0, 0);

	if (instance_count > 0) {
		VkDescriptorBufferInfo instance_desc = { r->instance_buffer.handle, 0, instance_count * sizeof(InstanceData) };

		VkWriteDescriptorSet instance_write = {};
		instance_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		instance_write.dstSet = r->cull_desc_set.handle;
		instance_write.dstBinding = 0;
		instance_write.descriptorCount = 1;
		instance_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instance_write.pBufferInfo = &instance_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &instance_write, 0, 0);
	}

	if (instance_count > 0) {
		VkDescriptorBufferInfo culled_instance_desc = { r->culled_instance_buffer.handle, 0, instance_count * sizeof(InstanceData) };

		VkWriteDescriptorSet culled_instance_write = {};
		culled_instance_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		culled_instance_write.dstSet = r->cull_desc_set.handle;
		culled_instance_write.dstBinding = 1;
		culled_instance_write.descriptorCount = 1;
		culled_instance_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		culled_instance_write.pBufferInfo = &culled_instance_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &culled_instance_write, 0, 0);
	}

	{
		VkDescriptorBufferInfo culled_counter_desc = { r->culled_counter_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet culled_counter_write = {};
		culled_counter_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		culled_counter_write.dstSet = r->cull_desc_set.handle;
		culled_counter_write.dstBinding = 2;
		culled_counter_write.descriptorCount = 1;
		culled_counter_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		culled_counter_write.pBufferInfo = &culled_counter_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &culled_counter_write, 0, 0);
	}

	{
		VkDescriptorBufferInfo frustum_info_desc = { r->frustum_info_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet frustum_info_write = {};
		frustum_info_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		frustum_info_write.dstSet = r->cull_desc_set.handle;
		frustum_info_write.dstBinding = 3;
		frustum_info_write.descriptorCount = 1;
		frustum_info_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		frustum_info_write.pBufferInfo = &frustum_info_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &frustum_info_write, 0, 0);
	}
	
	u32 group_count = (instance_count + 63) / 64;
	vkCmdDispatch(cmdbuf, group_count, 1, 1);

	VkBufferMemoryBarrier2 cull_pass_barrier = CreateBufferBarrier(
		r->culled_counter_buffer.handle, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT
	);
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, &cull_pass_barrier, 1);

	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, r->cull_pass_pipeline.handle);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, r->cull_pass_pipeline.layout, 0, 1, &r->cull_pass_desc_set.handle, 0, 0);
	
	{
		VkDescriptorBufferInfo culled_counter_desc = { r->culled_counter_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet culled_counter_write = {};
		culled_counter_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		culled_counter_write.dstSet = r->cull_pass_desc_set.handle;
		culled_counter_write.dstBinding = 0;
		culled_counter_write.descriptorCount = 1;
		culled_counter_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		culled_counter_write.pBufferInfo = &culled_counter_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &culled_counter_write, 0, 0);
	}

	{
		VkDescriptorBufferInfo indirect_buffer_desc = { r->indirect_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet indirect_buffer_write = {};
		indirect_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		indirect_buffer_write.dstSet = r->cull_pass_desc_set.handle;
		indirect_buffer_write.dstBinding = 1;
		indirect_buffer_write.descriptorCount = 1;
		indirect_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indirect_buffer_write.pBufferInfo = &indirect_buffer_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &indirect_buffer_write, 0, 0);
	}

	vkCmdDispatch(cmdbuf, 1, 1, 1);

	VkBufferMemoryBarrier2 cull_barrier = CreateBufferBarrier(
		r->indirect_buffer.handle, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT
	);
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, &cull_barrier, 1);

	return instance_count;
}

void Render(Renderer *r, Textures *textures, VkCommandBuffer cmdbuf, u32 instance_count) {
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, r->render_pipeline.handle);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, r->render_pipeline.layout, 0, 1, &r->render_desc_set.handle, 0, 0);

	{
		VkDescriptorBufferInfo vertex_desc = { r->vertex_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet vertex_write = {};
		vertex_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertex_write.dstSet = r->render_desc_set.handle;
		vertex_write.dstBinding = 0;
		vertex_write.descriptorCount = 1;
		vertex_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertex_write.pBufferInfo = &vertex_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &vertex_write, 0, 0);
	}

	{
		VkDescriptorBufferInfo globals_buffer_desc = { r->globals_buffer.handle, 0, VK_WHOLE_SIZE };

		VkWriteDescriptorSet desc_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		desc_write.dstSet = r->render_desc_set.handle;
		desc_write.dstBinding = 1;
		desc_write.descriptorCount = 1;
		desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_write.pBufferInfo = &globals_buffer_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &desc_write, 0, 0);
	}

	if (instance_count > 0) {
		VkDescriptorBufferInfo instance_desc = { r->culled_instance_buffer.handle, 0, instance_count * sizeof(InstanceData) };

		VkWriteDescriptorSet instance_write = {};
		instance_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		instance_write.dstSet = r->render_desc_set.handle;
		instance_write.dstBinding = 2;
		instance_write.descriptorCount = 1;
		instance_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instance_write.pBufferInfo = &instance_desc;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &instance_write, 0, 0);
	}
	
	{
		VkWriteDescriptorSet desc_write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		desc_write.dstSet = r->render_desc_set.handle;
		desc_write.dstBinding = 3;
		desc_write.descriptorCount = TEXTURE_COUNT;
		desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc_write.pImageInfo = textures->descriptors;

		vkUpdateDescriptorSets(GetLogicalDevice(), 1, &desc_write, 0, 0);
	}

	vkCmdBindIndexBuffer(cmdbuf, r->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
	// vkCmdDrawIndexed(cmdbuf, ArrayCount(cube_indices), instance_count, 0, 0, 0);
	vkCmdDrawIndexedIndirect(cmdbuf, r->indirect_buffer.handle, 0, 1, sizeof(VkDrawIndirectCommand));
}

void LoadTextureFromFile(Textures *textures, uint slot, const char *path, VkCommandPool cmdpool) {
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

	Texture result;
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

		result = CreateTextureFromPixels(w, h, 4, VK_FORMAT_R8G8B8A8_UNORM, rgba_pixels, sampler_info, cmdpool);

		HeapFree(rgba_pixels);
	} else {
		result = CreateTextureFromPixels(w, h, channels, VK_FORMAT_R8G8B8A8_UNORM, pixels, sampler_info, cmdpool);
	}
	
	free(pixels);

	textures->images[slot] = result.image;
	textures->descriptors[slot] = result.descriptor;
}

void LoadTextures(Textures *textures, VkCommandPool cmdpool) {
	LoadTextureFromFile(textures, TEXTURE_DIRT, "Assets/Textures/dirt.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_GRASS_SIDE, "Assets/Textures/grass_side.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_GRASS_TOP, "Assets/Textures/grass_top.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_OAK_LOG_SIDE, "Assets/Textures/log_oak.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_OAK_LOG_TOP, "Assets/Textures/log_oak_top.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_STONE, "Assets/Textures/stone.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_COBBLE_STONE, "Assets/Textures/cobblestone.png", cmdpool);
	LoadTextureFromFile(textures, TEXTURE_STONE_BRICKS, "Assets/Textures/stone_bricks.png", cmdpool);
}

void ReleaseTextures(Textures *textures) {
	for (int i = 0; i < TEXTURE_COUNT; ++i) {
		Image image = textures->images[i];
		VkDescriptorImageInfo descriptor = textures->descriptors[i];

		Texture t;
		t.image = image;
		t.descriptor = descriptor;

		DestroyTexture(t);
	}
}

void NKMain() {
	Window window = {};
	window.title = "nmc";
	window.size.x = 1280;
	window.size.y = 720;
	
	if (!InitWindow(&window)) {
		Print("Failed to initialize window!\n");
		Exit(1);
	}

	InitVulkan(&window);
	VkCommandPool cmdpool = CreateCommandPool();
	Swapchain swapchain = {};
	CreateSwapchain(&swapchain, cmdpool);
	VkCommandBuffer cmdbuf;
	AllocateCommandBuffers(cmdpool, &cmdbuf, 1);
	Image color_target = CreateImage(swapchain.width, swapchain.height, swapchain.format.format, 1,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	Image depth_target = CreateImage(swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT, 1,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	VkQueryPool pipeline_queries = CreateQueryPool(1, VK_QUERY_TYPE_PIPELINE_STATISTICS);

	Renderer renderer = CreateRenderer(cmdpool, cmdbuf, color_target.format, depth_target.format);
	OrbitCamera orbit_camera = CreateOrbitCamera();
	ResizeOrbitCamera(&orbit_camera, float(swapchain.width), float(swapchain.height));

	{
		VkCommandBuffer temp_cmdbuf = BeginTempCommandBuffer(cmdpool);

		UploadOrbitCameraMatrices(&orbit_camera, &renderer, temp_cmdbuf);

		EndTempCommandBuffer(cmdpool, temp_cmdbuf);
	}

	Textures textures;
	LoadTextures(&textures, cmdpool);

	VkPhysicalDeviceProperties pdev_props;
	vkGetPhysicalDeviceProperties(GetPhysicalDevice(), &pdev_props);
	Assert(pdev_props.limits.timestampComputeAndGraphics);

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			Chunk *c = &chunks[cx][cz];
			c->world_pos = vec3(cx * CHUNK_X, 0, cz * CHUNK_Z);

			for (int x = 0; x < CHUNK_X; ++x) {
				for (int z = 0; z < CHUNK_Z; ++z) {
					for (int y = 0; y < CHUNK_Y; ++y) {
						if (y < 11) {
							c->blocks[x][z][y].type = BLOCK_STONE;
						} else if (y < 15) {
							c->blocks[x][z][y].type = BLOCK_DIRT;
						} else {
							c->blocks[x][z][y].type = BLOCK_GRASS;
						}
					}
				}
			}
		}
	}

	double cpu_time_avg = 0.0;
	double gpu_time_avg = 0.0;
	u64 triangles = 0.0;
	double triangles_per_sec = 0.0;

	while (window.running) {
		u64 cpu_time_begin = GetTimeNowUs();

		UpdateWindow(&window);

		if (UpdateOrbitCamera(&orbit_camera)) {
			VkCommandBuffer temp_cmdbuf = BeginTempCommandBuffer(cmdpool);

			UploadOrbitCameraMatrices(&orbit_camera, &renderer, temp_cmdbuf);

			EndTempCommandBuffer(cmdpool, temp_cmdbuf);
		}

		if (window.resized) {
			UpdateSwapchain(&swapchain, cmdpool, 1);

			DestroyImage(color_target);
			DestroyImage(depth_target);
			color_target = CreateImage(swapchain.width, swapchain.height, swapchain.format.format, 1,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			depth_target = CreateImage(swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT, 1,
				VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			VkCommandBuffer temp_cmdbuf = BeginTempCommandBuffer(cmdpool);
			ResizeOrbitCamera(&orbit_camera, float(swapchain.width), float(swapchain.height));
			UploadOrbitCameraMatrices(&orbit_camera, &renderer, temp_cmdbuf);
			EndTempCommandBuffer(cmdpool, temp_cmdbuf);
		}

		if (!AcquireSwapchain(&swapchain, cmdpool, cmdbuf, color_target, depth_target)) {
			continue;
		}

		u32 instance_count = UpdateBlockInstances(&renderer, &orbit_camera, cmdbuf);

		vkCmdResetQueryPool(cmdbuf, pipeline_queries, 0, 1);
		vkCmdBeginQuery(cmdbuf, pipeline_queries, 0, 0);

		VkClearColorValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearDepthStencilValue depth_clear = { 1.0f, 0 };

		VkRenderingAttachmentInfo color_attachment = {};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = color_target.view;
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color = clear_color;

		VkRenderingAttachmentInfo depth_attachment = {};
		depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depth_attachment.imageView = depth_target.view;
		depth_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.clearValue.depthStencil = depth_clear;

		VkRenderingInfo rendering_info = {};
		rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering_info.renderArea.extent.width = swapchain.width;
		rendering_info.renderArea.extent.height = swapchain.height;
		rendering_info.layerCount = 1;
		rendering_info.colorAttachmentCount = 1;
		rendering_info.pColorAttachments = &color_attachment;
		rendering_info.pDepthAttachment = &depth_attachment;
		vkCmdBeginRendering(cmdbuf, &rendering_info);

		VkViewport viewport = {};
		viewport.width = float(swapchain.width);
		viewport.height = float(swapchain.height);
		viewport.maxDepth = 1;

		VkRect2D scissor = {};
		scissor.extent.width = swapchain.width;
		scissor.extent.height = swapchain.height;

		vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

		Render(&renderer, &textures, cmdbuf, instance_count);

		vkCmdEndRendering(cmdbuf);
		vkCmdEndQuery(cmdbuf, pipeline_queries, 0);

		PresentSwapchain(&swapchain, cmdbuf, color_target);

		uint64_t gpu_timestamps[2] = {};
		VK_CHECK(vkGetQueryPoolResults(GetLogicalDevice(), swapchain.query_pool, 0, 2, sizeof(gpu_timestamps),
			gpu_timestamps, sizeof(gpu_timestamps[0]), VK_QUERY_RESULT_64_BIT));

		uint64_t pipeline_stats[1] = {};
		VK_CHECK(vkGetQueryPoolResults(GetLogicalDevice(), pipeline_queries, 0, 1, sizeof(pipeline_stats),
			pipeline_stats, sizeof(pipeline_stats[0]), VK_QUERY_RESULT_64_BIT));

		double gpu_time_begin = double(gpu_timestamps[0]) * pdev_props.limits.timestampPeriod * 1e-6;
		double gpu_time_end = double(gpu_timestamps[1]) * pdev_props.limits.timestampPeriod * 1e-6;

		gpu_time_avg = gpu_time_avg * 0.95 + (gpu_time_end - gpu_time_begin) * 0.05;

		u64 cpu_time_end = GetTimeNowUs();
		double cpu_time_delta_ms = double(cpu_time_end - cpu_time_begin) / 1000.0;

		cpu_time_avg = cpu_time_avg * 0.95 + cpu_time_delta_ms * 0.05;

		triangles = pipeline_stats[0];
		triangles_per_sec = double(triangles) / double(gpu_time_avg * 1e-3);

		char perf_title[128];
		snprintf(perf_title, sizeof(perf_title), "cpu: %.2fms, gpu: %.2fms, tri: %llu, tri/sec: %.2fM", cpu_time_avg, gpu_time_avg, triangles, triangles_per_sec * 1e-6);
		SetWindowTitle(&window, perf_title);
	}

	WaitForDeviceIdle();

	DestroyQueryPool(pipeline_queries);

	ReleaseTextures(&textures);
	DestroyRenderer(&renderer);

	DestroyImage(depth_target);
	DestroyImage(color_target);
	FreeCommandBuffers(cmdpool, &cmdbuf, 1);
	DestroySwapchain(&swapchain);
	DestroyCommandPool(cmdpool);

	ReleaseVulkan();

	DestroyWindow(&window);
}