#pragma once
#include <random>
#include <cmath>
#include "vec3.h"

inline double randomDouble()
{
	thread_local std::uniform_real_distribution<double>  dist(0.0, 1.0);
	thread_local std::mt19937 generator(std::random_device{}());	
	double x = dist(generator);
	return x;
}

inline Vec3 randomVec3()
{
	return Vec3(2 * randomDouble() - 1, 2 * randomDouble() - 1, 2 * randomDouble() - 1);
}

inline Vec3 randomUnitVector()
{
	Vec3 p;
	do
	{
		p = randomVec3();

	} while (dot(p, p) > 1 || dot(p, p) < 1e-160);

	return unitVector(p);
}

inline Vec3 randomUnitDisk()
{
	Vec3 p;
	do
	{
		p = Vec3(2 * randomDouble() - 1, 2 * randomDouble() - 1, 0);

	} while (dot(p, p) > 1);

	return p;
}

inline double reflectance(const double cosine, const double refractionIndex)
{
	double r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 = r0 * r0;
	return (r0 + (1 - r0) * std::pow(1 - cosine, 5));
}