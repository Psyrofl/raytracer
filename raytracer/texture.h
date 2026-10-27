#pragma once
#include "vec3.h"
#include <memory>
#include <cmath>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <algorithm>
#include "perlin.h"

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

        if ((xInt + yInt + zInt) % 2 == 0)
        {
            return even->value(u, v, p);
        }
        else
        {
            return odd->value(u, v, p);
        }
    }
    
    CheckerColor(const std::shared_ptr<Texture> even, const std::shared_ptr<Texture> odd, double scale)
        : even(even), odd(odd), scale(scale) {
    }

    CheckerColor(const Vec3& c1, const Vec3& c2, double scale)
        : even(std::make_shared<SolidColor>(c1)),
        odd(std::make_shared<SolidColor>(c2)),
        scale(scale) {
    }
};

class ImageTexture : public Texture
{
public:
    int width = 0, height = 0;
    std::vector<Vec3> data;

    ImageTexture(const char* filename)
    {
        SDL_Surface* surface = IMG_Load(filename);
        if (!surface)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            return;
        }

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            return;
        }

        width = converted->w;
        height = converted->h;
        
        data.resize(width * height); 

        for (int j = 0; j < height; j++)
        {
            const uint8_t* row = static_cast<const uint8_t*>(converted->pixels) + j * converted->pitch;

            for (int i = 0; i < width; i++)
            {
                const uint8_t* px = row + i * 4;
                double r = std::pow(px[0] / 255.0,2);
                double g = std::pow(px[1] / 255.0,2);
                double b = std::pow(px[2] / 255.0,2);
                data[j * width + i] = Vec3(r, g, b);
            }
        }

        SDL_DestroySurface(converted);

    }

    Vec3 value(double u, double v, const Vec3& p) const override
    {
        if (data.empty())
        {
            return Vec3(0, 1, 1);
        }

        u = std::clamp(u, 0.0, 1.0);
        v = std::clamp(v, 0.0, 1.0);

        v = 1.0 - v;

        int i = int(u * width);
        int j = int(v * height);

        i = std::clamp(i, 0, width - 1);
        j = std::clamp(j, 0, height - 1);

        return data[j * width + i];

    }
};

class NoiseTexture : public Texture
{
public: 
    Perlin noise;
    double scale;

    NoiseTexture(const double scale): scale(scale) {}

    Vec3 value(double u, double v, const Vec3& p) const override
    {
        return (Vec3(1,1,1) * 0.5 * (1.0 + noise.noise(scale * p)));
    }

};
