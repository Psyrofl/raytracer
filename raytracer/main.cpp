#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include "vec3.h"

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

int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 640;
	state.height = 420;
	bool running = true;
	std::vector<uint32_t> pixels(state.width * state.height);

	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	// looping through pixels
	uint32_t b = 0;
	uint32_t a = 255;

	for (int j = 0; j < state.height; j++)
	{
		for (int i = 0; i < state.width; i++)
		{
			double fractionX = static_cast<double>(i) / (state.width - 1);
			uint32_t r = static_cast<uint32_t>(fractionX * 255);

			double fractionY = static_cast<double>(j) / (state.height - 1);
			uint32_t g = static_cast<uint32_t>(fractionY * 255);

			uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;

			pixels[j * state.width + i] = color;
		
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
	state.tex = SDL_CreateTexture(state.render, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, state.width, state.height);
	if (!state.tex)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), state.window);
		return false;


	}
}

void cleanup(SDLState& state)
{
	SDL_DestroyTexture(state.tex);
	SDL_DestroyRenderer(state.render);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}