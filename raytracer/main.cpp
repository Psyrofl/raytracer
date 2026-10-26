
#include <vector>
#include <cstdint>
#include <limits>
#include <random>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <thread>
#include <functional>

#include <SDL3/SDL.h>
#include "ray.h"
#include "random.h"
#include "sphere.h"
#include "hittable_list.h"
#include "material.h"
#include "bvh.h"


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


bool initialize(SDLState &state);
void cleanup(SDLState &state);
uint32_t toPixel(const Vec3& color);
Vec3 rayColor(const Ray& r, const Hittable& world, int depth);
void renderRows(int startRow, int endRow, const SDLState& state, const Hittable& world, std::vector<Vec3>& accum, const Camera& cam, std::vector<uint32_t>& pixels);


int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 640;
	state.height = 420;
	bool running = true;
	Camera cam;
	double aspectRatio = static_cast<double>(state.width) / state.height;
	cam.update(aspectRatio);
	double speed = 5.0;

	std::vector<uint32_t> pixels(state.width * state.height);

	HittableList world;


	auto groundMat = std::make_shared<Lambertian>(Vec3(0.5, 0.5, 0.5));
	auto glassMat = std::make_shared<Dielectric>(1.5);
	auto metalMat = std::make_shared<Metal>(Vec3(0.7, 0.6, 0.5), 0);
	auto brownMat = std::make_shared<Lambertian>(Vec3(0.4, 0.2, 0.1));

	world.add(std::make_shared<Sphere>(Vec3(0, -1000, 0), 1000, groundMat));
	world.add(std::make_shared<Sphere>(Vec3(0, 1, 0), 1.0, glassMat));
	world.add(std::make_shared<Sphere>(Vec3(4, 1, 0), 1, metalMat));
	world.add(std::make_shared<Sphere>(Vec3(-4, 1, 0), 1, brownMat));



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
					auto randomMat = std::make_shared<Lambertian>(Vec3(randomDouble(), randomDouble(), randomDouble()) * Vec3(randomDouble(), randomDouble(), randomDouble()));
					Vec3 center2 = ballCenter + Vec3(0, randomDouble() * 0.5, 0);
					world.add(std::make_shared<Sphere>(ballCenter, center2, 0.2, randomMat));
				}
				else if (choose < 0.95)
				{
					auto randomMat = std::make_shared<Metal>(Vec3(0.5 + 0.5 * randomDouble(), 0.5 + 0.5 * randomDouble(), 0.5 + 0.5 * randomDouble()), 0.5 * randomDouble());
					world.add(std::make_shared<Sphere>(ballCenter, 0.2, randomMat));

				}
				else
				{
					world.add(std::make_shared<Sphere>(ballCenter, 0.2, glassMat));

				}
			}
		}
	}

	world = HittableList(std::make_shared<BVHNode>(world));



	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	int sampleCount = 0;
	const int maxSamples = 500;
	std::vector<Vec3> accum(state.width * state.height);

	uint64_t prevTime = SDL_GetTicks();
	 
	Vec3 d = unitVector(cam.lookAt - cam.lookFrom);
	double yaw = std::atan2(d.x, d.z);
	double pitch = std::asin(d.y);

	while (running) 
	{
		bool cameraChanged = false;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

			if (event.type == SDL_EVENT_MOUSE_WHEEL)
			{
				cam.vfov = cam.vfov - event.wheel.y * 2.0;
				cam.vfov = std::clamp(cam.vfov, 1.0, 120.0);
				cameraChanged = true;
			}
		}

		int numThreads = std::thread::hardware_concurrency();
		if (numThreads <= 0)
		{
			numThreads = 1;
		}
		int rowsPerThread = state.height / numThreads;

		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = ((nowTime - prevTime)/1000.0f);
		prevTime = nowTime;

		const bool* keys = SDL_GetKeyboardState(nullptr);
		Vec3 move(0, 0, 0);

		if (keys[SDL_SCANCODE_W])
		{
			move = move + (- 1.0 * cam.w);
		}

		if (keys[SDL_SCANCODE_S])
		{
			move = move + cam.w;
		}

		if (keys[SDL_SCANCODE_A])
		{
			move = move + (-1.0 * cam.u);
		}

		if (keys[SDL_SCANCODE_D])
		{
			move = move + cam.u;
		}

		if (keys[SDL_SCANCODE_Q])
		{
			move = move + (-1.0 * cam.v);
		}

		if (keys[SDL_SCANCODE_E])
		{
			move = move + cam.v;
		}

		double turnSpeed = 1.5;

		if (keys[SDL_SCANCODE_LEFT])
		{
			yaw = yaw + turnSpeed * deltaTime;
			cameraChanged = true;
		}

		if (keys[SDL_SCANCODE_RIGHT])
		{
			yaw = yaw - turnSpeed * deltaTime;
			cameraChanged = true;

		}

		if (keys[SDL_SCANCODE_UP])
		{
			pitch = pitch + turnSpeed * deltaTime;
			cameraChanged = true;

		}

		if (keys[SDL_SCANCODE_DOWN])
		{
			pitch = pitch - turnSpeed * deltaTime;
			cameraChanged = true;
		}

		pitch = std::clamp(pitch, -1.5, 1.5); 



		if (dot(move, move) > 0)
		{
			move = move * (speed * deltaTime);

			cam.lookFrom = cam.lookFrom + move;
			cameraChanged = true;
		}

		if (cameraChanged)
		{
			Vec3 forward = Vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
			cam.lookAt = cam.lookFrom + forward * cam.focusDist;
			cam.update(aspectRatio);
			std::fill(accum.begin(), accum.end(), Vec3(0, 0, 0));
			sampleCount = 0;
		}


		if (sampleCount < maxSamples)
		{
			std::vector<std::thread> threads;

			for (int t = 0; t < numThreads; t++)
			{
				int start = t * rowsPerThread;
				int end = (t + 1) * rowsPerThread;
				if (t == numThreads - 1)
				{
					end = state.height;
				}

				threads.push_back(std::thread(renderRows, start, end, std::cref(state), std::cref(world), std::ref(accum), std::cref(cam), std::ref(pixels)));

			}

			for (std::thread& t : threads)
			{
				t.join();
			}


			sampleCount++;

			for (int k = 0; k < (int)accum.size(); k++)
			{
				pixels[k] = toPixel(accum[k] * (1.0 / sampleCount));
			}
			
		}

		else
		{
			SDL_Delay(16);
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

Vec3 rayColor(const Ray& r, const Hittable& world, int depth)
{
	if (depth <= 0)
	{
		return Vec3(0, 0, 0);
	}

	HitRecord rec;

	if (world.hit(r, 0.001, std::numeric_limits<double>::infinity(), rec))
	{
		Ray scattered;
		Vec3 attenuation;
		if (rec.mat->scatter(r, rec, attenuation, scattered))
		{
			return attenuation * rayColor(scattered, world, depth - 1);
		}

		else
		{
			return Vec3(0,0,0);
		}
	}

	Vec3 unitDir = unitVector(r.direction);
	double a = 0.5 * (unitDir.y + 1.0);
	return ((1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0));
}






void renderRows(int startRow, int endRow, const SDLState& state, const Hittable& world, std::vector<Vec3>& accum, const Camera& cam, std::vector<uint32_t>& pixels)
{
	Vec3 origin;
	for (int j = startRow; j < endRow; j++) {
		for (int i = 0; i < state.width; i++)
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
			Ray r(origin, direction, randomDouble());

			Vec3 color = rayColor(r, world, 10);
			accum[j * state.width + i] = accum[j * state.width + i] + color;
			

		}
	}

}