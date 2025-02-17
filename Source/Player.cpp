#include "Player.h"

#include "Math/NMath.h"

#include "Renderer.h"

global const float eye_height = 1.6f;

Player CreatePlayer() {
	Player result = {};

	result.position = vec3(20.0, 30.0f, 20.0f);
	result.velocity = vec3(0);
	result.acceleration = vec3(0);
	result.yaw = 0.0f;
	result.pitch = 0;
	result.on_ground = 1;
	result.flying = 1;

	return result;
}

void ResizePlayerCamera(Camera *c, float w, float h) {
	c->width = w;
	c->height = h;

	mat4 proj_matrix = Perspective(PI32 / 3.0f, c->width / c->height, 0.1f, 1000.f);
	c->proj_matrix = proj_matrix;
}

RayIntersection CastRay(Player *p) {
	RayIntersection result = {};

	const float RAY_MAX_DISTANCE = 6;
	const float RAY_STEP = 0.5f;

	vec3 origin = p->position + vec3(0, eye_height, 0);
	vec3 dir = p->camera.front;
	float dist = RAY_STEP;

	result.pos = origin;
	result.prev_pos = origin;

	while (dist < RAY_MAX_DISTANCE) {
		vec3 cp = origin + dir * dist;

		result.prev_pos = result.pos;
		result.pos = cp;

		BlockRef ref = GetBlockRef(cp);
		if (ref.c) {
			Block b = GetBlock(ref);
			if (b != BLOCK_AIR) {
				break;
			}
		}

		dist += RAY_STEP;
	}

	return result;
}

void UpdatePlayer(Player *p, float df) {
    Int2 mouse_delta = GetMouseDeltaPosition();

	if (mouse_delta.x != 0 || mouse_delta.y != 0) {
		p->yaw += mouse_delta.x * 0.001f;
		p->pitch -= mouse_delta.y * 0.001f;

		p->yaw = FMod(p->yaw, 2*PI32);

		const float eps = (PI32 / 2) - 0.02f;
		p->pitch = Clamp(p->pitch, -eps, eps);

		vec3 front;
		front.x = Cos(p->yaw) * Cos(p->pitch);
		front.y = Sin(p->pitch);
		front.z = Sin(p->yaw) * Cos(p->pitch);
		front = Normalize(front);

		p->camera.front = front;
	}

	p->acceleration = vec3(0);

	vec3 front = p->camera.front;
	float speed = df;
	if (p->flying) {
		speed *= 5;
	}

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

	if (WasKeyPressed(KEY_F)) {
		p->flying = !p->flying;
	}

	if (p->flying) {
		if (IsKeyDown(KEY_SPACE)) {
			p->velocity.y = 0.25f;
		}
		if (IsKeyDown(KEY_SHIFT)) {
			p->velocity.y = -0.25f;
		}
		p->on_ground = 0;
	} else {
		p->acceleration.y = -0.01f;
		if (IsKeyDown(KEY_SPACE) && p->on_ground) {
			p->velocity.y = 0.25f;
			p->on_ground = 0;
		}
	}

	p->velocity += p->acceleration;
	p->velocity *= 0.9;
	p->position += p->velocity;

	if (!p->flying) {
		float ground_y = GetGroundLevel(p->position);
		if (p->position.y <= ground_y) {
			p->position.y = ground_y;
			p->velocity.y = 0;
			p->on_ground = 1;
		}
	}

	if (WasButtonPressed(MOUSE_BUTTON_LEFT)) {
		RayIntersection intersect = CastRay(p);
		BlockRef br = GetBlockRef(intersect.pos);
		if (br.c) {
			PlaceBlock(br, BLOCK_AIR);
		}
	}

	if (WasButtonPressed(MOUSE_BUTTON_RIGHT)) {
		RayIntersection intersect = CastRay(p);
		BlockRef hit = GetBlockRef(intersect.pos);
		BlockRef place = GetBlockRef(intersect.prev_pos);
		if (hit.c && place.c) {
			if (GetBlock(hit) != BLOCK_AIR && GetBlock(place) == BLOCK_AIR) {
				PlaceBlock(place, BLOCK_OAK_LOG);
			}
		}
	}
}
vec3 GetEyePos(Player *p) {
	return p->position + vec3(0, eye_height, 0);
}
