#pragma once
#include "vec3.h"
#include "material.h"
#include "aabb.h"
#include "hittable.h"
#include <cmath>


class Quad : public Hittable
{
public:
    Vec3 Q, u, v;
    std::shared_ptr<Material> mat;
    AABB bbox;
    Vec3 normal;
    double D;
    Vec3 w;

    Quad(const Vec3& Q, const Vec3& u, const Vec3& v, std::shared_ptr<Material> mat)
    : Q(Q), u(u), v(v), mat(mat) 
    {
        Vec3 n = cross(u,v);
        normal = unitVector(n);
        D = dot(normal, Q);
        w = n * (1.0 / dot(n, n));

        AABB box1 = AABB(Q, Q + u + v);
        AABB box2 = AABB(Q + u, Q + v);
        bbox = AABB(box1, box2);
    }

    AABB boundingBox() const override
    {
        return bbox;
    }

    bool hit(const Ray& r, double tmin, double tmax, HitRecord& rec) const override
    {
        double denom = dot(normal, r.direction);
        if (std::fabs(denom) < 1e-8)
        {
            return false;
        }
        double t = ((D - dot(normal, r.origin)) / denom);
        if (!(tmin < t) || !(t < tmax))
        {
            return false;
        }
        Vec3 P = r.at(t);
        Vec3 p = P - Q;
        double a = dot(w, cross(p, v));
        double b = dot(w, cross(u, p));

        if ((0 < a) && (a < 1))
        {
            if ((0 < b) && (b < 1))
            {
                rec.t = t;
                rec.point = P;
                rec.mat = mat;
                rec.u = a;
                rec.v = b;
                rec.setFaceNormal(r, normal);
                return true;
            }
        }

        return false;
    }
};