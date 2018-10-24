#include <stdexcept>
#include <array>
#include "math.h"

vec3f::vec3f(float x) : x(x), y(x), z(x) {}
vec3f::vec3f(float x, float y, float z) : x(x), y(y), z(z) {}

vec3f operator+(const vec3f &va, const vec3f &vb) {
	return vec3f(va.x + vb.x, va.y + vb.y, va.z + vb.z);
}
vec3f operator-(const vec3f &va, const vec3f &vb) {
	return vec3f(va.x - vb.x, va.y - vb.y, va.z - vb.z);
}
vec3f operator/(const float x, const vec3f &v) {
	return vec3f(x / v.x, x / v.y, x / v.z);
}

box3f::box3f(const vec3f &lower, const vec3f &upper) : lower(lower), upper(upper) {}
const vec3f& box3f::operator[](const size_t i) const {
	switch (i) {
		case 0: return lower;
		case 1: return upper;
		default: throw std::runtime_error("invalid box index");
	}
	return vec3f();
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
	return tmin < 0.0 && tmax > 1.0;
}

bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box) {
	return line_box_intersection(pa, pb, box)
		|| line_box_intersection(pb, pc, box)
		|| line_box_intersection(pc, pa, box);
}

