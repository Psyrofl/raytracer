#pragma once
#include "hittable.h"
#include "vec3.h"
#include "aabb.h"

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