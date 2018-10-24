#pragma once

struct vec3f {
	float x, y, z;

	vec3f(float x = 0);
	vec3f(float x, float y, float z);
};

vec3f operator+(const vec3f &va, const vec3f &vb);
vec3f operator-(const vec3f &va, const vec3f &vb);
vec3f operator/(const float x, const vec3f &v);

struct box3f {
	vec3f lower, upper;

	box3f(const vec3f &lower, const vec3f &upper);
	const vec3f& operator[](const size_t i) const;
};

// Test if the line between pa and pb intersects the box
bool line_box_intersection(const vec3f &pa, const vec3f &pb, const box3f &box);

// Test if the triangle defined by pa, pb, pc intersects the box
bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box);


