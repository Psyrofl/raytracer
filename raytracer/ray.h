#pragma once
#include "vec3.h"

class Ray
{
public:

	Vec3 origin, direction;
	double time = 0;

	Ray() : origin(), direction() {}

	Ray(const Vec3& origin, const Vec3& direction, double time = 0.0): origin(origin), direction(direction), time(time) {}

	Vec3 at(double t) const
	{
		return origin + direction * t;
	}


};
