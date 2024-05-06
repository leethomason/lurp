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


/*
	Build is working! At least on windows.
	Test render.
	x basic event loop
	x basic window
		x manage coordinates
		x manage aspect ratio
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
		x manage resources
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
		std::shared_ptr<Texture> atlas = textureManager.loadTexture("ascii", "assets/ascii.png");
		std::shared_ptr<Texture> portrait11 = textureManager.loadTexture("portrait11", "assets/portraitTest11.png");
		std::shared_ptr<Texture> ps0 = textureManager.loadTexture("ps-back", "assets/back.png");
		std::shared_ptr<Texture> ps1 = textureManager.loadTexture("ps-layer1", "assets/layer1_100.png");
		std::shared_ptr<Texture> ps2 = textureManager.loadTexture("ps-layer2", "assets/layer2_100.png");
		std::shared_ptr<Texture> ps3 = textureManager.loadTexture("ps-layer3", "assets/layer3_100.png");
		std::shared_ptr<Texture> ps4 = textureManager.loadTexture("ps-layer4", "assets/layer4_100.png");
		std::shared_ptr<Texture> ps5 = textureManager.loadTexture("ps-layer5", "assets/layer5_100.png");
		std::shared_ptr<Texture> tree = textureManager.loadTexture("tree", "assets/tree.png");

		FontManager fontManager(sdlRenderer, pool, textureManager, SCREEN_WIDTH, SCREEN_HEIGHT);
		fontManager.loadFont("roboto16", "assets/Roboto-Regular.ttf", 22);
		std::shared_ptr<TextField> tf0 = fontManager.createTextField("textField0", "roboto16", 300, 600);

		lurp::RollingAverage<uint64_t, 48> innerAve;
		lurp::RollingAverage<uint64_t, 48> frameAve;

		nk_context* nukCtx = nk_sdl_init(window, sdlRenderer);
		nk_font_atlas* nukAtlas = nullptr;
		struct nk_font_config nukConfig = nk_font_config(0);
		float nukFontBaseSize = 16.0f;
		//float nukFontMult = 0.50f;

		nk_sdl_font_stash_begin(&nukAtlas);
		//nk_font* nukFontDefault16 = nk_font_atlas_add_default(nukAtlas, 16.0, &nukConfig);
		nk_font* nukFont12 = nk_font_atlas_add_from_file(nukAtlas, "assets/Roboto-Regular.ttf", 12.0f, &nukConfig);
		nk_font* nukFont16 = nk_font_atlas_add_from_file(nukAtlas, "assets/Roboto-Regular.ttf", 16.0f, &nukConfig);
		nk_font* nukFont32 = nk_font_atlas_add_from_file(nukAtlas, "assets/Roboto-Regular.ttf", 32.0f, &nukConfig);
		nk_sdl_font_stash_end();
		nk_style_set_font(nukCtx, &nukFont16->handle);

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
			// nuklear "endorsed" hack:
			//nukFont->handle.height = xFormer.s(nukFontBaseSize * nukFontMult);

			float fontSize = xFormer.s(nukFontBaseSize);
			float fontErr12 = fabs(fontSize - 12.0f);
			float fontErr16 = fabs(fontSize - 16.0f);
			float fontErr32 = fabs(fontSize - 32.0f);

			if (fontErr12 < fontErr16 && fontErr12 < fontErr32)
				nk_style_set_font(nukCtx, &nukFont12->handle);
			else if (fontErr16 < fontErr12 && fontErr16 < fontErr32)
				nk_style_set_font(nukCtx, &nukFont16->handle);
			else
				nk_style_set_font(nukCtx, &nukFont32->handle);

			RectF guiRect = xFormer.t(RectF{ 560, 20, 230, 250 });
			if (nk_begin(nukCtx, "Demo", nk_rect(guiRect.x, guiRect.y, guiRect.w, guiRect.h),
				NK_WINDOW_BORDER | // NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
				NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
			{
				enum { EASY, HARD };
				static int op = EASY;
				static int property = 20;
				nk_colorf bg{ 0, 0, 0, 0 };

				nk_layout_row_static(nukCtx, xFormer.s(30.f), xFormer.s(80), 1);
				if (nk_button_label(nukCtx, "button"))
					fprintf(stdout, "button pressed\n");
				nk_layout_row_dynamic(nukCtx, 30, 2);
				if (nk_option_label(nukCtx, "easy", op == EASY)) op = EASY;
				if (nk_option_label(nukCtx, "hard", op == HARD)) op = HARD;
				nk_layout_row_dynamic(nukCtx, xFormer.s(25.f), 1);
				// fixme: doesn't slide with the correct scale
				nk_property_int(nukCtx, "Compression:", 0, &property, 100, 10, 1.0f);

				nk_layout_row_dynamic(nukCtx, xFormer.s(20.f), 1);
				nk_label(nukCtx, "background:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(nukCtx, xFormer.s(25.f), 1);
				/*if (nk_combo_begin_color(nukCtx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(nukCtx), 400))) {
					nk_layout_row_dynamic(nukCtx, 120, 1);
					bg = nk_color_picker(nukCtx, bg, NK_RGBA);
					nk_layout_row_dynamic(nukCtx, 25, 1);
					bg.r = nk_propertyf(nukCtx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
					bg.g = nk_propertyf(nukCtx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
					bg.b = nk_propertyf(nukCtx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
					bg.a = nk_propertyf(nukCtx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
					nk_combo_end(nukCtx);
				}*/
			}
			nk_end(nukCtx);

			SDL_SetRenderDrawColor(sdlRenderer, 0, 179, 228, 255);
			SDL_RenderClear(sdlRenderer);
			DrawTestPattern(sdlRenderer, 
				380, SCREEN_HEIGHT, 16, 
				SDL_Color{192, 192, 192, 255}, SDL_Color{128, 128, 128, 255},
				xFormer);

			// Draw a texture. Confirm sRGB is working.
			if (portrait11->ready()) {
				SDL_Rect dest = xFormer.t(SDL_Rect{ 0, 0, portrait11->width(), portrait11->height() });
				SDL_RenderCopy(sdlRenderer, portrait11->sdlTexture(), nullptr, &dest);
			}

			// Test against Ps
			if (Texture::ready({ ps0, ps1, ps2, ps3, ps4, ps5 })) {
				SDL_Rect dest = xFormer.t(SDL_Rect{ 300, 0, 256, 256 });
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
				SDL_Rect dest = xFormer.t(SDL_Rect{ 400, 300, 400, 400 });
				SDL_RenderCopy(sdlRenderer, tree->sdlTexture(), nullptr, &dest);

				for (int i = 0; i < 3; i++) {
					SDL_Rect r = xFormer.t(SDL_Rect{ i * 100, 400, 50 + 50 * i, 50 + 50 * i });
					SDL_RenderCopy(sdlRenderer, tree->sdlTexture(), nullptr, &r);
				}
			}
			{
				std::string text = fmt::format("Hello, world! This is some text that will need to be wrapped to fit in the box. frame/60={}", frame/60);
				tf0->Render(text, 400, 300, SDL_Color{255, 255, 255, 255});
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