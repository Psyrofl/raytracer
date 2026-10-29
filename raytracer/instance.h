#pragma once
#include "hittable.h"
#include "vec3.h"
#include "aabb.h"
#include <numbers>
#include <cmath>
#include <limits>

class Translate : public Hittable
{
public:
	std::shared_ptr<Hittable> object;
	Vec3 offset;
	AABB bbox;

	AABB boundingBox() const override
	{
		return bbox;
	}

	Translate(std::shared_ptr<Hittable> object, const Vec3& offset) : object(object), offset(offset)
	{ 
		Vec3 min = object->boundingBox().min + offset;
		Vec3 max = object->boundingBox().max + offset;

		bbox = AABB(min, max);
	}


	bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override
	{
		Ray offsetRay = Ray(r.origin - offset, r.direction, r.time);
		if (!(object->hit(offsetRay, tmin, tmax, rec)))
		{
			return false;
		}
		rec.point = rec.point + offset;
		return true;
	}
};

class RotateY : public Hittable
{
public:
	double cosTheta;
	double sinTheta;
	AABB bbox;
	std::shared_ptr<Hittable> object;
	
	AABB boundingBox() const override
	{
		return bbox;
	}

	RotateY(std::shared_ptr<Hittable> object, double angle) : object(object)
	{
		double radians = angle * std::numbers::pi / 180;
		cosTheta = std::cos(radians);
		sinTheta = std::sin(radians);
		double infinity = std::numeric_limits<double>::infinity();
		Vec3 min = Vec3(infinity, infinity, infinity);
		Vec3 max = Vec3(-infinity, -infinity, -infinity);
		bbox = object->boundingBox();

		for (int i = 0 ; i < 2 ; i++)
		{
			for (int j = 0 ; j < 2 ; j++)
			{
				for (int k = 0; k < 2 ; k++)
				{
					double x = i == 1 ? bbox.max.x : bbox.min.x;
					double y = j == 1 ? bbox.max.y : bbox.min.y;
					double z = k == 1 ? bbox.max.z : bbox.min.z;
					
					double newX = cosTheta * x + sinTheta * z;
					double newZ = -sinTheta * x + cosTheta * z;

					min.x = std::min(min.x, newX);
					max.x = std::max(max.x, newX);	

					min.y = std::min(min.y, y);
					max.y = std::max(max.y, y);	

					min.z = std::min(min.z, newZ);
					max.z = std::max(max.z, newZ);	
					
				}
				
				
			}
		}

		bbox = AABB(min, max);
	}

	bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override
		{

			double xo = cosTheta * r.origin.x - sinTheta * r.origin.z;
			double yo = r.origin.y;
			double zo = sinTheta * r.origin.x + cosTheta * r.origin.z;
			Vec3 origin(xo,yo,zo);

			double xd = cosTheta * r.direction.x - sinTheta * r.direction.z;
			double yd = r.direction.y;
			double zd = sinTheta * r.direction.x + cosTheta * r.direction.z;
			Vec3 direction(xd, yd, zd);

			Ray rotatedRay(origin, direction, r.time);

			if (!(object->hit(rotatedRay, tmin, tmax, rec)))
			{
				return false;
			}
			
			double px = cosTheta * rec.point.x + sinTheta * rec.point.z;
			double pz = -sinTheta * rec.point.x + cosTheta * rec.point.z;
			rec.point = Vec3(px, rec.point.y, pz);

			double nx = cosTheta * rec.normal.x + sinTheta * rec.normal.z;
			double nz = -sinTheta * rec.normal.x + cosTheta * rec.normal.z;
			rec.normal = Vec3(nx, rec.normal.y, nz);
			return true;
		}
};