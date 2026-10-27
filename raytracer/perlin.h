#pragma once
#include <utility>
#include "vec3.h"
#include "random.h"
#include <cmath>

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

	static double perlinInterp(const Vec3 c[2][2][2], double u, double v, double w)
	{
		double uu = u * u * (3 - 2 * u);
		double vv = v * v * (3 - 2 * v); 
		double ww = w * w * (3 - 2 * w);
		double total = 0.0;

		for (int di = 0; di < 2; di++)
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					Vec3 offset = Vec3(u-di, v - dj, w -dk);
					double weight = (di * uu + (1 - di) * (1 - uu))
					* (dj * vv + (1 - dj) * (1 - vv))
					* (dk * ww + (1 - dk) * (1 - ww));

					total += weight * dot(c[di][dj][dk], offset);
				}
			}
			
		}	

		return total;	
	}

	double noise(const Vec3& p) const
	{
		double u = p.x - std::floor(p.x);
		double v = p.y - std::floor(p.y);
		double w = p.z - std::floor(p.z);

		int i = int(std::floor(p.x));
		int j = int(std::floor(p.y));
		int k = int(std::floor(p.z));

		Vec3 c[2][2][2];
		for (int di = 0; di < 2; di++)
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					c[di][dj][dk] =randvec[ permX[(i+di) & 255] ^ permY[(j+dj) & 255] ^ permZ[(k+dk) & 255] ];	
				}
			}
			
		}
		return perlinInterp(c, u, v, w);
	}

	

};