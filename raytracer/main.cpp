#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include "ray.h"
#include <limits>
#include <random>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <thread>
#include <functional>

struct SDLState
{
	SDL_Window* window = nullptr;
	SDL_Renderer* render = nullptr;
	SDL_Texture* tex = nullptr;
	int width, height; 
};

struct Camera
{
	Vec3 lookFrom = Vec3(13, 2, 3);
	Vec3 lookAt = Vec3(0, 0, 0);
	Vec3 vup = Vec3(0, 1, 0);
	double defocusAngle = 0.6;
	double focusDist = 10;
	double vfov = 20;
	double viewportHeight = 0;
	double viewportWidth = 0;
	Vec3 u, v, w;
	Vec3 defocusDiskU, defocusDiskV;



	void update(double aspectRatio)
	{
	double h = std::tan(vfov * 2 * std::numbers::pi / 360 / 2);
	viewportHeight = 2.0 * h * focusDist;
	viewportWidth = viewportHeight * aspectRatio;

	w = unitVector(lookFrom - lookAt);
	u = unitVector(cross(vup, w));
	v = cross(w, u);

	double defocusRadius = focusDist * tan(defocusAngle * 2 * std::numbers::pi / 360 / 2);
	defocusDiskU = u * defocusRadius;
	defocusDiskV = v * defocusRadius;
	}
};
enum class Material
{
	Diffuse, Metal, Glass
};

struct Sphere
{
	Vec3 center;
	double radius;
	Vec3 albedo;
	Material material;
	double fuzz;
	double refractionIndex;

	Sphere(const Vec3& center, const double radius, const Vec3& albedo, const Material material, const double fuzz, const double refractionIndex) 
		: center(center), radius(radius), albedo(albedo), material(material), fuzz(fuzz), refractionIndex(refractionIndex) {}
};

bool initialize(SDLState &state);
void cleanup(SDLState &state);
uint32_t toPixel(const Vec3& color);
Vec3 rayColor(const Ray& r, const std::vector<Sphere>& spheres, int depth);
double hitSphere(const Sphere& sphere, const Ray& r);
double randomDouble();
Vec3 randomVec3();
Vec3 randomUnitVector();
double reflectance(const double cosine, const double refractionIndex);
Vec3 randomUnitDisk();
void renderRows(int startRow, int endRow, const SDLState& state, const std::vector<Sphere>& spheres, const int samples, const Camera& cam, std::vector<uint32_t>& pixels);


int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 640;
	state.height = 420;
	bool running = true;
	Camera cam;
	cam.update(static_cast<double>(state.width) / state.height);

	std::vector<uint32_t> pixels(state.width * state.height);

	


	Vec3 center = Vec3(0, 0, -1);
	Vec3 groundVector = Vec3(0, -1000, 0);
	Vec3 ballColor = Vec3(0.1, 0.2, 0.5);
	Vec3 groundColor = Vec3(0.5, 0.5, 0.5);


	Sphere ground(groundVector, 1000, groundColor, Material::Diffuse, 0, 1.0);
	Sphere glass(Vec3(0, 1, 0), 1, Vec3(1, 1, 1), Material::Glass, 0, 1.5);
	Sphere metal(Vec3(4, 1, 0), 1, Vec3(0.7, 0.6, 0.5), Material::Metal, 0, 1.0);
	Sphere brown(Vec3(-4, 1, 0), 1, Vec3(0.4, 0.2, 0.1), Material::Diffuse, 0, 1.0 / 1.5);

	int samples = 10;

	std::vector<Sphere>spheres;

	spheres.push_back(ground);
	spheres.push_back(glass);
	spheres.push_back(metal);
	spheres.push_back(brown);


	for (int i = -11; i <= 10; i++)
	{
		for (int j = -11; j <= 10; j++)
		{
			double choose = randomDouble();
			Vec3 ballCenter = Vec3(i + 0.9 * randomDouble(), 0.2, j + 0.9 * randomDouble());
			bool tooClose = (ballCenter - Vec3(4, 0.2, 0)).length() <= 0.9;

			if (!tooClose)
			{
				if (choose < 0.8)
				{
					Vec3 albedo = Vec3(randomDouble(), randomDouble(), randomDouble()) * Vec3(randomDouble(), randomDouble(), randomDouble());
					Sphere ball(ballCenter, 0.2, albedo, Material::Diffuse, 0, 1);
					spheres.push_back(ball);
				}
				else if (choose < 0.95)
				{
					Sphere ball(ballCenter, 0.2, Vec3(0.5 + 0.5 * randomDouble(), 0.5 + 0.5 * randomDouble(), 0.5 + 0.5 * randomDouble()), Material::Metal, 0.5 * randomDouble(), 0);
					spheres.push_back(ball);
				}
				else
				{
					Sphere ball(ballCenter, 0.2, Vec3(1,1,1), Material::Glass, 0, 1.5);
					spheres.push_back(ball);
				}
			}
		}
	}


	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	// looping through pixels
	int numThreads = std::thread::hardware_concurrency();
	if (numThreads <= 0)
	{
		numThreads = 1;
	}
	int rowsPerThread = state.height / numThreads;

	std::vector<std::thread> threads;

	for (int t = 0; t < numThreads; t++)
	{
		int start = t * rowsPerThread;
		int end = (t + 1) * rowsPerThread;
		if (t == numThreads - 1)
		{
			end = state.height;
		}
		threads.push_back(std::thread(renderRows, start, end, std::cref(state), std::cref(spheres), samples, std::cref(cam), std::ref(pixels)));

	}

	for (std::thread& t : threads)
	{
		t.join();
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
		Vec3 newDir;

		Vec3 hitPoint = r.at(closestT);
		Vec3 normal = unitVector(hitPoint - spheres[hitIndex].center);
		bool frontFace = dot(r.direction, normal) < 0;

		if (!frontFace)
		{
			normal = -1.0 * normal;
		}
		
		if (spheres[hitIndex].material == Material::Metal)
		{
			newDir = unitVector(reflect(r.direction, normal)) + spheres[hitIndex].fuzz * randomUnitVector();
			if (dot(newDir, normal) <= 0)
			{
				return Vec3(0, 0, 0);
			}
		}



		else if (spheres[hitIndex].material == Material::Glass)
		{
			double ratio;

			if (frontFace)
			{
				ratio = 1.0 / spheres[hitIndex].refractionIndex;
			}
			else 
			{
				ratio = spheres[hitIndex].refractionIndex;
			}
			
			Vec3 unitDir = unitVector(r.direction);
			double cosTheta = std::fmin(dot(-1.0 * unitDir, normal), 1.0);
			double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
			bool cannotRefract = ratio * sinTheta > 1.0;
			if (cannotRefract || reflectance(cosTheta, ratio) > randomDouble())
			{
				newDir = reflect(unitDir, normal);
			}

			else
			{
				newDir = refract(unitDir, normal, ratio);
			}
		}

		else
		{
			newDir = normal + randomUnitVector();
		}



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
	double tnear;
	double tfar;

	if (discriminant < 0)
	{
		return -1.0;
	}

	else
	{
		double sqrtD = std::sqrt(discriminant);
		tnear = (-b - sqrtD) / (2.0 * a);
		if (tnear > 0.001)
		{
			return tnear;
		}
		else
		{
			tfar = (-b + sqrtD) / (2.0 * a);
			if (tfar > 0.001)
			{
				return tfar;
			}
			else
			{
				return -1.0;
			}
		}
	}
}

double randomDouble()
{
	thread_local std::uniform_real_distribution<double>  dist(0.0, 1.0);
	thread_local std::mt19937 generator;
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

Vec3 randomUnitDisk()
{
	Vec3 p;
	do
	{
		p = Vec3(2 * randomDouble() - 1, 2 * randomDouble() - 1, 0);

	} while (dot(p, p) > 1);

	return p;
}

double reflectance(const double cosine, const double refractionIndex)
{
	double r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 = r0 * r0;
	return (r0 + (1 - r0) * std::pow(1 - cosine, 5));
}

void renderRows(int startRow, int endRow, const SDLState& state, const std::vector<Sphere>& spheres, const int samples, const Camera& cam, std::vector<uint32_t>& pixels)
{
	Vec3 origin;
	for (int j = startRow; j < endRow; j++) {
		for (int i = 0; i < state.width; i++)
		{
			Vec3 total(0, 0, 0);

			for (int s = 0; s < samples; s++)
			{

				double fractionX = static_cast<double>(i + randomDouble()) / (state.width);

				double fractionY = static_cast<double>(j + randomDouble()) / (state.height);

				double x = (fractionX - 0.5) * cam.viewportWidth;
				double y = (0.5 - fractionY) * cam.viewportHeight;

				Vec3 pixelPoint = cam.lookFrom + x * cam.u + y * cam.v - cam.focusDist * cam.w;
				Vec3 p = randomUnitDisk();
				if (cam.defocusAngle <= 0)
				{
					origin = cam.lookFrom;
				}
				else
				{
					origin = cam.lookFrom + p.x * cam.defocusDiskU + p.y * cam.defocusDiskV;
				}

				Vec3 direction = pixelPoint - origin;
				Ray r(origin, direction);

				Vec3 color = rayColor(r, spheres, 10);
				total = total + color;
			}

			Vec3 average = total * (1.0 / samples);

			pixels[j * state.width + i] = toPixel(average);

		}
	}

}