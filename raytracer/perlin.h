#pragma once
#include <utility>
#include "vec3.h"
#include "random.h"

class Perlin
{
private:
	static const int pointCount = 256;
	Vec3 randvec[pointCount];
	int permX[pointCount];
	int permY[pointCount];
	int permZ[pointCount];


	static void generatePerm(int* p) 
	{
		for (int i = 0; i < pointCount; i++)
		{
			p[i] = i;
		}

		for (int i = pointCount-1; i > 0; i--)
		{
			int j = int(randomDouble() * (i + 1));
			std::swap(p[i], p[j]);
		}
	}

public: 
	Perlin()
	{
		for (int i = 0; i < pointCount; i++)
		{
			randvec[i] = randomUnitVector();
		}
		generatePerm(permX);
		generatePerm(permY);
		generatePerm(permZ);
	}

};