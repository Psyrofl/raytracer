#pragma once
#include "vec3.h"
#include "ray.h"
#include <memory>
#include "aabb.h"

class Material;

struct HitRecord
{
	Vec3 point;
	Vec3 normal;
	double t = 0;
	double u = 0;
	double v = 0;
	bool frontFace = false;
	std::shared_ptr<Material> mat;

	void setFaceNormal(const Ray& r, const Vec3& outwardNormal)
	{
		frontFace = dot(r.direction, outwardNormal) < 0;

		if (!frontFace)
		{
			normal = -1.0 * outwardNormal;
		}		

		else
		{
			normal = outwardNormal;
		}
	}

};

class Hittable
{
public:
	virtual bool hit(const Ray& r, double tMin, double tMax, HitRecord& rec) const = 0;
	virtual ~Hittable() = default;
	virtual AABB boundingBox() const = 0;
};