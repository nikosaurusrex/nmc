#include "MapGen.h"
#include "World.h"

#include "Math/NMath.h"
#include "Platform/Platform.h"

#include "ThirdParty/stb_image_write.h"
#include "ThirdParty/stb_perlin.h"

struct GMRandom {
	u32 state;
};

void GenerateMap() {
	int width = CHUNK_X * WORLD_CHUNK_COUNT_X;
	int height = CHUNK_Z * WORLD_CHUNK_COUNT_Z;

	u8 *heightmap = (u8 *) HeapAlloc(width * height);

	float sx = 1.0f / width;
	float sz = 1.0f / height;

	for (int z = 0; z < height; ++z) {
		for (int x = 0; x < width; ++x) {
			float height = stb_perlin_fbm_noise3(x * sx, 0.0f, z * sz, 2, 0.5f, 6);
			height = (height + 1.0f) * 0.5f;
			heightmap[z * width + x] = u8(height * 50);
		}
	}

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = GetChunk(cx, cy, cz);
				c->world_pos = vec3(cx * CHUNK_X, cy * CHUNK_Y, cz * CHUNK_Z);
			}
		}
	}

	for (int cx = 0; cx < WORLD_CHUNK_COUNT_X; ++cx) {
		for (int cz = 0; cz < WORLD_CHUNK_COUNT_Z; ++cz) {
			for (int cy = 0; cy < WORLD_CHUNK_COUNT_Y; ++cy) {
				Chunk *c = GetChunk(cx, cy, cz);
				for (int bx = 0; bx < CHUNK_X; ++bx) {
					int wx = c->world_pos.x + bx;

					for (int bz = 0; bz < CHUNK_Z; ++bz) {
						int wz = c->world_pos.z + bz;

						int height = heightmap[wz * width + wx];
						int ydiff = height - c->world_pos.y;
						if (ydiff > 0) {
							for (int i = 0; i < Min(ydiff, CHUNK_Y); ++i) {
								int wy = c->world_pos.y + i;

								Block block;
								int yd = height - wy;
								if (yd == 1) {
									block = BLOCK_GRASS;
								} else if (yd > 1 && yd <= 4) {
									block = BLOCK_DIRT;
								} else {
									block = BLOCK_STONE;
								}

								PlaceBlock(c, bx, i, bz, block);
							}
						}
					}
				}
			}
		}
	}

	HeapFree(heightmap);
}

/*
void GenerateMapImage() {
	int width = 256;
	int height = 256;
	u8 *data = (u8 *) HeapAlloc(width * height);

	float sx = 1.0f / width;
	float sz = 1.0f / height;

	for (int z = 0; z < height; ++z) {
		for (int x = 0; x < width; ++x) {
			float height = stb_perlin_fbm_noise3(x * sx, 0.0f, z * sz, 2, 0.5f, 6);
			height = (height + 1.0f) * 0.5f;
			data[z * width + x] = u8(height * 255);
		}
	}

	stbi_write_png("HeightMap.png", width, height, 1, data, width);

	HeapFree(data);
}*/
