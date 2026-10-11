#pragma once
#include "vec3.h"

class Ray
{
public:

	Vec3 origin, direction;

	Ray() : origin(), direction() {}

	Ray(const Vec3& origin, const Vec3& direction): origin(origin), direction(direction) {}

};

inline Vec3 at(double t);