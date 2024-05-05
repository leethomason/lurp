#include "texture.h"
#include "text.h"
#include "debug.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <SDL_image.h>

#define NK_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#include <fmt/core.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>


/*
	Build is working! At least on windows.
	Test render.
	x basic event loop
	- basic window
	    sd:      960 x 540
		1080p:  1920 x 1080
		4k: 	3840 x 2160 
		16:9

		- manage coordinates
		- manage aspect ratio
	x draw a texture or two.
		x get sRGB correct
		x get thread pool working
		x get async loading
	x draw debug text
	    x time to render
		x time between frames
	x draw text
		x font loading
		x render on a thread 
		x sync eveything up
		- manage resources
	x memory tracking
	- nuklear
*/

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

int main(int argc, char* args[])
{
	(void)argc;
	(void)args;

	// SDL init - not included by memory tracker
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
		return 1;
	}
	if (TTF_Init()) {
		fprintf(stderr, "could not initialize sdl2_ttf: %s\n", TTF_GetError());
		return 1;
	}

	// SDL_RenderSetLogicalSize: maybe?
	// SDL_RenderSetViewport: maybe?

	SDL_Window* window = SDL_CreateWindow(
		"LuRP",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN  //| SDL_WINDOW_ALLOW_HIGHDPI //| SDL_WINDOW_FULLSCREEN_DESKTOP
	);
	if (!window) {
		PLOG(plog::error) << "Could not create window (SDL_CreateWindow failed): " << SDL_GetError();
		FATAL_INTERNAL_ERROR();
	}
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!sdlRenderer) {
		PLOG(plog::error) << "Could not create window (SDL_CreateRenderer failed): " << SDL_GetError();
		FATAL_INTERNAL_ERROR();
	}

	int windowW, windowH, renderW, renderH;
	SDL_GetWindowSize(window, &windowW, &windowH);
	SDL_GetRendererOutputSize(sdlRenderer, &renderW, &renderH);
	fmt::print("SDL window = {}x{} renderer= {}x{}\n", windowW, windowH, renderW, renderH);

	//SDL_Rect view = { 0, 100, 800, 500 };
	//SDL_RenderSetViewport(sdlRenderer, &view);
	// or clip rect. Hmm.

#if defined(_DEBUG) && defined(_WIN32)
	// plog::init throws off memory tracking.
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		enki::TaskScheduler pool;
		pool.Initialize();

		TextureManager textureManager(pool, sdlRenderer);
		const Texture* atlas = textureManager.loadTexture("ascii", "assets/ascii.png");
		textureManager.loadTexture("portrait11", "assets/portraitTest11.png");
		const Texture* ps0 = textureManager.loadTexture("ps-back", "assets/back.png");
		const Texture* ps1 = textureManager.loadTexture("ps-layer1", "assets/layer1_100.png");
		const Texture* ps2 = textureManager.loadTexture("ps-layer2", "assets/layer2_100.png");
		const Texture* ps3 = textureManager.loadTexture("ps-layer3", "assets/layer3_100.png");
		const Texture* ps4 = textureManager.loadTexture("ps-layer4", "assets/layer4_100.png");
		const Texture* ps5 = textureManager.loadTexture("ps-layer5", "assets/layer5_100.png");
		const Texture* tree = textureManager.loadTexture("tree", "assets/tree.png");
		Texture* tf0 = textureManager.createTextField("textField0", 300, 600);

		FontManager fontManager(textureManager);
		fontManager.loadFont("roboto16", "assets/Roboto-Regular.ttf", 22);

		lurp::RollingAverage<uint64_t, 48> innerAve;
		lurp::RollingAverage<uint64_t, 48> frameAve;

		bool done = false;
		SDL_Event e;
		uint64_t last = SDL_GetPerformanceCounter();

		std::string textField = "Hello, world! This is some text that will need to be wrapped to fit in the box.";
		fontManager.drawText("roboto16", tf0, textField, SDL_Color{ 255, 255, 255, 255 });

		while (!done) {
			uint64_t start = SDL_GetPerformanceCounter();

			textureManager.update();

			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					done = true;
				}
			}

			SDL_SetRenderDrawColor(sdlRenderer, 0, 179, 228, 255);
			SDL_RenderClear(sdlRenderer);
			DrawTestPattern(sdlRenderer, 380, SCREEN_HEIGHT, 16, Color{ 192, 192, 192, 255 }, Color{ 128, 128, 128, 255 });

			// Draw a texture. Confirm sRGB is working.
			const Texture* t = textureManager.getTexture("portrait11");
			if (t->ready()) {
				SDL_Rect dest = { 0, 0, t->width(), t->height() };
				SDL_RenderCopy(sdlRenderer, t->sdlTexture(), nullptr, &dest);
			}

			// Test against Ps
			if (Texture::ready({ ps0, ps1, ps2, ps3, ps4, ps5 })) {
				SDL_Rect dest = { 400, 0, 256, 256 };
				//SDL_RenderBlend
				SDL_RenderCopy(sdlRenderer, ps0->sdlTexture(), nullptr, &dest);
				SDL_RenderCopy(sdlRenderer, ps1->sdlTexture(), nullptr, &dest);
				SDL_SetTextureAlphaMod(ps2->sdlTexture(), 128);
				SDL_RenderCopy(sdlRenderer, ps2->sdlTexture(), nullptr, &dest);
				SDL_RenderCopy(sdlRenderer, ps3->sdlTexture(), nullptr, &dest);
				SDL_SetTextureAlphaMod(ps4->sdlTexture(), 128);
				SDL_RenderCopy(sdlRenderer, ps4->sdlTexture(), nullptr, &dest);
				SDL_SetTextureAlphaMod(ps5->sdlTexture(), 128);
				SDL_RenderCopy(sdlRenderer, ps5->sdlTexture(), nullptr, &dest);
			}
			if (tree->ready()) {
				SDL_Rect dest = { 400, 300, 400, 400 };
				SDL_RenderCopy(sdlRenderer, tree->sdlTexture(), nullptr, &dest);

				for (int i = 0; i < 3; i++) {
					SDL_Rect r = { i * 100, 400, 50 + 50 * i, 50 + 50 * i };
					SDL_RenderCopy(sdlRenderer, tree->sdlTexture(), nullptr, &r);
				}
			}
			if (tf0->ready()) {
				SDL_Rect dest = { 400, 300, tf0->width(), tf0->height() };
				SDL_SetTextureBlendMode(tf0->sdlTexture(), SDL_BLENDMODE_BLEND);
				SDL_RenderCopy(sdlRenderer, tf0->sdlTexture(), nullptr, &dest);
			}

			// Sample *before* the present to exclude vsync. Also exclude the time to render the debug text.
			uint64_t end = SDL_GetPerformanceCounter();
			if (end > start) {
				innerAve.add(end - start);
			}
			{
				uint64_t microInner = innerAve.average() * 1'000'000 / SDL_GetPerformanceFrequency();
				uint64_t microFrame = frameAve.average() * 1'000'000 / SDL_GetPerformanceFrequency();
				std::string msg = fmt::format("Inner: {:.2f} Frame: {:.2f} ms Textures: {}/{} {}/{} MBytes",
					microInner / 1000.0,
					microFrame / 1000.0,
					textureManager.numTexturesReady(),
					textureManager.numTextures(),
					textureManager.totalTextureMemory() / 1'000'000,
					textureManager.readyTextureMemory() / 1'000'000);
				DrawDebugText(msg, sdlRenderer, atlas, 5, 5, 12);
			}

			//Update screen
			SDL_RenderPresent(sdlRenderer);

			// Sample *after* the present to include vsync
			uint64_t now = SDL_GetPerformanceCounter();
			if (now > last)
				frameAve.add(now - last);
			last = now;
		}
		textureManager.freeAll();
	}
#if defined(_DEBUG) && defined(_WIN32)
	int knownNumLeak = 0;
	int knownLeakSize = 0;

	_CrtMemCheckpoint(&s2);
	_CrtMemDifference(&s3, &s1, &s2);
	_CrtMemDumpStatistics(&s3);

	auto leakCount = s3.lCounts[1];
	auto leakSize = s3.lSizes[1];
	auto totalAllocaton = s3.lTotalCount;
	auto highWater = s3.lHighWaterCount / 1024;
	plog::Severity severity = leakCount ? plog::warning : plog::info;

	PLOG(severity) << "Memory report:";
	PLOG(severity) << "Leak count = " << leakCount << " size = " << leakSize;
	PLOG(severity) << "High water = " << highWater << " total allocated = " << totalAllocaton;
	assert(s3.lCounts[1] <= knownNumLeak);
	assert(s3.lSizes[1] <= knownLeakSize);
#endif

	TTF_Quit();
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}