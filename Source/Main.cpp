#include <stdio.h>

#include "General.h"
#include "Window/Window.h"
#include "Graphics/NVulkan.h"
#include "Math/Vec.h"
#include "Math/Mat.h"
#include "Math/Quat.h"
#include "Math/NMath.h"

#include "World.h"
#include "MapGen.h"
#include "Renderer.h"
#include "Player.h"

void LoadTextures(TextureArray *textures, VkCommandPool cmdpool) {
	LoadTextureAtSlot(textures, TEXTURE_WATER, "Assets/Textures/water.png", cmdpool);
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

	VkCommandBuffer init_cmdbuf = BeginTempCommandBuffer(cmdpool);
	InitRenderer(cmdpool, init_cmdbuf, color_target.format, depth_target.format);

	Player player = CreatePlayer();
	ResizePlayerCamera(&player.camera, float(swapchain.width), float(swapchain.height));

	UploadTransformations(&player, init_cmdbuf);
	EndTempCommandBuffer(cmdpool, init_cmdbuf);

	TextureArray textures = CreateTextureArray(TEXTURE_COUNT);
	LoadTextures(&textures, cmdpool);

	VkPhysicalDeviceProperties pdev_props;
	vkGetPhysicalDeviceProperties(GetPhysicalDevice(), &pdev_props);
	Assert(pdev_props.limits.timestampComputeAndGraphics);

	GenerateMap();
	// GenerateMapImage();

	double cpu_time_avg = 0.0;
	double gpu_time_avg = 0.0;
	u64 triangles = 0.0;
	double triangles_per_sec = 0.0;

	u64 last_frame_time = GetTimeNowUs();

	b32 cursor_locked = 1;
	SetCursorToNone(&window);

	BlockInstanceCounts prev_instance_counts = {};

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

		UploadTransformations(&player, cmdbuf);

		BlockInstanceCounts instance_counts = UpdateBlockInstances(cmdbuf, prev_instance_counts);
		prev_instance_counts = instance_counts;

		Cull(&player, instance_counts, cmdbuf);

		vkCmdResetQueryPool(cmdbuf, pipeline_queries, 0, 1);
		vkCmdBeginQuery(cmdbuf, pipeline_queries, 0, 0);

		RenderShadow(cmdbuf, instance_counts);
		Render(&textures, &swapchain, color_target.view, depth_target.view, cmdbuf, instance_counts);

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
	DestroyRenderer();

	DestroyImage(depth_target);
	DestroyImage(color_target);
	FreeCommandBuffers(cmdpool, &cmdbuf, 1);
	DestroySwapchain(&swapchain);
	DestroyCommandPool(cmdpool);

	ReleaseVulkan();

	DestroyWindow(&window);
}