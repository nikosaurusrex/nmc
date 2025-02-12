#include <stdio.h>

#include "General.h"
#include "Window/Window.h"
#include "Graphics/NVulkan.h"
#include "Math/Vec.h"
#include "Math/Mat.h"
#include "Math/Quat.h"
#include "Math/NMath.h"

enum {
	CHUNK_X = 16,
	CHUNK_Y = 16,
	CHUNK_Z = 16,
};

enum {
	WORLD_CHUNK_COUNT_X = 8,
	WORLD_CHUNK_COUNT_Y = 1,
	WORLD_CHUNK_COUNT_Z = 8,
};

enum {
	SIDE_TOP,
	SIDE_BOT,
	SIDE_WEST,
	SIDE_EAST,
	SIDE_NORTH,
	SIDE_SOUTH
};

enum {
	BLOCK_AIR,
	BLOCK_DIRT,
	BLOCK_GRASS,
	BLOCK_OAK_LOG,
	BLOCK_STONE,
	BLOCK_COBBLE_STONE,
	BLOCK_STONE_BRICKS,

	BLOCK_COUNT
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
	MAX_INSTANCE_COUNT = 10000000
};

struct FrustumInfo {
	mat4 view_matrix;
	vec4 planes[6];
	u32 instance_count;
};

struct InstanceData {
	vec3 pos;
	u32 side;
	u32 texture;
};

struct Block {
	u8 type;
};

struct Chunk {
	Block blocks[CHUNK_X][CHUNK_Z][CHUNK_Y];
	vec3 world_pos;
};

struct Renderer {
	Pipeline render_pipeline;
	Pipeline cull_pipeline;
	Pipeline cull_pass_pipeline;
	VkCommandBuffer cmdbuf;
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

struct Camera {
	mat4 proj_matrix;
	mat4 view_matrix;

	vec3 front;

	float width;
	float height;
};

struct Player {
	vec3 position;
	vec3 velocity;
	vec3 acceleration;

	float yaw;
	float pitch;

	b32 on_ground;

	Camera camera;
};

global Chunk chunks[WORLD_CHUNK_COUNT_X][WORLD_CHUNK_COUNT_Z][WORLD_CHUNK_COUNT_Y];

global u32 block_textures_map[BLOCK_COUNT][6] = {
	{},
	{TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT},
	{TEXTURE_GRASS_TOP, TEXTURE_DIRT, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE},
	{TEXTURE_OAK_LOG_TOP, TEXTURE_OAK_LOG_TOP, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE},
	{TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE},
	{TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE},
	{TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS},
};

global const Block air_block = { BLOCK_AIR };

Player CreatePlayer() {
	Player result = {};

	result.position = vec3(10.0, 14.0f, 10.0f);
	result.velocity = vec3(0);
	result.acceleration = vec3(0);
	result.yaw = 125.0f;
	result.pitch = 0;
	result.on_ground = 1;

	return result;
}

void ResizePlayerCamera(Camera *c, float w, float h) {
	c->width = w;
	c->height = h;

    mat4 proj_matrix = Perspective(PI32 / 3.0f, c->width / c->height, 0.01f, 1000.f);
	c->proj_matrix = proj_matrix;
}

void UploadPlayerCameraMatrices(Player *p, Renderer *r, VkCommandBuffer cmdbuf) {
	Camera *c = &p->camera;

	vec3 pos = p->position + vec3(0, 1.6f, 0);
	mat4 view_matrix = LookAt(pos, pos + c->front, vec3(0, 1, 0));

	c->view_matrix = view_matrix;

	Globals globals = { c->proj_matrix, view_matrix };
    UpdateRendererBuffer(r->globals_buffer, sizeof(globals), &globals, cmdbuf);
}

void UpdatePlayer(Player *p, float df) {
    Int2 mouse_delta = GetMouseDeltaPosition();

	if (mouse_delta.x != 0 || mouse_delta.y != 0) {
		p->yaw += mouse_delta.x * 0.01f;
		p->pitch -= mouse_delta.y * 0.01f;

		p->yaw = FMod(p->yaw, 360);
		p->pitch = Clamp(p->pitch, -89.0f, 89.0f);

		vec3 front;
		front.x = Cos(ToRadians(p->yaw)) * Cos(ToRadians(p->pitch));
		front.y = Sin(ToRadians(p->pitch));
		front.z = Sin(ToRadians(p->yaw)) * Cos(ToRadians(p->pitch));
		front = Normalize(front);

		p->camera.front = front;
	}

	p->acceleration = vec3(0);

	vec3 front = p->camera.front;
	float speed = df;
	if (IsKeyDown(KEY_W)) {
		p->acceleration += front * speed;
	}
	if (IsKeyDown(KEY_S)) {
		p->acceleration -= front * speed;
	}
	if (IsKeyDown(KEY_A)) {
		vec3 right = Cross(front, vec3(0, 1, 0));
		p->acceleration -= right * speed;
	}
	if (IsKeyDown(KEY_D)) {
		vec3 right = Cross(front, vec3(0, 1, 0));
		p->acceleration += right * speed;
	}
	p->acceleration.y = -0.01f;
	if (IsKeyDown(KEY_SPACE) && p->on_ground) {
		p->velocity.y = 0.2f;
		p->on_ground = 0;
	}

	p->velocity += p->acceleration;
	p->velocity *= 0.9;
	p->position += p->velocity;

	if (p->position.y <= 14) {
		p->position.y = 14;
		p->velocity.y = 0;
		p->on_ground = 1;
	}
}

Renderer CreateRenderer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf,
	VkFormat color_format, VkFormat depth_format) {
	Renderer result = {};

	// Graphics pipeline
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, 0}
	};

	Shader shaders[] = {
		{"Assets/Shaders/Core-Vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Core-Frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	GraphicsPipelineOptions options = {};
	options.bindings = bindings;
	options.bindings_count = ArrayCount(bindings);
	options.color_formats = &color_format;
	options.color_formats_count = 1;
	options.depth_format = depth_format;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_BACK_BIT;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_test = VK_TRUE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);

	result.render_pipeline = CreateGraphicsPipeline(&options);

	// Culling pipeline
	VkDescriptorSetLayoutBinding culling_bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0}
	};

	Shader culling_shader = {
		"Assets/Shaders/Culling-Comp.spv", VK_SHADER_STAGE_COMPUTE_BIT
	};

	result.cull_pipeline = CreateComputePipeline(culling_shader, culling_bindings, ArrayCount(culling_bindings));

	// Culling pass pipeline
	VkDescriptorSetLayoutBinding culling_pass_bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
	};

	Shader culling_pass_shader = {
		"Assets/Shaders/CullingPass-Comp.spv", VK_SHADER_STAGE_COMPUTE_BIT
	};

	result.cull_pass_pipeline = CreateComputePipeline(culling_pass_shader, culling_pass_bindings, ArrayCount(culling_pass_bindings));

	result.cmdbuf = cmdbuf;

	result.instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.instance_staging_buffer = CreateStagingBuffer(sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.culled_instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	result.culled_counter_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(u32), 0);

	result.frustum_info_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrustumInfo), 0);

	VkDrawIndexedIndirectCommand indirect_cmd = {6, 0, 0, 0, 0};
	result.indirect_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, sizeof(VkDrawIndirectCommand) + sizeof(u32), &indirect_cmd);

	Globals globals = {};
	result.globals_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(globals), &globals);

	return result;
}

void DestroyRenderer(Renderer *r) {
	DestroyBuffer(r->instance_buffer);
	DestroyStagingBuffer(r->instance_staging_buffer);
	DestroyBuffer(r->culled_instance_buffer);
	DestroyBuffer(r->culled_counter_buffer);
	DestroyBuffer(r->frustum_info_buffer);
	DestroyBuffer(r->indirect_buffer);
	DestroyBuffer(r->globals_buffer);

	DestroyPipeline(r->render_pipeline);
	DestroyPipeline(r->cull_pipeline);
	DestroyPipeline(r->cull_pass_pipeline);
}

Block GetBlock(int x, int y, int z) {
	if_unlikely(x < 0 || y < 0 || z < 0) {
		return air_block;
	}

	int cx = x / CHUNK_X;
	int cy = y / CHUNK_Y;
	int cz = z / CHUNK_Z;

	if_unlikely(cx >= WORLD_CHUNK_COUNT_X || cy >= WORLD_CHUNK_COUNT_Y || cz >= WORLD_CHUNK_COUNT_Z) {
		return air_block;
	}

	int bx = x % CHUNK_X;
	int by = y % CHUNK_Y;
	int bz = z % CHUNK_Z;
	
	Chunk *c = &chunks[cx][cz][cy];
	return c->blocks[bx][bz][by];
}

u32 UpdateBlockInstances(Renderer *r, Camera *camera, VkCommandBuffer cmdbuf) {
	InstanceData *instance_data = (InstanceData *) r->instance_staging_buffer.allocation_info.pMappedData;

	u32 instance_count = 0;
	
	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = &chunks[cx][cz][cy];

				for (int x = 0; x < CHUNK_X; ++x) {
					int wx = c->world_pos.x + x;
					for (int z = 0; z < CHUNK_Z; ++z) {
						int wz = c->world_pos.z + z;
						for (int y = 0; y < CHUNK_Y; ++y) {
							int wy = c->world_pos.y + y;
							Block b = c->blocks[x][z][y];

							if (b.type != BLOCK_AIR) {
								Block above = GetBlock(wx, wy + 1, wz);
								Block below = GetBlock(wx, wy - 1, wz);
								Block west = GetBlock(wx - 1, wy, wz);
								Block east = GetBlock(wx + 1, wy, wz);
								Block north = GetBlock(wx, wy, wz + 1);
								Block south = GetBlock(wx, wy, wz - 1);

								vec3 pos = c->world_pos + vec3(x, y, z);
								u32 *tex = block_textures_map[b.type];

								if (above.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_TOP, tex[SIDE_TOP]};
								}
								if (below.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_BOT, tex[SIDE_BOT]};
								}
								if (west.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_WEST, tex[SIDE_WEST]};
								}
								if (east.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_EAST, tex[SIDE_EAST]};
								}
								if (north.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_NORTH, tex[SIDE_NORTH]};
								}
								if (south.type == BLOCK_AIR) {
									*(instance_data + instance_count++) = {pos, SIDE_SOUTH, tex[SIDE_SOUTH]};
								}
							}
						}
					}
				}
			}
		}
	}

	if (instance_count == 0) {
		return 0;
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

	BindPipeline(&r->cull_pipeline, cmdbuf);

	VkDeviceSize buffer_size = instance_count * sizeof(InstanceData);
	BindBuffer(&r->cull_pipeline, 0, &r->instance_buffer, buffer_size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(&r->cull_pipeline, 1, &r->culled_instance_buffer, buffer_size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(&r->cull_pipeline, 2, &r->culled_counter_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(&r->cull_pipeline, 3, &r->frustum_info_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	u32 group_count = (instance_count + 63) / 64;
	vkCmdDispatch(cmdbuf, group_count, 1, 1);

	VkBufferMemoryBarrier2 cull_pass_barrier = CreateBufferBarrier(
		r->culled_counter_buffer.handle, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT
	);
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, &cull_pass_barrier, 1);

	BindPipeline(&r->cull_pass_pipeline, cmdbuf);
	BindBuffer(&r->cull_pass_pipeline, 0, &r->culled_counter_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(&r->cull_pass_pipeline, 1, &r->indirect_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	vkCmdDispatch(cmdbuf, 1, 1, 1);

	VkBufferMemoryBarrier2 cull_barrier = CreateBufferBarrier(
		r->indirect_buffer.handle, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT
	);
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, &cull_barrier, 1);

	return instance_count;
}

void Render(Renderer *r, TextureArray *textures, VkCommandBuffer cmdbuf, u32 instance_count) {
	if (instance_count == 0) {
		return;
	}

	BindPipeline(&r->render_pipeline, cmdbuf);
	BindBuffer(&r->render_pipeline, 0, &r->globals_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	BindBuffer(&r->render_pipeline, 1, &r->culled_instance_buffer, instance_count * sizeof(InstanceData), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindTextureArray(&r->render_pipeline, 2, textures);

	vkCmdDrawIndirect(cmdbuf, r->indirect_buffer.handle, 0, 1, sizeof(VkDrawIndirectCommand));
}

void LoadTextures(TextureArray *textures, VkCommandPool cmdpool) {
	LoadTextureAtSlot(textures, TEXTURE_DIRT, "Assets/Textures/dirt.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_GRASS_SIDE, "Assets/Textures/grass_side.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_GRASS_TOP, "Assets/Textures/grass_top.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_OAK_LOG_SIDE, "Assets/Textures/log_oak.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_OAK_LOG_TOP, "Assets/Textures/log_oak_top.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_STONE, "Assets/Textures/stone.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_COBBLE_STONE, "Assets/Textures/cobblestone.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_STONE_BRICKS, "Assets/Textures/stone_bricks.png", cmdpool);
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

	Player player = CreatePlayer();
	ResizePlayerCamera(&player.camera, float(swapchain.width), float(swapchain.height));

	{
		VkCommandBuffer temp_cmdbuf = BeginTempCommandBuffer(cmdpool);

		UploadPlayerCameraMatrices(&player, &renderer, temp_cmdbuf);

		EndTempCommandBuffer(cmdpool, temp_cmdbuf);
	}

	TextureArray textures = CreateTextureArray(TEXTURE_COUNT);
	LoadTextures(&textures, cmdpool);

	VkPhysicalDeviceProperties pdev_props;
	vkGetPhysicalDeviceProperties(GetPhysicalDevice(), &pdev_props);
	Assert(pdev_props.limits.timestampComputeAndGraphics);

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = &chunks[cx][cz][cy];
				c->world_pos = vec3(cx * CHUNK_X, cy * CHUNK_Y, cz * CHUNK_Z);

				for (int x = 0; x < CHUNK_X; ++x) {
					for (int z = 0; z < CHUNK_Z; ++z) {
						for (int y = 0; y < CHUNK_Y - 2; ++y) {
							if (y < 11) {
								c->blocks[x][z][y].type = BLOCK_STONE;
							} else if (y < 13) {
								c->blocks[x][z][y].type = BLOCK_DIRT;
							} else {
								c->blocks[x][z][y].type = BLOCK_GRASS;
							}
						}
					}
				}
			}
		}
	}

	Chunk *c = &chunks[0][0][0];
	c->blocks[14][14][14] = { BLOCK_STONE_BRICKS };
	c->blocks[14][14][15] = { BLOCK_STONE_BRICKS };

	double cpu_time_avg = 0.0;
	double gpu_time_avg = 0.0;
	u64 triangles = 0.0;
	double triangles_per_sec = 0.0;

	u64 last_frame_time = GetTimeNowUs();

	b32 cursor_locked = 1;
	SetCursorToNone(&window);

	while (window.running) {
		u64 cpu_time_begin = GetTimeNowUs();

		UpdateWindow(&window);

		u64 now_time = GetTimeNowUs();
		u64 delta = now_time - last_frame_time;
		last_frame_time = now_time;
		float df = float(delta) / 1000000;

		if (IsKeyDown(KEY_ESCAPE)) {
			cursor_locked ^= 1;
			if (cursor_locked) {
				SetCursorToNone(&window);
			} else {
				SetCursorToArrow();
			}
		}

		if (IsKeyDown(KEY_Q)) {
			window.running = false;
		}

		UpdatePlayer(&player, df);

		if (window.resized) {
			UpdateSwapchain(&swapchain, cmdpool, 1);

			DestroyImage(color_target);
			DestroyImage(depth_target);
			color_target = CreateImage(swapchain.width, swapchain.height, swapchain.format.format, 1,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
			depth_target = CreateImage(swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT, 1,
				VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			VkCommandBuffer temp_cmdbuf = BeginTempCommandBuffer(cmdpool);
			ResizePlayerCamera(&player.camera, float(swapchain.width), float(swapchain.height));
			EndTempCommandBuffer(cmdpool, temp_cmdbuf);
		}

		if (!AcquireSwapchain(&swapchain, cmdpool, cmdbuf, color_target, depth_target)) {
			continue;
		}

		UploadPlayerCameraMatrices(&player, &renderer, cmdbuf);

		u32 instance_count = UpdateBlockInstances(&renderer, &player.camera, cmdbuf);

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

	DestroyTextureArray(&textures);
	DestroyRenderer(&renderer);

	DestroyImage(depth_target);
	DestroyImage(color_target);
	FreeCommandBuffers(cmdpool, &cmdbuf, 1);
	DestroySwapchain(&swapchain);
	DestroyCommandPool(cmdpool);

	ReleaseVulkan();

	DestroyWindow(&window);
}