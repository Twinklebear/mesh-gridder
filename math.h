#pragma once

#include <stdexcept>
#include <ostream>
#include <array>

template<typename T>
T lerp(const float t, const T &a, const T &b) {
	return (1.f - t) * a + t * b;
}

inline float rescale_value(const float x, const float oldmin, const float oldmax,
		const float newmin, const float newmax)
{
	return (newmax - newmin) * (x - oldmin) / (oldmax - oldmin) + newmin;
}

template<typename T>
struct vec3 {
	T x, y, z;

	vec3(T x = 0) : x(x), y(x), z(x) {}
	vec3(T x, T y, T z) : x(x), y(y), z(z) {}
	template<typename S>
	vec3(const vec3<S> &v) : x(v.x), y(v.y), z(v.z) {}
	const float& operator[](const int i) const {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: break;
		}
		throw std::runtime_error("Invalid index");
	}
	float& operator[](const int i) {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: break;
		}
		throw std::runtime_error("Invalid index");
	}
};
template<typename T>
vec3<T> operator+(const vec3<T> &va, const vec3<T> &vb) {
	return vec3<T>(va.x + vb.x, va.y + vb.y, va.z + vb.z);
}
template<typename T>
vec3<T> operator-(const vec3<T> &va, const vec3<T> &vb) {
	return vec3<T>(va.x - vb.x, va.y - vb.y, va.z - vb.z);
}
template<typename T>
vec3<T> operator*(const vec3<T> &va, const vec3<T> &vb) {
	return vec3<T>(va.x * vb.x, va.y * vb.y, va.z * vb.z);
}
template<typename T>
vec3<T> operator/(const vec3<T> va, const vec3<T> &vb) {
	return vec3<T>(va.x / vb.x, va.y / vb.y, va.z / vb.z);
}
template<typename T>
vec3<T> operator/(const float x, const vec3<T> &v) {
	return vec3<T>(x / v.x, x / v.y, x / v.z);
}
template<typename T>
vec3<T> operator*(const float x, const vec3<T> &v) {
	return vec3<T>(x * v.x, x * v.y, x * v.z);
}
template<typename T>
vec3<T> operator*(const vec3<T> &v, const float x) {
	return vec3<T>(x * v.x, x * v.y, x * v.z);
}
template<typename T>
bool operator==(const vec3<T> &a, const vec3<T> &b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template<typename T>
bool operator!=(const vec3<T> &a, const vec3<T> &b) {
	return a.x != b.x || a.y != b.y || a.z != b.z;
}
template<typename T>
bool operator<(const vec3<T> &a, const vec3<T> &b) {
	return a.x < b.x
		|| (a.x == b.x && a.y < b.y)
		|| (a.x == b.x && a.y == b.y && a.z < b.z);
}

template<typename T>
T dot(const vec3<T> &a, const vec3<T> &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
template<typename T>
vec3<T> cross(const vec3<T> &a, const vec3<T> &b) {
	return vec3<T>(a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
}

template<typename T>
std::ostream& operator<<(std::ostream &os, const vec3<T> &v) { 
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}

using vec3f = vec3<float>;
using vec3sz = vec3<size_t>;

struct box3f {
	vec3f lower, upper;

	box3f();
	box3f(const vec3f &lower, const vec3f &upper);
	void extend(const vec3f &v);
	vec3f center() const;
	vec3f half_lengths() const;
	const vec3f& operator[](const size_t i) const;
};
std::ostream& operator<<(std::ostream &os, const box3f &b);

// Test if the line between pa and pb intersects the box
bool line_box_intersection(const vec3f &pa, const vec3f &pb, const box3f &box);

// Test if the triangle defined by pa, pb, pc intersects the box
// Uses Akenine-Moller's SAT method:
// http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox3.txt
bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box);

