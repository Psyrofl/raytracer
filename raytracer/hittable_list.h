#pragma once
#include <memory>
#include "hittable.h"
#include <vector>

class HittableList : public Hittable
{
public:
	std::vector<std::shared_ptr<Hittable>> objects;

	void add(std::shared_ptr<Hittable> object)
	{
		objects.push_back(object);
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

};