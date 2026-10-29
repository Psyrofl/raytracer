#pragma once
#include <memory>
#include "hittable.h"
#include "random.h"
#include "texture.h"

class Material
{
public:

    virtual bool scatter(const Ray& rIn, const HitRecord& rec,Vec3& attenuation, Ray& scattered) const = 0;
    virtual ~Material() = default;
	virtual Vec3 emitted(double u, double v, const Vec3& p)const
	{
		return Vec3(0, 0, 0);
	}

};

class Lambertian : public Material
{
public:

    std::shared_ptr<Texture> tex;
    bool scatter(const Ray& rIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const override
    {
        Vec3 dir = rec.normal + randomUnitVector();
        scattered = Ray(rec.point, dir, rIn.time);
        attenuation = tex -> value(rec.u, rec.v, rec.point);
        return true;
    }

    Lambertian(const Vec3& albedo) : tex(std::make_shared<SolidColor>(albedo)) {}
	Lambertian(std::shared_ptr<Texture> tex) : tex(tex) {}

};

class Metal : public Material
{
public:
    Vec3 albedo;
    double fuzz; 

    Metal(const Vec3& albedo, const double fuzz) : albedo(albedo), fuzz(fuzz) {}

    bool scatter(const Ray& rIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const override
    {
       
        Vec3 dir = unitVector(reflect(rIn.direction, rec.normal)) + fuzz * randomUnitVector();
        scattered = Ray(rec.point, dir, rIn.time);
        attenuation = albedo;

        return (dot(dir, rec.normal) > 0);
        
    }

};

class Dielectric : public Material
{
public:
	double refractionIndex;
	Dielectric(double refractionIndex) : refractionIndex(refractionIndex) {}

	bool scatter(const Ray& rIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const override
	{
		double ratio;
		attenuation = Vec3(1, 1, 1);
		Vec3 dir;

		if (rec.frontFace)
		{
			ratio = 1.0 /refractionIndex;
		}
		else
		{
			ratio = refractionIndex;
		}

		Vec3 unitDir = unitVector(rIn.direction);
		double cosTheta = std::fmin(dot(-1.0 * unitDir, rec.normal), 1.0);
		double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
		bool cannotRefract = ratio * sinTheta > 1.0;
		if (cannotRefract || reflectance(cosTheta, ratio) > randomDouble())
		{
			dir = reflect(unitDir, rec.normal);
		}

		else
		{
			dir = refract(unitDir, rec.normal, ratio);
		}

			scattered = Ray(rec.point, dir, rIn.time);
			return true;
	}	
};

class DiffuseLight : public Material
{
public:
	std::shared_ptr<Texture> tex;

	DiffuseLight(const Vec3& color) : tex(std::make_shared<SolidColor>(color)) {}
	DiffuseLight(std::shared_ptr<Texture> tex) : tex(tex) {}

	Vec3 emitted(double u, double v, const Vec3& p) const override
	{
		return tex->value(u, v, p);
	}

	bool scatter(const Ray& rIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const override
	{
		return false;
	}
};

class Isotropic : public Material
{
public:

    std::shared_ptr<Texture> tex;
    bool scatter(const Ray& rIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const override
    {
        Vec3 dir = randomUnitVector();
        scattered = Ray(rec.point, dir, rIn.time);
        attenuation = tex -> value(rec.u, rec.v, rec.point);
        return true;
    }

    Isotropic(const Vec3& color) : tex(std::make_shared<SolidColor>(color)) {}
	Isotropic(std::shared_ptr<Texture> tex) : tex(tex) {}
};
