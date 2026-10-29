#pragma once
#include <algorithm>
#include "vec3.h"
#include "ray.h"
#include <limits>

class AABB 
{
public:
    Vec3 min;
    Vec3 max;

    AABB(const Vec3& a, const Vec3& b) : min(a), max(b) 
    {
        min.x = std::min(a.x,b.x);
        min.y = std::min(a.y,b.y);
        min.z = std::min(a.z,b.z);

        max.x = std::max(a.x,b.x);
        max.y = std::max(a.y, b.y);
        max.z = std::max(a.z, b.z);

    if ((max.x - min.x) < 0.0001)
    {
        min.x -= 0.00005;
        max.x += 0.00005;
    }
    
    if ((max.y - min.y) < 0.0001)
    {
        min.y -= 0.00005;
        max.y += 0.00005;
    }

    if ((max.z - min.z) < 0.0001)
    {
        min.z -= 0.00005;
        max.z += 0.00005;
    }
    }   

    AABB(const AABB& box1, const AABB& box2) : min(box1.min), max(box2.max)
    {
        min.x = std::min(box1.min.x, box2.min.x);
        min.y = std::min(box1.min.y, box2.min.y);
        min.z = std::min(box1.min.z, box2.min.z);

        max.x = std::max(box1.max.x, box2.max.x);
        max.y = std::max(box1.max.y, box2.max.y);
        max.z = std::max(box1.max.z, box2.max.z);
    }

    AABB() 
    {
        min.x = std::numeric_limits<double>::infinity();
        max.x = -1.0 * (std::numeric_limits<double>::infinity());

        min.y = std::numeric_limits<double>::infinity();
        max.y = -1.0 * (std::numeric_limits<double>::infinity());

        min.z = std::numeric_limits<double>::infinity();
        max.z = -1.0 * (std::numeric_limits<double>::infinity());
    };
    
    bool hit(const Ray& r, double tmin, double tmax) const
    {
        double t0 = (min.x - r.origin.x) / r.direction.x;
        double t1 = (max.x - r.origin.x) / r.direction.x; 

        if (t0 > t1)
        {
            std::swap(t0,t1);
        }
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);

        if (tmax <= tmin)
        {
            return false;
        }
        
        double t2 = (min.y - r.origin.y) / r.direction.y;
        double t3 = (max.y - r.origin.y) / r.direction.y; 

        if (t2 > t3)
        {
            std::swap(t2,t3);
        }
        tmin = std::max(tmin, t2);
        tmax = std::min(tmax, t3);

        if (tmax <= tmin)
        {
            return false;
        }

        double t4 = (min.z - r.origin.z) / r.direction.z;
        double t5 = (max.z - r.origin.z) / r.direction.z; 

        if (t4 > t5)
        {
            std::swap(t4,t5);
        }
        tmin = std::max(tmin, t4);
        tmax = std::min(tmax, t5);

        if (tmax <= tmin)
        {
            return false;
        }
        
        return true;
    }

};