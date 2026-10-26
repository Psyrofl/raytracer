#pragma once
#include "vec3.h"
#include <memory>
#include <cmath>

class Texture
{
public:
    virtual Vec3 value(double u, double v, const Vec3& p) const = 0;
    virtual ~Texture() = default;

};

class SolidColor : public Texture
{
public:
    Vec3 color;

    Vec3 value(double u, double v, const Vec3& p) const override
    {
        return color;
    }

    SolidColor(const Vec3& color) : color(color) {}
};

class CheckerColor : public Texture
{
public: 
    std::shared_ptr<Texture> even, odd;
    double scale;
    Vec3 value(const double u, const double v, const Vec3& p) const override
    {
        int xInt = int(std::floor(p.x / scale));
        int yInt = int(std::floor(p.y / scale));
        int zInt = int(std::floor(p.z / scale));

        if ((xInt + yInt + zInt) %2 == 0)
        {
            return even ->value(u,v,p);
        }
        else
        {
            return odd ->value(u,v,p);
        }
    }

    CheckerColor(const std::shared_ptr<Texture> even, const std::shared_ptr<Texture> odd, double scale)
    : even(even), odd(odd), scale(scale) {}

    CheckerColor(const Vec3& c1, const Vec3& c2, double scale)
    : even(std::make_shared<SolidColor>(c1)),
      odd(std::make_shared<SolidColor>(c2)),
      scale(scale) {}
};