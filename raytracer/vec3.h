#pragma once
#include <cmath>

class Vec3 
{
public:
	double x, y, z;

	Vec3() : x(0), y(0), z(0) {}
		
	Vec3(double x, double y, double z): x(x), y(y), z(z) {}
	
	double length() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}
	
};

inline Vec3 operator+(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

inline Vec3 operator-(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

inline Vec3 operator*(const Vec3& u, const double k)
{
	return Vec3(u.x * k, u.y * k, u.z * k);
}

inline Vec3 operator*(const double k, const Vec3& u)
{
	return u * k;

}


inline Vec3 operator*(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x * v.x, u.y * v.y, u.z * v.z);

}

inline double dot(const Vec3& u, const Vec3& v)
{
	return (u.x * v.x + u.y * v.y + u.z * v.z);
}

inline Vec3 unitVector(const Vec3& v)
{
	double len = v.length();
	return Vec3(v.x / len, v.y / len, v.z / len);
}

inline Vec3 reflect(const Vec3& v, const Vec3& n)
{
	return v - 2 * dot(v, n) * n;
}