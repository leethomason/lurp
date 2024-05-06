#include "texture.h"
#include "text.h"
#include "debug.h"
#include "xform.h"
#include "test2d.h"

#include "nuk.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <SDL_image.h>

#include <fmt/core.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>

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

	SDL_Window* window = SDL_CreateWindow(
		"LuRP",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE //| SDL_WINDOW_ALLOW_HIGHDPI //| SDL_WINDOW_FULLSCREEN_DESKTOP
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


#if defined(_DEBUG) && defined(_WIN32)
	// plog::init throws off memory tracking.
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		enki::TaskScheduler pool;
		pool.Initialize();

		RunTests2D();
		int rc = TestReturnCode();
		LogTestResults();
		if (rc == 0)
			PLOG(plog::info) << "LuRP2D tests run successfully.";
		else
			PLOG(plog::error) << "LuRP2D tests failed.";

		XFormer xFormer(SCREEN_WIDTH, SCREEN_HEIGHT);
		xFormer.setRenderSize(renderW, renderH);
		{
			SDL_Rect clip = xFormer.sdlClipRect();
			SDL_RenderSetClipRect(sdlRenderer, &clip);
		}

		TextureManager textureManager(pool, sdlRenderer);
		// fixme: think about asset names
		std::shared_ptr<Texture> atlas = textureManager.loadTexture("ascii", "assets/ascii.png");

		FontManager fontManager(sdlRenderer, pool, textureManager, SCREEN_WIDTH, SCREEN_HEIGHT);
		fontManager.loadFont("roboto16", "assets/Roboto-Regular.ttf", 16);
		//fontManager.loadFont("roboto16", "assets/Lora-Medium.ttf", 16);

		AssetsTest assetsTest(sdlRenderer, textureManager, fontManager);
		assetsTest.load();
		
		lurp::RollingAverage<uint64_t, 48> innerAve;
		lurp::RollingAverage<uint64_t, 48> frameAve;

		nk_context* nukCtx = nk_sdl_init(window, sdlRenderer);
		const float nukFontBaseSize = 16.0f;
		NukFontAtlas nukFontAtlas(nukCtx);
		nukFontAtlas.load("assets/Roboto-Regular.ttf", { 12.f, 16.f, 24.f, 32.f, 48.f, 64.f });

		bool done = false;
		SDL_Event e;
		uint64_t last = SDL_GetPerformanceCounter();
		uint64_t frame = 0;

		while (!done) {
			uint64_t start = SDL_GetPerformanceCounter();

			SDL_GetRendererOutputSize(sdlRenderer, &renderW, &renderH);
			xFormer.setRenderSize(renderW, renderH);
			{
				SDL_Rect clip = xFormer.sdlClipRect();
				SDL_RenderSetClipRect(sdlRenderer, &clip);
			}

			textureManager.update();
			fontManager.update(xFormer);

			nk_input_begin(nukCtx);
			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					done = true;
				}
				else if (e.type == SDL_WINDOWEVENT) {
					fmt::print("SDL renderer= {}x{}\n", renderW, renderH);
				}
				nk_sdl_handle_event(&e);
			}
			//nk_sdl_handle_grab();	// FIXME: do we want grab? why isn't it defined?
			nk_input_end(nukCtx);

			/* GUI */
			nk_font* nukFontBest = nukFontAtlas.select(xFormer.s(nukFontBaseSize));
			nk_style_set_font(nukCtx, &nukFontBest->handle);

			assetsTest.drawGUI(nukCtx, xFormer, frame);

			const SDL_Color drawColor = { 0, 179, 228, 255 };
			SDL_SetRenderDrawColor(sdlRenderer, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
			SDL_RenderClear(sdlRenderer);

			DrawTestPattern(sdlRenderer, 
				380, SCREEN_HEIGHT, 16, 
				SDL_Color{192, 192, 192, 255}, SDL_Color{128, 128, 128, 255},
				xFormer);

			assetsTest.draw(xFormer, frame);

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
				DrawDebugText(msg, sdlRenderer, atlas.get(), 5, 5, 12, xFormer);
			}

			// Update screen
			nk_sdl_render(NK_ANTI_ALIASING_ON);
			SDL_RenderPresent(sdlRenderer);
			++frame;

			// Sample *after* the present to include vsync
			uint64_t now = SDL_GetPerformanceCounter();
			if (now > last)
				frameAve.add(now - last);
			last = now;
		}
		pool.WaitforAll();	// flush out texture loads in flight
		textureManager.freeAll();
		nk_sdl_shutdown();
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