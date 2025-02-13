#pragma

#include "General.h"
#include "Math/Vec.h"

enum {
	CHUNK_X = 16,
	CHUNK_Y = 16,
	CHUNK_Z = 16,
};

enum {
	WORLD_CHUNK_COUNT_X = 8,
	WORLD_CHUNK_COUNT_Y = 8,
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

typedef u8 Block;

struct Chunk {
	Block blocks[CHUNK_X][CHUNK_Z][CHUNK_Y];
	vec3 world_pos;
};

struct World {
	Chunk chunks[WORLD_CHUNK_COUNT_X][WORLD_CHUNK_COUNT_Z][WORLD_CHUNK_COUNT_Y];
};

Block *GetBlockAtPos(vec3 pos);
Block GetBlock(int x, int y, int z);

Chunk *GetChunk(int x, int y, int z);

float GetGroundLevel(vec3 pos);
