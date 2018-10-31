#include <limits>
#include <iostream>
#include <stdexcept>
#include <array>
#include "math.h"

box3f::box3f()
	: lower(std::numeric_limits<float>::infinity()),
	upper(-std::numeric_limits<float>::infinity())
{}
box3f::box3f(const vec3f &lower, const vec3f &upper) : lower(lower), upper(upper) {}
void box3f::extend(const vec3f &v) {
	lower.x = std::min(lower.x, v.x);
	lower.y = std::min(lower.y, v.y);
	lower.z = std::min(lower.z, v.z);

	upper.x = std::max(upper.x, v.x);
	upper.y = std::max(upper.y, v.y);
	upper.z = std::max(upper.z, v.z);
}
vec3f box3f::center() const {
	return lerp(0.5, lower, upper);
}
vec3f box3f::half_lengths() const {
	const vec3f c = center();
	return upper - c;
}
const vec3f& box3f::operator[](const size_t i) const {
	switch (i) {
		case 0: return lower;
		case 1: return upper;
		default: throw std::runtime_error("invalid box index");
	}
	return vec3f();
}
std::ostream& operator<<(std::ostream &os, const box3f &b) {
	os << "[" << b.lower << ", " << b.upper << "]";
	return os;
}

bool line_box_intersection(const vec3f &pa, const vec3f &pb, const box3f &box) {
	const vec3f dir = pb - pa;
	const vec3f inv_dir = 1.0 / dir;

	const std::array<int, 3> neg_dir = {
		dir.x < 0 ? 1 : 0,
		dir.y < 0 ? 1 : 0,
		dir.z < 0 ? 1 : 0,
	};

	// Check X & Y intersection
	float tmin = (box[neg_dir[0]].x - pa.x) * inv_dir.x;
	float tmax = (box[1 - neg_dir[0]].x - pa.x) * inv_dir.x;
	float tymin = (box[neg_dir[1]].y - pa.y) * inv_dir.y;
	float tymax = (box[1 - neg_dir[1]].y - pa.y) * inv_dir.y;
	if (tmin > tymax || tymin > tmax){
		return false;
	}
	if (tymin > tmin){
		tmin = tymin;
	}
	if (tymax < tmax){
		tmax = tymax;
	}

	//Check Z intersection
	float tzmin = (box[neg_dir[2]].z - pa.z) * inv_dir.z;
	float tzmax = (box[1 - neg_dir[2]].z - pa.z) * inv_dir.z;
	if (tmin > tzmax || tzmin > tmax){
		return false;
	}
	if (tzmin > tmin){
		tmin = tzmin;
	}
	if (tzmax < tmax){
		tmax = tzmax;
	}
	return tmin >= 0.0 && tmax <= 1.0;
}

bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box) {
	// Translate so that the box center is at the origin
	const vec3f bcenter = box.center();
	const std::array<vec3f, 3> vert{pa - bcenter, pb - bcenter, pc - bcenter};
	const std::array<vec3f, 3> edge{vert[1] - vert[0], vert[2] - vert[1], vert[0] - vert[2]};
	const std::array<vec3f, 3> axes{vec3f(1, 0, 0), vec3f(0, 1, 0), vec3f(0, 0, 1)};
	const vec3f half_lens = box.half_lengths();

	// Bullet 1: Check if we can separate the triangle AABB and the box
	for (int i = 0; i < 3; ++i) {
		const float min = std::min(vert[0][i], std::min(vert[1][i], vert[2][i]));
		const float max = std::max(vert[0][i], std::max(vert[1][i], vert[2][i]));
		if (min > half_lens[i] || max < -half_lens[i]) {
			return false;
		}
	}

	// Bullet 2: test for overlap of the triangle plane and AABB
	const vec3f tri_normal = cross(edge[0], edge[1]);
	{
		vec3f vmin, vmax;
		for (int i = 0; i < 3; ++i) {
			if (tri_normal[i] > 0.0f) {
				vmin[i] = -half_lens[i] - vert[0][i];
				vmax[i] = half_lens[i] - vert[0][i];
			} else {
				vmin[i] = half_lens[i] - vert[0][i];
				vmax[i] = -half_lens[i] - vert[0][i];
			}
		}
		if (dot(tri_normal, vmin) > 0.0f) {
			return false;
		}
	}

	// Bullet 3
	// This is not the most optimal way to write the 9 tests for bullet 3, but it's easier to read
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			const vec3f a = cross(axes[i], edge[j]);
			const std::array<float, 3> p = {dot(a, vert[0]), dot(a, vert[1]), dot(a, vert[2])};
			const float minp = std::min(p[0], std::min(p[1], p[2]));
			const float maxp = std::max(p[0], std::max(p[1], p[2]));
			const float r = half_lens.x * std::abs(a.x) + half_lens.y * std::abs(a.y)
				+ half_lens.z * std::abs(a.z);
			if (minp > r || maxp < -r) {
				return false;
			}
		}
	}
	return true;
}

