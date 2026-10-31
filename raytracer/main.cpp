#include <vector>
#include <cstdint>
#include <limits>
#include <random>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <thread>
#include <functional>
#include <string>

#include <SDL3/SDL.h>
#include "ray.h"
#include "random.h"
#include "sphere.h"
#include "hittable_list.h"
#include "material.h"
#include "bvh.h"
#include "texture.h"
#include "perlin.h"
#include "quad.h"
#include "instance.h"
#include "constant_medium.h"


struct SDLState
{
	SDL_Window* window = nullptr;
	SDL_Renderer* render = nullptr;
	SDL_Texture* tex = nullptr;
	int width, height; 
};

struct Camera
{
	Vec3 lookFrom = Vec3(0,0,9);
	Vec3 lookAt = Vec3(0, 0, 0);
	Vec3 vup = Vec3(0, 1, 0);
	double defocusAngle = 0;
	double focusDist = 20;
	double vfov = 80;
	double viewportHeight = 0;
	double viewportWidth = 0;
	Vec3 u, v, w;
	Vec3 defocusDiskU, defocusDiskV;
	Vec3 background = Vec3(0, 0, 0);
	bool useSky = true;
	double speed = 5;


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
Vec3 rayColor(const Ray& r, const Hittable& world, int depth, const Camera& cam);
void renderRows(int startRow, int endRow, int step, int depth, const SDLState& state, const Hittable& world, std::vector<Vec3>& accum, const Camera& cam, std::vector<uint32_t>& pixels, bool preview);
Vec3 previewColor(const Ray& r, const Hittable& world, const Camera& cam);
HittableList cornellBox(Camera& cam);
HittableList cityScene(Camera& cam);
std::shared_ptr<HittableList> makeCar(std::shared_ptr<Material> paint, std::shared_ptr<Material> glass, std::shared_ptr<Material> tire, std::shared_ptr<Material> lights);


int main(int argc, char* argv[])
{
	SDLState state;
	SDL_Event event;
	state.width = 600;
	state.height = 400;
	bool running = true;
	Camera cam;

	std::vector<uint32_t> pixels(state.width * state.height);

	HittableList world = cityScene(cam);
	
	world = HittableList(std::make_shared<BVHNode>(world));

	double aspectRatio = static_cast<double>(state.width) / state.height;
	cam.update(aspectRatio);



	if (!initialize(state))
	{
		cleanup(state);
		return 1;
	}

	int sampleCount = 0;
	const int maxSamples = 2000;
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

			if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_P)
			{
				SDL_Surface* shot = SDL_CreateSurfaceFrom(state.width, state.height, SDL_PIXELFORMAT_ARGB8888, pixels.data(), state.width * sizeof(uint32_t));

				if (shot)
				{
					std::string name = "render_" + std::to_string(SDL_GetTicks()) + "_" + std::to_string(sampleCount) + "samples.png";

					if (!IMG_SavePNG(shot, name.c_str()))
					{
						SDL_Log("Save failed: %s", SDL_GetError());
					}

					SDL_DestroySurface(shot);
				}
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
			move = move * (cam.speed * deltaTime);

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

		bool preview = cameraChanged;
		int step  = preview ? 2 : 1;
		int depth = preview ? 5 : 50;


		if (preview || sampleCount < maxSamples)
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

				threads.push_back(std::thread(renderRows, start, end, step, depth, std::cref(state), std::cref(world), std::ref(accum), std::cref(cam), std::ref(pixels), preview));

			}

			for (std::thread& t : threads)
			{
				t.join();
			}

			
			if (!preview)
			{
				sampleCount ++;
				for (int k = 0; k < (int)accum.size(); k++)
				{
					pixels[k] = toPixel(accum[k] * (1.0 / sampleCount));
				}
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

Vec3 rayColor(const Ray& r, const Hittable& world, int depth, const Camera& cam)
{
	if (depth <= 0)
	{
		return Vec3(0, 0, 0);
	}

	HitRecord rec;

	if (world.hit(r, 0.001, std::numeric_limits<double>::infinity(), rec))
	{
		Vec3 light = rec.mat->emitted(rec.u, rec.v, rec.point);
		Ray scattered;
		Vec3 attenuation;
		if (rec.mat->scatter(r, rec, attenuation, scattered))
		{
			return light + attenuation * rayColor(scattered, world, depth - 1, cam);
		}

		else
		{
			return light;
		}
	}

	if (cam.useSky)
	{
		Vec3 unitDir = unitVector(r.direction);
		double a = 0.5 * (unitDir.y + 1.0);
		return ((1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0));
	}

	else
	{
		return cam.background;
	}
}

void renderRows(int startRow, int endRow, int step, int depth, const SDLState& state, const Hittable& world, std::vector<Vec3>& accum, const Camera& cam, std::vector<uint32_t>& pixels, bool preview)
{
	Vec3 origin;
	for (int j = startRow; j < endRow; j += step) {
		for (int i = 0; i < state.width; i += step)
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

			Vec3 color = preview ? previewColor(r, world, cam) : rayColor(r, world, depth, cam);
			color = Vec3(std::min(color.x, 10.0), std::min(color.y, 10.0), std::min(color.z, 10.0));
			if (preview)
			{
				uint32_t pixel = toPixel(color);
				for (int py = j; py < j + step && py < endRow; py++)
				{
					for (int px = i; px < i + step && px < state.width; px++)
					{
						pixels[py * state.width + px] = pixel;
					}
				}
			}

			else
			{
				accum[j * state.width + i] = accum[j * state.width + i] + color;
			}

		}
	}
}

Vec3 previewColor(const Ray& r, const Hittable& world, const Camera& cam)
{
	Vec3 attenuation = Vec3(0,0,0);

	HitRecord rec;

	if (world.hit(r, 0.001, std::numeric_limits<double>::infinity(), rec))
	{
		Vec3 light = rec.mat->emitted(rec.u, rec.v, rec.point);
		Ray scattered;
		if (rec.mat->scatter(r, rec, attenuation, scattered))
		{
			double shade = std::fabs(dot(rec.normal, unitVector(r.direction)));
			return attenuation * (0.1 + 0.4 * shade) + light;
		}

		else
		{
			return light;
		}
	}
	
	if (cam.useSky)
		{
			Vec3 unitDir = unitVector(r.direction);
			double a = 0.5 * (unitDir.y + 1.0);
			return ((1.0 - a) * Vec3(1.0, 1.0, 1.0) + a * Vec3(0.5, 0.7, 1.0));
		}

		else
		{
			return cam.background;
		}
}

HittableList cornellBox(Camera& cam)
{
	HittableList world;

	cam.useSky = false;
	cam.lookFrom = Vec3(278, 278, -800);
	cam.lookAt = Vec3(278, 278, 0);
	cam.vfov = 40;
	cam.speed = 200;

	auto red = std::make_shared<Lambertian>(Vec3(0.65, 0.05, 0.05));
	auto white = std::make_shared<Lambertian>(Vec3(0.73, 0.73, 0.73));
	auto green = std::make_shared<Lambertian>(Vec3(0.12, 0.45, 0.15));
	auto light = std::make_shared<DiffuseLight>(Vec3(7, 7, 7));


	world.add(std::make_shared<Quad>(Vec3(555, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), green));
	world.add(std::make_shared<Quad>(Vec3(0, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), red));
	world.add(std::make_shared<Quad>(Vec3(113, 554, 127), Vec3(330, 0, 0), Vec3(0, 0, 305), light));
	world.add(std::make_shared<Quad>(Vec3(0, 0, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));
	world.add(std::make_shared<Quad>(Vec3(555, 555, 555), Vec3(-555, 0, 0), Vec3(0, 0, -555), white));
	world.add(std::make_shared<Quad>(Vec3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 555, 0), white));

	std::shared_ptr<Hittable> box1 = box(Vec3(0, 0, 0), Vec3(165, 330, 165), white);
	box1 = std::make_shared<RotateY>(box1, 15);
	box1 = std::make_shared<Translate>(box1, Vec3(265, 0, 295));
	world.add(std::make_shared<ConstantMedium>(box1, 0.01, Vec3(0, 0, 0)));

	std::shared_ptr<Hittable> box2 = box(Vec3(0, 0, 0), Vec3(165, 165, 165), white);
	box2 = std::make_shared<RotateY>(box2, -18);
	box2 = std::make_shared<Translate>(box2, Vec3(130, 0, 65));
	world.add(std::make_shared<ConstantMedium>(box2, 0.01, Vec3(1, 1, 1)));
	return world;
}

HittableList cityScene(Camera& cam)
{
	HittableList world; 

	std::mt19937 rng(42);
	std::uniform_real_distribution<double> dist(0.0, 1.0);

	cam.useSky = false;
	cam.lookFrom = Vec3(0, 1.7, 5);
	cam.lookAt = Vec3(0, 8, -50);
	cam.vfov = 50;
	cam.background = Vec3(0.02, 0.025, 0.05);

	auto asphalt = std::make_shared<Lambertian>(Vec3(0.12, 0.12, 0.12));
	auto sidewalkMat = std::make_shared<Lambertian>(Vec3(0.3, 0.3, 0.3));
	auto poleMat = std::make_shared<Lambertian>(Vec3(0.05, 0.05, 0.05));
	auto lampMat = std::make_shared<DiffuseLight>(Vec3(20, 15, 7));
	auto windowMat = std::make_shared<DiffuseLight>(Vec3(1.2, 0.8, 0.35));
	auto darkWindowMat = std::make_shared<Metal>(Vec3(0.1, 0.1, 0.12), 0.05);
	auto puddleMat = std::make_shared<Metal>(Vec3(0.5,0.5,0.55), 0.02);
	auto headlightMat = std::make_shared<DiffuseLight>(Vec3(120, 20, 16));
	auto rearLight = std::make_shared<DiffuseLight>(Vec3(15, 1, 1));
	auto redPaint = std::make_shared<Lambertian>(Vec3(0.7, 0.08, 0.08));
	auto tireMat = std::make_shared<Lambertian>(Vec3(0.02, 0.02, 0.02));
	auto headMat = std::make_shared<DiffuseLight>(Vec3(20, 20, 16));
	auto bluePaint = std::make_shared<Lambertian>(Vec3(0.1, 0.25, 0.75));
	auto greyPaint = std::make_shared<Lambertian>(Vec3(0.55, 0.55, 0.6));
	auto moonTex = std::make_shared<ImageTexture>("moonmap.jpg");
	auto moonMat = std::make_shared<DiffuseLight>(moonTex); 
	auto white = std::make_shared<Lambertian>(Vec3(0.73, 0.73, 0.73));


	std::shared_ptr<Hittable> sidewalk1 = box(Vec3(-9, 0, 10), Vec3(-6, 0.15, -110), sidewalkMat);
	std::shared_ptr<Hittable> sidewalk2 = box(Vec3(6, 0, 10), Vec3(9, 0.15, -110), sidewalkMat);


	world.add(std::make_shared<Quad>(Vec3(-6, 0, 10), Vec3(12, 0, 0), Vec3(0, 0, -120), asphalt));
	world.add(sidewalk1);
	world.add(sidewalk2);
	world.add(std::make_shared<Sphere>(Vec3(-2.4, 0.7, -30), Vec3(-2.4, 0.7, -22), 0.15, headlightMat));
	world.add(std::make_shared<Sphere>(Vec3(-3.6, 0.7, -30), Vec3(-3.6, 0.7, -22), 0.15, headlightMat));
	world.add(std::make_shared<Sphere>(Vec3(2.4, 0.8, -18), Vec3(2.4, 0.8, -26), 0.1, rearLight));
	world.add(std::make_shared<Sphere>(Vec3(3.6, 0.8, -18), Vec3(3.6, 0.8, -26), 0.1, rearLight));
	world.add(std::make_shared<Sphere>(Vec3(0, 70, -200), 12, moonMat));



	for (int side = 0; side < 2; side++)
	{
		double z = 10;
		while (z > -110)
		{
			double length = 6 + 6 * dist(rng);
			double depth = 8 + 6 * dist(rng);
			double height = 8 + 30 * dist(rng);
			double gap = 0.5 + 1.0 * dist(rng);

			std::shared_ptr<Material> mat;

			bool isGlass = dist(rng) < 0.1;

			
			if (isGlass)
			{
				mat = std::make_shared<Dielectric>(1.5);
			}
			else
			{
				mat = std::make_shared<Lambertian>(Vec3(0.1 + 0.15 * dist(rng), 0.1 + 0.15 * dist(rng), 0.1 + 0.15 * dist(rng)));
			}

			if (side == 0)
			{
				world.add(box(Vec3(-9 - depth, 0, z), Vec3(-9, height, z - length), mat));
				
			}

			else
			{
				world.add(box(Vec3(9, 0, z), Vec3(9 + depth, height, z - length), mat));

			}

			double wallX = (side == 0) ? -9 + 0.01 : 9 - 0.01;

			int count = int((length - 0.5) / 2.5);                 
			double totalWidth = count * 2.5 - 1.5;                    
			double startZ = z - (length - totalWidth) / 2;       

			for (int n = 0; n < count; n++)
			{
				double wz = startZ - n * 2.5;
				if (!isGlass)
				{
					for (double wy = 2.0; wy < height - 2; wy+= 2.5)
					{
						if (dist(rng) < 0.3)
						{
							world.add(std::make_shared<Quad>(Vec3(wallX, wy, wz), Vec3(0, 0, -1), Vec3(0, 1.5, 0), windowMat));
						}
						else
						{
							world.add(std::make_shared<Quad>(Vec3(wallX, wy, wz), Vec3(0, 0, -1), Vec3(0, 1.5, 0), darkWindowMat));
						} 
					}
				}
			}
			
			

			z = z - length - gap;

		}

		
		for (double lampZ = 5; lampZ > -105; lampZ -= 12)
		{
			
			if (side == 0)
			{
				double x = -7.5;
				world.add(box(Vec3(x - 0.1, 0.15, lampZ - 0.1), Vec3(x + 0.1, 4.5, lampZ + 0.1), poleMat));
				world.add(std::make_shared<Sphere>(Vec3(x, 4.7, lampZ), 0.6, lampMat));
			}

			else
			{
				double x = 7.5;
				world.add(box(Vec3(x - 0.1, 0.15, lampZ - 0.1), Vec3(x + 0.1, 4.5, lampZ + 0.1), poleMat));
				world.add(std::make_shared<Sphere>(Vec3(x, 4.7, lampZ), 0.6, lampMat));
			}
		}

		
	}
	

	bool leftSide = true;

	for (double puddleZ = -5; puddleZ > -95; puddleZ -= 10)
	{
		double puddleWidth = 2 + 2 * dist(rng);
		double puddleLength = 3 + 5 * dist(rng);
		double puddleX = (leftSide ? -6 : 0) + (6 - puddleWidth) * dist(rng);
		leftSide = !leftSide;
		world.add(std::make_shared<Quad>(Vec3(puddleX, 0.01, puddleZ), Vec3(puddleWidth, 0, 0), Vec3(0,0, -puddleLength), puddleMat));
	}

	auto oncoming = makeCar(redPaint, darkWindowMat, tireMat, headMat);
	world.add(std::make_shared<MovingTranslate>(oncoming, Vec3(-3.6, 0, -22), Vec3(-3.6, 0, -20.5)));
	auto leaving = makeCar(bluePaint, darkWindowMat, tireMat, rearLight);
	world.add(std::make_shared<MovingTranslate>(leaving, Vec3(1.8, 0, -12), Vec3(1.8, 0, -13.5)));

	auto parked1 = makeCar(greyPaint, darkWindowMat, tireMat, tireMat);
	world.add(std::make_shared<Translate>(parked1, Vec3(4.1, 0, -32)));     

	auto parked2 = makeCar(bluePaint, darkWindowMat, tireMat, tireMat);
	world.add(std::make_shared<Translate>(parked2, Vec3(4.1, 0, -60)));    

	auto parked3 = makeCar(redPaint, darkWindowMat, tireMat, tireMat);
	world.add(std::make_shared<Translate>(parked3, Vec3(-5.9, 0, -12)));    

	auto parked4 = makeCar(greyPaint, darkWindowMat, tireMat, tireMat);
	world.add(std::make_shared<Translate>(parked4, Vec3(-5.9, 0, -45)));    

	auto fogBounds = box(Vec3(-40, 0, -130), Vec3(40, 50, 15), white);
	world.add(std::make_shared<ConstantMedium>(fogBounds, 0.006, Vec3(0.6, 0.65, 0.8)));

	auto groundFog = box(Vec3(-40, 0, -130), Vec3(40, 2.5, 15), white);
	world.add(std::make_shared<ConstantMedium>(groundFog, 0.01, Vec3(0.7, 0.7, 0.8)));
	

	return world;
}

std::shared_ptr<HittableList> makeCar(std::shared_ptr<Material> paint, std::shared_ptr<Material> glass, std::shared_ptr<Material> tire, std::shared_ptr<Material> lights) 
{
	auto car = std::make_shared<HittableList>();
	auto paintMat = std::make_shared<Metal>(Vec3(0.3, 0.05, 0.05), 0.2);
	auto darkWindowMat = std::make_shared<Metal>(Vec3(0.1, 0.1, 0.12), 0.05);	
	
	car->add(box(Vec3(0, 0.3, 0), Vec3(1.8, 0.9, -4.5), paintMat));
	car->add(box( Vec3(0.15, 0.9, -1), Vec3(1.65, 1.4, - 3.5), darkWindowMat));
	car->add(std::make_shared<Sphere>(Vec3(0, 0.3, -1), 0.3, tire));
	car->add(std::make_shared<Sphere>(Vec3(1.8, 0.3, -1), 0.3, tire));
	car->add(std::make_shared<Sphere>(Vec3(0, 0.3, -3.5), 0.3, tire));
	car->add(std::make_shared<Sphere>(Vec3(1.8, 0.3, -3.5), 0.3, tire));

	return car;
	
}