#pragma once

#include "hittable.h"
#include "material.h"
#include "texture.h"
#include "vec3.h"
#include "aabb.h"
#include <limits>
#include <cmath>
#include "random.h"

class ConstantMedium : public Hittable
{
public:
    std::shared_ptr<Hittable> boundary;
    double negInvDensity;
    std::shared_ptr<Material> phaseFunction;
    

    ConstantMedium(std::shared_ptr<Hittable> boundary, double density, std::shared_ptr<Texture> tex) :
    boundary(boundary), negInvDensity(-1.0 / density)
    {
        
        phaseFunction = std::make_shared<Isotropic>(tex);
    }

    ConstantMedium(std::shared_ptr<Hittable> boundary, double density, const Vec3& color) :
    boundary(boundary), negInvDensity(-1.0 / density)
    {
        phaseFunction = std::make_shared<Isotropic>(color);
    }

    AABB boundingBox() const override
    {
        return boundary->boundingBox();
    }
    bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override
    {
        HitRecord rec1;
        HitRecord rec2;
        double infinity = std::numeric_limits<double>::infinity();

        if(!(boundary->hit(r, -infinity, infinity, rec1)))
        {
            return false;
        }
        if (!(boundary->hit(r, rec1.t + 0.0001, infinity, rec2)))
        {
            return false;
        }

        rec1.t = std::clamp(rec1.t, tmin, tmax);
        rec2.t = std::clamp(rec2.t, tmin, tmax);

        if (rec1.t >= rec2.t)
        {
            return false;
        }

        if (rec1.t < 0)
        {
            rec1.t = 0;
        }
    

        double rayLength = r.direction.length();
        double distanceInside = (rec2.t - rec1.t) * rayLength;
        double hitDistance = negInvDensity * std::log(randomDouble());
        if (hitDistance > distanceInside) 
        {
            return false;
        }

        rec.t = rec1.t + hitDistance / rayLength;
        rec.point = r.at(rec.t);
        rec.normal = Vec3(1, 0, 0);
        rec.frontFace = true;
        rec.mat = phaseFunction.get();
        return true;
    }
};