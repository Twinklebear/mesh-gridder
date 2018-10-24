#pragma once

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
	const vec3f& operator[](const size_t i) const;
};
std::ostream& operator<<(std::ostream &os, const box3f &b);

// Test if the line between pa and pb intersects the box
bool line_box_intersection(const vec3f &pa, const vec3f &pb, const box3f &box);

// Test if the triangle defined by pa, pb, pc intersects the box
bool triangle_box_intersection(const vec3f &pa, const vec3f &pb, const vec3f &pc, const box3f &box);


