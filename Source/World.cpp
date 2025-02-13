#include "World.h"

global World world;

Block *GetBlockAtPos(vec3 pos) {
	int cx = int(pos.x) / CHUNK_X;
	int cz = int(pos.z) / CHUNK_Z;
	int cy = int(pos.y) / CHUNK_Y;

	if (cx < 0 || cz < 0 || cy < 0 || cx >= WORLD_CHUNK_COUNT_X || cz >= WORLD_CHUNK_COUNT_Z || cy >= WORLD_CHUNK_COUNT_Y) {
		return 0;
	}

	int bx = int(pos.x) % CHUNK_X;
	int bz = int(pos.z) % CHUNK_Z;
	int by = int(pos.y) % CHUNK_Y;

	Chunk *c = &world.chunks[cx][cz][cy];
	Block *b = &c->blocks[bx][bz][by];

	return b;
}

Block GetBlock(int x, int y, int z) {
	if_unlikely(x < 0 || y < 0 || z < 0) {
		return BLOCK_AIR;
	}

	int cx = x / CHUNK_X;
	int cy = y / CHUNK_Y;
	int cz = z / CHUNK_Z;

	if_unlikely(cx >= WORLD_CHUNK_COUNT_X || cy >= WORLD_CHUNK_COUNT_Y || cz >= WORLD_CHUNK_COUNT_Z) {
		return BLOCK_AIR;
	}

	int bx = x % CHUNK_X;
	int by = y % CHUNK_Y;
	int bz = z % CHUNK_Z;
	
	Chunk *c = &world.chunks[cx][cz][cy];
	return c->blocks[bx][bz][by];
}

Chunk *GetChunk(int x, int y, int z) {
	return &world.chunks[x][z][y];
}

float GetGroundLevel(vec3 pos) {
	int cx = int(pos.x) / CHUNK_X;
	int cy = int(pos.y) / CHUNK_Y;
	int cz = int(pos.z) / CHUNK_Z;

	int bx = int(pos.x) % CHUNK_X;
	int by = int(pos.y) % CHUNK_Y;
	int bz = int(pos.z) % CHUNK_Z;

	Chunk *cys = world.chunks[cx][cz];
	for (int ccy = cy; ccy >= 0; --ccy) {
		Chunk *c = &cys[ccy];
		Block *bys = c->blocks[bx][bz];
		for (int bby = by; bby >= 0; --bby) {
			if (bys[bby] != BLOCK_AIR) {
				return bby + 1;
			}
		}
	}

	return 0;
}
