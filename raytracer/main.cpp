#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include "ray.h"
#include <limits>
#include <random>
#include <algorithm>

// Your ray tracer starts here.
//
// Plan:
//   1. Set up SDL: window + renderer (you can reuse initialize/cleanup from your game)
//   2. Make a std::vector<uint32_t> of pixels and a streaming texture
//   3. Fill the pixels with the red/green gradient
//   4. Each frame: SDL_UpdateTexture, SDL_RenderTexture, SDL_RenderPresent




struct SDLState
{
	SDL_Window* window = nullptr;
	SDL_Renderer* render = nullptr;
	SDL_Texture* tex = nullptr;
	int width, height; 
};

struct Sphere
{
	Vec3 center;
	double radius;
	Vec3 albedo;


	Sphere(const Vec3& center, const double radius, const Vec3&albedo) : center(center), radius(radius), albedo(albedo) {}
};

bool initialize(SDLState &state);
void cleanup(SDLState &state);
uint32_t toPixel(const Vec3& color);
Vec3 rayColor(const Ray& r, const std::vector<Sphere>& spheres, int depth);
double hitSphere(const Sphere& sphere, const Ray& r);
double randomDouble();
Vec3 randomVec3();
Vec3 randomUnitVector();


int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 640;
	state.height = 420;
	bool running = true;

	std::vector<uint32_t> pixels(state.width * state.height);

	Vec3 camera;
	Vec3 center = Vec3(0, 0, -1);
	Vec3 groundVector = Vec3(0, -100.5, -1);
	Vec3 ballColor = Vec3(0.1, 0.2, 0.5);
	Vec3 groundColor = Vec3(0.8, 0.8, 0.0);

	double viewportHeight = 2.0;	
	double viewportWidth = viewportHeight * static_cast<double>(state.width) / state.height;


	Sphere ball(center, 0.5, ballColor);
	Sphere ground(groundVector, 100, groundColor );

	std::vector<Sphere>spheres;

	spheres.push_back(ball);
	spheres.push_back(ground);

	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	// looping through pixels
	int samples = 10;
	for (int j = 0; j < state.height; j++)
	{
		for (int i = 0; i < state.width; i++)
		{
			Vec3 total(0, 0, 0);

			for (int s = 0; s < samples; s++)
			{

				double fractionX = static_cast<double>(i + randomDouble()) / (state.width);

				double fractionY = static_cast<double>(j + randomDouble()) / (state.height);

				double x = (fractionX - 0.5) * viewportWidth;
				double y = (0.5 - fractionY) * viewportHeight;

				Vec3 direction(x, y, -1.0);
				Ray r(camera, direction);

				Vec3 color = rayColor(r, spheres, 10);
				total = total + color;
			}
			
			Vec3 average = total * (1.0 / samples);

			pixels[j * state.width + i] = toPixel(average);




		
		}
	}

	while (running) 
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
		}

		SDL_UpdateTexture(state.tex, nullptr, pixels.data(), state.width * sizeof(uint32_t));
		SDL_RenderTexture(state.render, state.tex, nullptr, nullptr);
		SDL_RenderPresent(state.render);
	}

	
	cleanup(state);
	return 0;
}

bool initialize(SDLState& state)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
		return false;
	}
	state.window = SDL_CreateWindow("raytracer", state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
		return false;
	}
	state.render = SDL_CreateRenderer(state.window, nullptr);
	if (!state.render)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), state.window);
		return false;
	}
	SDL_SetRenderLogicalPresentation(state.render, state.width, state.height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	state.tex = SDL_CreateTexture(state.render, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, state.width, state.height);
	if (!state.tex)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), state.window);
		return false;
	}
	return true;
}

void cleanup(SDLState& state)
{
	SDL_DestroyTexture(state.tex);
	SDL_DestroyRenderer(state.render);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

uint32_t toPixel(const Vec3& color)
{
	uint32_t a = 255;
	uint32_t r = static_cast<uint32_t>(std::sqrt(std::clamp(color.x, 0.0, 1.0)) * 255);
	uint32_t g = static_cast<uint32_t>(std::sqrt(std::clamp(color.y,0.0,1.0)) * 255);
	uint32_t b = static_cast<uint32_t>(std::sqrt(std::clamp(color.z, 0.0, 1.0)) * 255);
	uint32_t pixelColor = (a << 24) | (r << 16) | (g << 8) | b;
	return pixelColor;
}

Vec3 rayColor(const Ray& r, const std::vector<Sphere> &spheres, int depth)
{
	if (depth <= 0)
	{
		return Vec3(0, 0, 0);
	}
	double closestT = std::numeric_limits<double>::infinity();
	int hitIndex = -1;

	for (int k = 0; k < spheres.size(); k++)
	{
		double t = hitSphere(spheres[k], r);
		if (t > 0.001 && t < closestT)
		{
			closestT = t;
			hitIndex = k;
		}
	}

	if (hitIndex != -1)
	{
		Vec3 hitPoint = r.at(closestT);
		Vec3 normal = unitVector(hitPoint - spheres[hitIndex].center);
		Vec3 newDir = normal + randomUnitVector();
		Ray rr(hitPoint, newDir);
		return spheres[hitIndex].albedo *rayColor(rr, spheres, depth - 1);

	}

	Vec3 unitDir = unitVector(r.direction);
	double a = 0.5 * (unitDir.y + 1.0);
	return ((1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0));
}

double hitSphere(const Sphere& sphere, const Ray& r)
{
	Vec3 oc = sphere.center - r.origin;
	double a = dot(r.direction, r.direction);
	double b = -2.0 * dot(r.direction, oc);
	double c = dot(oc, oc) - sphere.radius * sphere.radius;
	double discriminant = b * b - 4 * a * c;

	if (discriminant < 0)
	{
		return -1.0;
	}
	else
	{
		return ((-b - std::sqrt(discriminant)) / (2.0 * a));
	}
}

double randomDouble()
{
	static std::uniform_real_distribution<double>  dist(0.0, 1.0);
	static std::mt19937 generator;
	double x =   dist(generator);
	return x;
}

Vec3 randomVec3()
{
	return Vec3(2 * randomDouble() - 1, 2 * randomDouble() - 1, 2 * randomDouble() - 1);
}

Vec3 randomUnitVector()
{
	Vec3 p;
	do
	{
		p = randomVec3();

	} while (dot(p, p) > 1 || dot(p,p) < 1e-160);

	return unitVector(p);
}