#pragma once
#include "hittable.h"
#include <cmath>
#include <numbers>

class Sphere : public Hittable
{
public:
	Ray center;
	double radius;
	std::shared_ptr<Material> mat;
	AABB bbox;

	Sphere(const Vec3& staticCenter, double radius, std::shared_ptr<Material> mat) 
	: center(staticCenter, Vec3(0,0,0)), radius(radius), mat(mat) 
	{
		Vec3 rvec(radius,radius,radius);
		Vec3 minstart = center.at(0) - rvec;
		Vec3 maxstart = center.at(0) + rvec;

		AABB startbox = AABB(minstart,maxstart);

		Vec3 minend = center.at(1) - rvec;
		Vec3 maxend = center.at(1) + rvec;

		AABB endbox = AABB(minend, maxend);

		bbox = AABB(startbox, endbox);
	}

	Sphere(const Vec3& center1, const Vec3& center2, double radius, std::shared_ptr<Material> mat) 
	: center(center1, center2 - center1), radius(radius), mat(mat) 
	{
		{
		Vec3 rvec(radius,radius,radius);
		Vec3 minstart = center.at(0) - rvec;
		Vec3 maxstart = center.at(0) + rvec;

		AABB startbox = AABB(minstart,maxstart);

		Vec3 minend = center.at(1) - rvec;
		Vec3 maxend = center.at(1) + rvec;

		AABB endbox = AABB(minend, maxend);

		bbox = AABB(startbox, endbox);
		}
	}

	static void getSphereUV(const Vec3& p, double& u, double& v)
	{
		double theta = std::acos(-p.y);
		double phi = std::atan2(-p.z, p.x) + std::numbers::pi;
		u = phi / (2 * std::numbers::pi);
		v = theta / std::numbers::pi;
	}


	bool hit(const Ray& r, double tMin, double tMax, HitRecord& rec) const override
	{
		Vec3 currentCenter = center.at(r.time);
		Vec3 oc = currentCenter - r.origin;
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
		outward = (rec.point - currentCenter) * (1.0 / radius);
		rec.setFaceNormal(r, outward);
		rec.mat = mat;
		getSphereUV(outward, rec.u, rec.v);
		return true;
	}

	AABB boundingBox() const override
	{
		return bbox;
	}
};