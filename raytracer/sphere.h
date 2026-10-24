#pragma once
#include "hittable.h"

class Sphere : public Hittable
{
public:
	Vec3 center;
	double radius;

	Sphere(const Vec3& center, const double radius) : center(center), radius(radius) {}

	bool hit(const Ray& r, double tMin, double tMax, HitRecord& rec) const override
	{
		Vec3 oc = center - r.origin;
		double a = dot(r.direction, r.direction);
		double b = -2.0 * dot(r.direction, oc);
		double c = dot(oc, oc) - radius * radius;
		double discriminant = b * b - 4 * a * c;
		Vec3 outward;

		if (discriminant < 0)
		{
			return false;
		}

	
		double sqrtD = std::sqrt(discriminant);
		double tnear = (-b - sqrtD) / (2.0 * a);

		if (tnear > tMin &&  tnear < tMax)
		{
			rec.t = tnear;
		}

		else
		{
			double tfar = (-b + sqrtD) / (2.0 * a);
			if (tfar > tMin && tfar < tMax)
			{
				rec.t = tfar;
					
			}
			else
			{
				return false;
			}
		}

		rec.point = r.at(rec.t);
		outward = (rec.point - center) * (1.0 / radius);
		rec.setFaceNormal(r, outward);
		return true;
	}
};