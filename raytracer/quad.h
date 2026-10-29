#pragma once
#include "vec3.h"
#include "material.h"
#include "aabb.h"
#include "hittable.h"
#include <cmath>
#include "hittable_list.h"


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

inline std::shared_ptr<HittableList> box(const Vec3& a, const Vec3& b, std::shared_ptr<Material> mat)
{
    double maxX = std::max(a.x, b.x);
    double minX = std::min(a.x, b.x);

    double maxY = std::max(a.y, b.y);
    double minY = std::min(a.y, b.y);

    double maxZ = std::max(a.z, b.z);
    double minZ = std::min(a.z, b.z);

    Vec3 dx = Vec3(maxX - minX, 0, 0);
    Vec3 dy = Vec3(0, maxY - minY, 0);
    Vec3 dz = Vec3(0, 0, maxZ - minZ);

    auto sides = std::make_shared<HittableList>();

    sides->add(std::make_shared<Quad>(Vec3(minX, minY, minZ), dx, dy, mat));
    sides->add(std::make_shared<Quad>(Vec3(minX, minY, maxZ), dx, dy, mat));
    sides->add(std::make_shared<Quad>(Vec3(minX, minY, minZ), dz, dy, mat));
    sides->add(std::make_shared<Quad>(Vec3(maxX, minY, minZ), dz, dy, mat));
    sides->add(std::make_shared<Quad>(Vec3(minX, minY, minZ), dx, dz, mat));
    sides->add(std::make_shared<Quad>(Vec3(minX, maxY, minZ), dx, dz, mat));

    return sides;
}