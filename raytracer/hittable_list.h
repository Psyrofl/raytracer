#pragma once
#include <memory>
#include "hittable.h"
#include <vector>
#include "aabb.h"

class HittableList : public Hittable
{
public:
	AABB bbox;
	std::vector<std::shared_ptr<Hittable>> objects;

	void add(std::shared_ptr<Hittable> object)
	{
		objects.push_back(object);
		bbox = AABB(bbox, object -> boundingBox());	
	}

	bool hit(const Ray& r, double tMin, double tMax, HitRecord& rec) const override
	{
		HitRecord tempRec;
		bool hitAnything = false;
		double closestSoFar = tMax;

		for (const auto& object : objects)
		{
			if (object->hit(r, tMin, closestSoFar, tempRec))
			{
				hitAnything = true;
				closestSoFar = tempRec.t;
				rec = tempRec;
			}
		}
		return hitAnything;
	}
	
	AABB boundingBox() const override
	{
		return bbox;
	}

	HittableList(std::shared_ptr<Hittable> object)
	{
		add(object);
	}

	HittableList() {}
};