#include "Renderer.h"

global u32 block_textures_map[BLOCK_COUNT][6] = {
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT, TEXTURE_DIRT},
	{TEXTURE_GRASS_TOP, TEXTURE_DIRT, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE, TEXTURE_GRASS_SIDE},
	{TEXTURE_OAK_LOG_TOP, TEXTURE_OAK_LOG_TOP, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE, TEXTURE_OAK_LOG_SIDE},
	{TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE, TEXTURE_STONE},
	{TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE, TEXTURE_COBBLE_STONE},
	{TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS, TEXTURE_STONE_BRICKS},
};

global Renderer renderer;

internal void LoadTextures(TextureArray *textures, VkCommandPool cmdpool) {
	LoadTextureAtSlot(textures, TEXTURE_DIRT, "Assets/Textures/dirt.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_GRASS_SIDE, "Assets/Textures/grass_side.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_GRASS_TOP, "Assets/Textures/grass_top.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_OAK_LOG_SIDE, "Assets/Textures/log_oak.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_OAK_LOG_TOP, "Assets/Textures/log_oak_top.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_STONE, "Assets/Textures/stone.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_COBBLE_STONE, "Assets/Textures/cobblestone.png", cmdpool);
	LoadTextureAtSlot(textures, TEXTURE_STONE_BRICKS, "Assets/Textures/stone_bricks.png", cmdpool);
}

void CreateSkyRenderPass(VkFormat color_format, VkFormat depth_format, VkCommandPool cmdpool, RenderPass *pass) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
	};

	Shader shaders[] = {
		{"Assets/Shaders/Sky.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Sky.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	GraphicsPipelineOptions options = {};
	options.color_formats = &color_format;
	options.color_formats_count = 1;
	options.depth_format = depth_format;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_NONE;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_read = VK_FALSE;
	options.depth_write = VK_FALSE;
	options.blend = VK_FALSE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);

	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	pass->pipeline = CreateGraphicsPipeline(&options, layout);
	pass->desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);
}

void DestroySkyRenderPass(RenderPass *pass) {
	DestroyPipeline(pass->pipeline);
	DestroyDescriptorSet(&pass->desc_set);
}

void CreateSolidRenderPass(VkFormat color_format, VkFormat depth_format, VkCommandPool cmdpool, RenderPass *pass) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, TEXTURE_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0}
	};

	Shader shaders[] = {
		{"Assets/Shaders/Core.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Core.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	GraphicsPipelineOptions options = {};
	options.color_formats = &color_format;
	options.color_formats_count = 1;
	options.depth_format = depth_format;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_BACK_BIT;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_read = VK_TRUE;
	options.depth_write = VK_TRUE;
	options.blend = VK_FALSE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);

	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	pass->pipeline = CreateGraphicsPipeline(&options, layout);
	pass->desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);

	VkDrawIndirectCommand indirect_cmd = {6, 0, 0, 0};
	pass->instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->instance_staging_buffer = CreateStagingBuffer(sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->culled_instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->indirect_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, sizeof(VkDrawIndirectCommand), &indirect_cmd);
}

void CreateWaterRenderPass(VkFormat color_format, VkFormat depth_format, VkCommandPool cmdpool, RenderPass *pass) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
		{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
	};

	Shader shaders[] = {
		{"Assets/Shaders/Water.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Water.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	VkPushConstantRange time_pc = {};
    time_pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	time_pc.offset = 0;
    time_pc.size = sizeof(float);

	GraphicsPipelineOptions options = {};
	options.color_formats = &color_format;
	options.color_formats_count = 1;
	options.depth_format = depth_format;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_BACK_BIT;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_read = VK_TRUE;
	options.depth_write = VK_FALSE;
	options.blend = VK_TRUE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);
	options.push_contants = &time_pc;
	options.push_constants_count = 1;

	VkDrawIndirectCommand indirect_cmd = {6, 0, 0, 0};
	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	pass->pipeline = CreateGraphicsPipeline(&options, layout);
	pass->desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);
	pass->instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->instance_staging_buffer = CreateStagingBuffer(sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->culled_instance_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(InstanceData) * MAX_INSTANCE_COUNT, 0);
	pass->indirect_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, sizeof(VkDrawIndirectCommand), &indirect_cmd);
}

void DestroyRenderPass(RenderPass *pass) {
	DestroyPipeline(pass->pipeline);
	DestroyDescriptorSet(&pass->desc_set);
	DestroyBuffer(pass->instance_buffer);
	DestroyStagingBuffer(pass->instance_staging_buffer);
	DestroyBuffer(pass->culled_instance_buffer);
	DestroyBuffer(pass->indirect_buffer);
}

void CreateShadowRenderPass(VkCommandBuffer cmdbuf, VkCommandPool cmdpool, ShadowPass *pass) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0 },
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, 0},
	};

	Shader shaders[] = {
		{"Assets/Shaders/Shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	GraphicsPipelineOptions options = {};
	options.color_formats = 0;
	options.color_formats_count = 0;
	options.depth_format = VK_FORMAT_D32_SFLOAT;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_BACK_BIT;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_read = VK_TRUE;
	options.depth_write = VK_TRUE;
	options.blend = VK_FALSE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);

	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	pass->pipeline = CreateGraphicsPipeline(&options, layout);
	pass->desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);

	VkSamplerCreateInfo shadow_sampler = {};
	shadow_sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadow_sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadow_sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadow_sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	pass->shadow_map = CreateTexture(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadow_sampler
	);

	VkImageMemoryBarrier2 shadow_map_barrier = CreateImageBarrier(pass->shadow_map.image.handle, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
	PipelineImageBarriers(cmdbuf, 0, &shadow_map_barrier, 1);
	pass->light_space_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(mat4), 0);
}

void DestroyShadowPass(ShadowPass *pass) {
	DestroyDescriptorSet(&pass->desc_set);
	DestroyPipeline(pass->pipeline);
	DestroyTexture(pass->shadow_map);
	DestroyBuffer(pass->light_space_buffer);
}

void CreateCullPass(VkCommandPool cmdpool, CullPass *pass) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
		{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0}
	};

	Shader culling_shader = {
		"Assets/Shaders/Culling.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	pass->pipeline = CreateComputePipeline(culling_shader, layout);
	pass->desc_sets[0] = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);
	pass->desc_sets[1] = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);

	pass->frustum_info_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrustumInfo), 0);
}

void DestroyCullPass(CullPass *pass) {
	DestroyDescriptorSet(&pass->desc_sets[0]);
	DestroyDescriptorSet(&pass->desc_sets[1]);
	DestroyPipeline(pass->pipeline);
	DestroyBuffer(pass->frustum_info_buffer);
}

void CreatePostprocess(VkCommandPool cmdpool, Postprocess *post) {
	VkDescriptorSetLayoutBinding bindings[] = {
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0}
	};

	Shader shaders[] = {
		{"Assets/Shaders/Post.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"Assets/Shaders/Post.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	GraphicsPipelineOptions options = {};
	options.color_formats = &format;
	options.color_formats_count = 1;
	options.depth_format = VK_FORMAT_UNDEFINED;
	options.polygon_mode = VK_POLYGON_MODE_FILL;
	options.cull_mode = VK_CULL_MODE_NONE;
	options.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	options.depth_read = VK_FALSE;
	options.depth_write = VK_FALSE;
	options.blend = VK_FALSE;
	options.shaders = shaders;
	options.shaders_count = ArrayCount(shaders);

	VkDescriptorSetLayout layout = CreateDescriptorSetLayout(bindings, ArrayCount(bindings));
	post->desc_set = CreateDescriptorSet(bindings, ArrayCount(bindings), layout);
	post->pipeline = CreateGraphicsPipeline(&options, layout);
}

void DestroyPostprocess(Postprocess *post) {
	DestroyPipeline(post->pipeline);
	DestroyDescriptorSet(&post->desc_set);
}

void InitRenderer(VkCommandPool cmdpool, VkCommandBuffer cmdbuf,
	VkFormat color_format, VkFormat depth_format) {

	CreateSkyRenderPass(color_format, depth_format, cmdpool, &renderer.sky_pass);
	CreateSolidRenderPass(color_format, depth_format, cmdpool, &renderer.solid_pass);
	CreateWaterRenderPass(color_format, depth_format, cmdpool, &renderer.water_pass);
	CreateShadowRenderPass(cmdbuf, cmdpool, &renderer.shadow_pass);
	CreateCullPass(cmdpool, &renderer.cull_pass);
	CreatePostprocess(cmdpool, &renderer.post_process);

	Globals globals = {};
	renderer.globals_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(globals), &globals);

	SkyUniform sky_uniform = {};
	renderer.sky_buffer = CreateBuffer(cmdpool, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(sky_uniform), &sky_uniform);

	renderer.textures = CreateTextureArray(TEXTURE_COUNT);
	LoadTextures(&renderer.textures, cmdpool);

	renderer.noise_texture = LoadTextureFromFile("Assets/Textures/noise.png", cmdpool);
	renderer.water_texture1 = LoadTextureFromFile("Assets/Textures/water1.png", cmdpool);
	renderer.water_texture2 = LoadTextureFromFile("Assets/Textures/water2.png", cmdpool);
}

void DestroyRenderer() {
	DestroyRenderPass(&renderer.sky_pass);
	DestroyRenderPass(&renderer.solid_pass);
	DestroyRenderPass(&renderer.water_pass);
	DestroyShadowPass(&renderer.shadow_pass);
	DestroyCullPass(&renderer.cull_pass);
	DestroyBuffer(renderer.globals_buffer);
	DestroyBuffer(renderer.sky_buffer);
	DestroyTextureArray(&renderer.textures);
	DestroyTexture(renderer.noise_texture);
	DestroyTexture(renderer.water_texture1);
	DestroyTexture(renderer.water_texture2);
}

internal void Cull(CullCall *cull, VkCommandBuffer cmdbuf) {
	FrustumInfo frustum_info = {};
	frustum_info.view_matrix = cull->view_matrix;
	ExtractFrustumPlanes(cull->proj_matrix, frustum_info.planes);
	frustum_info.instance_count = cull->instance_count;

	UpdateRendererBuffer(renderer.cull_pass.frustum_info_buffer, sizeof(frustum_info), &frustum_info, cmdbuf);

	VkDrawIndirectCommand indirect_cmd = {6, 0, 0, 0};
	UpdateRendererBuffer(cull->indirect_buffer, sizeof(indirect_cmd), &indirect_cmd, cmdbuf);

	Pipeline *pipeline = &renderer.cull_pass.pipeline;
	DescriptorSet *desc_set = &renderer.cull_pass.desc_sets[cull->desc_set_index];
	BindPipeline(pipeline, cmdbuf);
	BindDescriptorSet(desc_set, pipeline, cmdbuf);

	VkDeviceSize buffer_size = cull->instance_count * sizeof(InstanceData);
	BindBuffer(desc_set, 0, &cull->instance_buffer, buffer_size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(desc_set, 1, &cull->culled_instance_buffer, buffer_size, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(desc_set, 2, &cull->indirect_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	BindBuffer(desc_set, 3, &renderer.cull_pass.frustum_info_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	u32 group_count = (cull->instance_count + 63) / 64;
	vkCmdDispatch(cmdbuf, group_count, 1, 1);

	VkBufferMemoryBarrier2 cull_barriers[] = { CreateBufferBarrier(
		cull->indirect_buffer.handle, VK_WHOLE_SIZE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
		CreateBufferBarrier(
		cull->culled_instance_buffer.handle, cull->instance_count * sizeof(InstanceData), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT) };
    PipelineBufferBarriers(cmdbuf, VK_DEPENDENCY_DEVICE_GROUP_BIT, cull_barriers, 2);
}

void Cull(Player *p, BlockInstanceCounts instance_counts, VkCommandBuffer cmdbuf) {
	CullCall solid_cull_call = {};
	solid_cull_call.instance_buffer = renderer.solid_pass.instance_buffer;
	solid_cull_call.culled_instance_buffer = renderer.solid_pass.culled_instance_buffer;
	solid_cull_call.indirect_buffer = renderer.solid_pass.indirect_buffer;
	solid_cull_call.proj_matrix = p->camera.proj_matrix;
	solid_cull_call.view_matrix = p->camera.view_matrix;
	solid_cull_call.desc_set_index = 0;
	solid_cull_call.instance_count = instance_counts.solid;
	Cull(&solid_cull_call, cmdbuf);

	CullCall water_cull_call = {};
	water_cull_call.instance_buffer = renderer.water_pass.instance_buffer;
	water_cull_call.culled_instance_buffer = renderer.water_pass.culled_instance_buffer;
	water_cull_call.indirect_buffer = renderer.water_pass.indirect_buffer;
	water_cull_call.proj_matrix = p->camera.proj_matrix;
	water_cull_call.view_matrix = p->camera.view_matrix;
	water_cull_call.desc_set_index = 1;
	water_cull_call.instance_count = instance_counts.water;
	Cull(&water_cull_call, cmdbuf);
}

void RenderShadow(VkCommandBuffer cmdbuf, BlockInstanceCounts instance_counts) {
	if (instance_counts.solid == 0) {
		return;
	}

	VkClearDepthStencilValue depth_clear = { 1.0f, 0 };

	VkRenderingAttachmentInfo depth_attachment = {};
	depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_attachment.imageView = renderer.shadow_pass.shadow_map.image.view;
	depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.clearValue.depthStencil = depth_clear;

	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea.extent.width = SHADOW_MAP_WIDTH;
	rendering_info.renderArea.extent.height = SHADOW_MAP_HEIGHT;
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = 0;
	rendering_info.pColorAttachments = 0;
	rendering_info.pDepthAttachment = &depth_attachment;
	vkCmdBeginRendering(cmdbuf, &rendering_info);

	VkViewport viewport = {};
	viewport.width = SHADOW_MAP_WIDTH;
	viewport.height = SHADOW_MAP_HEIGHT;
	viewport.maxDepth = 1;

	VkRect2D scissor = {};
	scissor.extent.width = SHADOW_MAP_WIDTH;
	scissor.extent.height = SHADOW_MAP_HEIGHT;

	vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

	ShadowPass *pass = &renderer.shadow_pass;
	RenderPass *solid_pass = &renderer.solid_pass;
	Pipeline *pipeline = &pass->pipeline;
	DescriptorSet *desc_set = &pass->desc_set;
	BindPipeline(pipeline, cmdbuf);
	BindDescriptorSet(desc_set, pipeline, cmdbuf);

	BindBuffer(desc_set, 0, &pass->light_space_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	BindBuffer(desc_set, 1, &solid_pass->culled_instance_buffer, instance_counts.solid * sizeof(InstanceData), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	vkCmdDrawIndirect(cmdbuf, solid_pass->indirect_buffer.handle, 0, 1, sizeof(VkDrawIndirectCommand));

	vkCmdEndRendering(cmdbuf);
}

void Render(Swapchain *swapchain, VkImageView color_view, VkImageView depth_view, VkCommandBuffer cmdbuf,
	BlockInstanceCounts instance_counts, float time) {
	VkClearColorValue clear_color = {};
	VkClearDepthStencilValue depth_clear = { 1.0f, 0 };

	VkRenderingAttachmentInfo color_attachment = {};
	color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment.imageView = color_view;
	color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.clearValue.color = clear_color;

	VkRenderingAttachmentInfo depth_attachment = {};
	depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depth_attachment.imageView = depth_view;
	depth_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.clearValue.depthStencil = depth_clear;

	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea.extent.width = swapchain->width;
	rendering_info.renderArea.extent.height = swapchain->height;
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment;
	rendering_info.pDepthAttachment = &depth_attachment;
	vkCmdBeginRendering(cmdbuf, &rendering_info);

	VkViewport viewport = {};
	viewport.width = float(swapchain->width);
	viewport.height = float(swapchain->height);
	viewport.maxDepth = 1;

	VkRect2D scissor = {};
	scissor.extent.width = swapchain->width;
	scissor.extent.height = swapchain->height;

	vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

	// sky
	{
		RenderPass *pass = &renderer.sky_pass;
		Pipeline *pipeline = &pass->pipeline;
		DescriptorSet *desc_set = &pass->desc_set;
		BindPipeline(pipeline, cmdbuf);
		BindDescriptorSet(desc_set, pipeline, cmdbuf);

		BindBuffer(desc_set, 0, &renderer.sky_buffer, sizeof(SkyUniform), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		vkCmdDraw(cmdbuf, 6, 1, 0, 0);
	}

	if (instance_counts.solid > 0) {
		RenderPass *pass = &renderer.solid_pass;
		Pipeline *pipeline = &pass->pipeline;
		DescriptorSet *desc_set = &pass->desc_set;
		BindPipeline(pipeline, cmdbuf);
		BindDescriptorSet(desc_set, pipeline, cmdbuf);

		BindBuffer(desc_set, 0, &renderer.globals_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		BindBuffer(desc_set, 1, &pass->culled_instance_buffer, instance_counts.solid * sizeof(InstanceData), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		BindTextureArray(desc_set, 2, &renderer.textures);
		BindTexture(desc_set, 3, renderer.shadow_pass.shadow_map);
		BindTexture(desc_set, 4, renderer.noise_texture);

		vkCmdDrawIndirect(cmdbuf, pass->indirect_buffer.handle, 0, 1, sizeof(VkDrawIndirectCommand));
	}

	if (instance_counts.water > 0) {
		RenderPass *pass = &renderer.water_pass;
		Pipeline *pipeline = &pass->pipeline;
		DescriptorSet *desc_set = &pass->desc_set;
		BindPipeline(pipeline, cmdbuf);
		BindDescriptorSet(desc_set, pipeline, cmdbuf);

		BindBuffer(desc_set, 0, &renderer.globals_buffer, VK_WHOLE_SIZE, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		BindBuffer(desc_set, 1, &pass->culled_instance_buffer, instance_counts.water * sizeof(InstanceData), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		BindTexture(desc_set, 2, renderer.shadow_pass.shadow_map);
		BindTexture(desc_set, 3, renderer.noise_texture);
		BindTexture(desc_set, 4, renderer.water_texture1);
		BindTexture(desc_set, 5, renderer.water_texture2);

		vkCmdPushConstants(cmdbuf, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);

		vkCmdDrawIndirect(cmdbuf, pass->indirect_buffer.handle, 0, 1, sizeof(VkDrawIndirectCommand));
	}

	vkCmdEndRendering(cmdbuf);
}

void DoPostprocessing(Swapchain *swapchain, Texture render_target, Image swapchain_target, VkCommandBuffer cmdbuf) {
	Postprocess *post = &renderer.post_process;
	BindPipeline(&post->pipeline, cmdbuf);
	BindDescriptorSet(&post->desc_set, &post->pipeline, cmdbuf);

	VkClearColorValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderingAttachmentInfo color_attachment = {};
	color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	color_attachment.imageView = swapchain_target.view;
	color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.clearValue.color = clear_color;

	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	rendering_info.renderArea.extent.width = swapchain->width;
	rendering_info.renderArea.extent.height = swapchain->height;
	rendering_info.layerCount = 1;
	rendering_info.colorAttachmentCount = 1;
	rendering_info.pColorAttachments = &color_attachment;
	rendering_info.pDepthAttachment = 0;
	vkCmdBeginRendering(cmdbuf, &rendering_info);

	VkViewport viewport = {};
	viewport.width = float(swapchain->width);
	viewport.height = float(swapchain->height);
	viewport.maxDepth = 1;

	VkRect2D scissor = {};
	scissor.extent.width = swapchain->width;
	scissor.extent.height = swapchain->height;

	vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

	BindTexture(&post->desc_set, 0, render_target);
	vkCmdDraw(cmdbuf, 6, 1, 0, 0);

	vkCmdEndRendering(cmdbuf);
}

internal mat4 UploadLightSpaceTransform(Player *p, VkCommandBuffer cmdbuf) {
	const float shadow_dist = 50.0f;
	const float shadow_range = 120.0f;

	vec3 player_eye = GetEyePos(p);

	vec3 light_dir = Normalize(vec3(-1.0f, -1.0f, -1.0f));
	vec3 light_pos = player_eye - light_dir * shadow_dist;
	mat4 light_view = LookAt(light_pos, player_eye, vec3(0, 1, 0));
	mat4 light_proj = Ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.1f, 1000.0f);
	mat4 light_vp = light_proj * light_view;

	UpdateRendererBuffer(renderer.shadow_pass.light_space_buffer, sizeof(mat4), &light_vp, cmdbuf);
	return light_vp;
}

internal void UploadPlayerCameraMatrices(Player *p, mat4 light_space_matrix, VkCommandBuffer cmdbuf) {
	Camera *c = &p->camera;

	vec3 pos = GetEyePos(p);
	mat4 view_matrix = LookAt(pos, pos + c->front, vec3(0, 1, 0));

	c->view_matrix = view_matrix;

	Globals globals = { c->proj_matrix, view_matrix, light_space_matrix, pos };
    UpdateRendererBuffer(renderer.globals_buffer, sizeof(globals), &globals, cmdbuf);

	SkyUniform sky_uniform = { Inverse(c->proj_matrix), Inverse(view_matrix), pos };
    UpdateRendererBuffer(renderer.sky_buffer, sizeof(sky_uniform), &sky_uniform, cmdbuf);
}

void UploadTransformations(Player *p, VkCommandBuffer cmdbuf) {
	mat4 light_vp = UploadLightSpaceTransform(p, cmdbuf);
	UploadPlayerCameraMatrices(p, light_vp, cmdbuf);
}

BlockInstanceCounts UpdateBlockInstances(VkCommandBuffer cmdbuf, BlockInstanceCounts prev_instance_counts) {
	if (!AnyChunkDirty()) {
		return prev_instance_counts;
	}

	ResetChunkDirtiness();

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = GetChunk(cx, cy, cz);
				if (!c->dirty) continue;

				// pass 1 - count instances
				u32 chunk_instance_count = 0;
				u32 chunk_water_instance_count = 0;
				for (int x = 0; x < CHUNK_X; ++x) {
					int wx = c->world_pos.x + x;
					for (int z = 0; z < CHUNK_Z; ++z) {
						int wz = c->world_pos.z + z;
						for (int y = 0; y < CHUNK_Y; ++y) {
							int wy = c->world_pos.y + y;
							Block b = c->blocks[x][z][y];

							if (b == BLOCK_AIR) continue;

							if (b != BLOCK_WATER) {
								if (GetBlock(wx, wy + 1, wz) <= BLOCK_WATER) chunk_instance_count++;
								if (GetBlock(wx, wy - 1, wz) <= BLOCK_WATER) chunk_instance_count++;
								if (GetBlock(wx - 1, wy, wz) <= BLOCK_WATER) chunk_instance_count++;
								if (GetBlock(wx + 1, wy, wz) <= BLOCK_WATER) chunk_instance_count++;
								if (GetBlock(wx, wy, wz + 1) <= BLOCK_WATER) chunk_instance_count++;
								if (GetBlock(wx, wy, wz - 1) <= BLOCK_WATER) chunk_instance_count++;
							} else {
								if (GetBlock(wx, wy + 1, wz) != BLOCK_WATER) chunk_water_instance_count++;
								if (GetBlock(wx, wy - 1, wz) != BLOCK_WATER) chunk_water_instance_count++;
								if (GetBlock(wx - 1, wy, wz) != BLOCK_WATER) chunk_water_instance_count++;
								if (GetBlock(wx + 1, wy, wz) != BLOCK_WATER) chunk_water_instance_count++;
								if (GetBlock(wx, wy, wz + 1) != BLOCK_WATER) chunk_water_instance_count++;
								if (GetBlock(wx, wy, wz - 1) != BLOCK_WATER) chunk_water_instance_count++;
							}
						}
					}
				}

				if (!c->cached_instance_data) {
					c->cached_instance_data = (InstanceData *) HeapAlloc(
						(chunk_instance_count + chunk_water_instance_count) * sizeof(InstanceData));
				} else {
					c->cached_instance_data =
						(InstanceData *) HeapRealloc(c->cached_instance_data,
							(chunk_instance_count + chunk_water_instance_count) * sizeof(InstanceData));
				}
				c->instance_count = chunk_instance_count;
				c->water_instance_count = chunk_water_instance_count;

				// pass 2 - fill instance data cache
				u32 idx = 0;
				u32 water_idx = chunk_instance_count;
				for (int x = 0; x < CHUNK_X; ++x) {
					int wx = c->world_pos.x + x;
					for (int z = 0; z < CHUNK_Z; ++z) {
						int wz = c->world_pos.z + z;
						for (int y = 0; y < CHUNK_Y; ++y) {
							int wy = c->world_pos.y + y;
							Block b = c->blocks[x][z][y];

							if (b == BLOCK_AIR) continue;

							vec3 pos = vec3(wx, wy, wz);
							u32 *tex = block_textures_map[b];

							if (b != BLOCK_WATER) {
								if (GetBlock(wx, wy + 1, wz) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_TOP, tex[SIDE_TOP] };
								}
								if (GetBlock(wx, wy - 1, wz) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_BOT, tex[SIDE_BOT] };
								}
								if (GetBlock(wx - 1, wy, wz) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_WEST, tex[SIDE_WEST] };
								}
								if (GetBlock(wx + 1, wy, wz) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_EAST, tex[SIDE_EAST] };
								}
								if (GetBlock(wx, wy, wz + 1) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_NORTH, tex[SIDE_NORTH] };
								}
								if (GetBlock(wx, wy, wz - 1) <= BLOCK_WATER) {
									c->cached_instance_data[idx++] = { pos, SIDE_SOUTH, tex[SIDE_SOUTH] };
								}
							} else {
								if (GetBlock(wx, wy + 1, wz) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_TOP, tex[SIDE_TOP] };
								}
								if (GetBlock(wx, wy - 1, wz) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_BOT, tex[SIDE_BOT] };
								}
								if (GetBlock(wx - 1, wy, wz) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_WEST, tex[SIDE_WEST] };
								}
								if (GetBlock(wx + 1, wy, wz) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_EAST, tex[SIDE_EAST] };
								}
								if (GetBlock(wx, wy, wz + 1) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_NORTH, tex[SIDE_NORTH] };
								}
								if (GetBlock(wx, wy, wz - 1) != BLOCK_WATER) {
									c->cached_instance_data[water_idx++] = { pos, SIDE_SOUTH, tex[SIDE_SOUTH] };
								}
							}
						}
					}
				}

				c->dirty = 0;
			}
		}
	}

	RenderPass *solid_pass = &renderer.solid_pass;
	RenderPass *water_pass = &renderer.water_pass;
	InstanceData *instance_data = (InstanceData *) solid_pass->instance_staging_buffer.allocation_info.pMappedData;
	InstanceData *water_instance_data = (InstanceData *) water_pass->instance_staging_buffer.allocation_info.pMappedData;
	u32 instance_count = 0;
	u32 water_instance_count = 0;

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = GetChunk(cx, cy, cz);
				if (c->cached_instance_data) {
					if (c->instance_count > 0) {
						CopyMemory(instance_data + instance_count,
							c->cached_instance_data, c->instance_count * sizeof(InstanceData));
						instance_count += c->instance_count;
					}
					if (c->water_instance_count > 0) {
						CopyMemory(water_instance_data + water_instance_count,
							c->cached_instance_data + c->instance_count, c->water_instance_count * sizeof(InstanceData));
						water_instance_count += c->water_instance_count;
					}
				}
			}
		}
	}

    VkBufferCopy copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = instance_count * sizeof(InstanceData);
    vkCmdCopyBuffer(cmdbuf, solid_pass->instance_staging_buffer.handle, solid_pass->instance_buffer.handle, 1, &copy);
	
    copy = {};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = water_instance_count * sizeof(InstanceData);
    vkCmdCopyBuffer(cmdbuf, water_pass->instance_staging_buffer.handle, water_pass->instance_buffer.handle, 1, &copy);

	return { instance_count, water_instance_count };
}
