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
vec3f box3f::center() const;
std::array<vec3f, 3> box3f::half_vectors() const;
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
	return tmin >= -0.001 && tmax <= 1.001;
}

bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box) {
	// TODO: This is not enough, we actually need to do a collision test for this using SAT
	return line_box_intersection(pa, pb, box)
		|| line_box_intersection(pb, pc, box)
		|| line_box_intersection(pc, pa, box);
}

