#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include "ray.h"

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

bool initialize(SDLState &state);
void cleanup(SDLState &state);
uint32_t toPixel(const Vec3& color);
Vec3 rayColor(const Ray& r);
double hitSphere(const Vec3& center, double radius, const Ray& r);

int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 640;
	state.height = 420;
	bool running = true;
	std::vector<uint32_t> pixels(state.width * state.height);
	Vec3 camera;
	Vec3 sphere = Vec3(0, 0, -1);
	double viewportHeight = 2.0;	
	double viewportWidth = viewportHeight * static_cast<double>(state.width) / state.height;

	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	// looping through pixels
	
	for (int j = 0; j < state.height; j++)
	{
		for (int i = 0; i < state.width; i++)
		{
			double fractionX = static_cast<double>(i) / (state.width - 1);

			double fractionY = static_cast<double>(j) / (state.height - 1);

			double x = (fractionX - 0.5) * viewportWidth;
			double y = (0.5 - fractionY) * viewportHeight;

			Vec3 direction(x, y, -1.0);
			Ray r(camera, direction);

			Vec3 color = rayColor(r);
			
			pixels[j * state.width + i] = toPixel(color);



		
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
	uint32_t r = static_cast<uint32_t>(color.x * 255);
	uint32_t g = static_cast<uint32_t>(color.y * 255);
	uint32_t b = static_cast<uint32_t>(color.z * 255);
	uint32_t pixelColor = (a << 24) | (r << 16) | (g << 8) | b;
	return pixelColor;
}

Vec3 rayColor(const Ray& r)
{
	Vec3 center = Vec3(0, 0, -1);
	double t = (hitSphere(center, 0.5, r));
	if (t > 0.0)
	{
		Vec3 hitPoint = r.at(t);
		Vec3 normal = unitVector(hitPoint - center);
		return 0.5 * (normal + Vec3(1, 1, 1));
	}

	
	Vec3 unitDir = unitVector(r.direction);
	double a = 0.5 * (unitDir.y + 1.0);
	return ((1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0));
}

double hitSphere(const Vec3& center, double radius, const Ray& r)
{
	Vec3 oc = center - r.origin;
	double a = dot(r.direction, r.direction);
	double b = -2.0 * dot(r.direction, oc);
	double c = dot(oc, oc) - radius * radius;
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