#pragma once

#include "General.h"
#include "Math/Mat.h"

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
	b32 flying;

	Camera camera;
};

struct RayIntersection {
	vec3 pos;
	vec3 prev_pos;
};

Player CreatePlayer();
void ResizePlayerCamera(Camera *c, float w, float h);
RayIntersection CastRay(Player *p);
void UpdatePlayer(Player *p);
void UpdatePlayerPhysics(Player *p, float df);
vec3 GetEyePos(Player *p);
