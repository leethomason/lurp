#include "texture.h"
#include "text.h"
#include "debug.h"
#include "xform.h"
#include "test2d.h"
#include "drawable.h"
#include "config.h"
#include "statemachine.h"
#include "../platform.h"

#include "nuk.h"
#include "argh.h"

#include "../platform.h"

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

int main(int argc, char* argv[])
{
	fmt::print("LuRP2D\n");
	fmt::print("Usage: lurp2d <path/to/lua/file> <starting-zone>\n");
	fmt::print("Options:\n");
	fmt::print("  -l, --log         Set log level: 'error', 'warning', 'info', 'debug'. Default: 'warning'\n");
	fmt::print("  --assets          Run assets test\n");

	argh::parser cmdl;
	cmdl.add_params({ "-l", "--log" });
	cmdl.parse(argc, argv);
	std::string scriptFile = cmdl[1];
	std::string startingZone = cmdl[2];

	std::string log = "warning";
	cmdl({ "-l", "--log" }, log) >> log;
	plog::Severity logLevel = plog::severityFromString(log.c_str());
	if (logLevel == plog::none) {
		logLevel = plog::warning;
	}

	bool doAssetsTest = cmdl[{"--assets" }];

	std::string dir = lurp::GameFileToDir(scriptFile);
	std::filesystem::path savePath = lurp::SavePath(dir, "saves");
	std::filesystem::path logPath = lurp::LogPath("lurp2d");
	fmt::print("Save path: {}\n", savePath.string());
	fmt::print("Log path: {}\n", logPath.string());

	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	static plog::RollingFileAppender<plog::TxtFormatter> fileAppender(logPath.string().c_str(), 1'000'000, 3);
	plog::init(logLevel, &consoleAppender).addAppender(&fileAppender);

	PLOG(plog::info) << "Logging started.";
	PLOG(plog::info) << "Save path: " << savePath.string();

	// SDL init - not included by memory tracker
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
		PLOG(plog::error) << "Could not initialize SDL2: " << SDL_GetError();
		FATAL_INTERNAL_ERROR();
	}
	if (TTF_Init()) {
		PLOG(plog::error) << "Could not initialize SDL TTF: " << SDL_GetError();
		FATAL_INTERNAL_ERROR();
	}

	// Get the display info...but it doesn't work.
	// On a modern high dpi display (at least on windows) the UI scale is getting applied
	// before any of these numbers. Only the GetDisplayMode() is correct. And even working around
	// all that isn't what I want - I want to address in actual pixels.
	// The upshot of all this is that text looks fuzzy - all the code to render things 1:1 is
	// getting bypassed, and it's really rendering 1:1.75. (Which is my display scale, not coincidentally.)
#if 0
	SDL_Rect displayBounds;
	int nDisplay = SDL_GetNumVideoDisplays();
	for (int i = 0; i < nDisplay; i++) {
		const char* name = SDL_GetDisplayName(i);
		SDL_GetDisplayBounds(0, &displayBounds);
		float dpi, hdpi, vdpi;
		SDL_GetDisplayDPI(i, &dpi, &hdpi, &vdpi);
		fmt::print("Display {} {}: {}x{} at {},{} dpi = {} {} {}\n", i, name, displayBounds.w, displayBounds.h, displayBounds.x, displayBounds.y, dpi, hdpi, vdpi);

		int nModes = 1;	// first is all that matters
		for (int j = 0; j < nModes; j++) {
			SDL_DisplayMode mode;
			SDL_GetDisplayMode(i, j, &mode);
			fmt::print("  Mode {}: {}x{} {}Hz\n", j, mode.w, mode.h, mode.refresh_rate);
		}
	}
#endif
#ifdef _WIN32
	{
		// Finally solved the HDPI issue on windows.
		// To get real 1:1 pixels, need this in cmake: set_target_properties(lurp2d PROPERTIES VS_DPI_AWARE "PerMonitor")

		SDL_DisplayMode desktop;
		SDL_GetDesktopDisplayMode(0, &desktop);

		SDL_DisplayMode mode;
		SDL_GetDisplayMode(0, 0, &mode);
		fmt::print("Desktop: {}x{}  Display: {}x{}\n", desktop.w, desktop.h, mode.w, mode.h);
		if (mode.w != desktop.w || mode.h != desktop.h) {
			fmt::print("WARNING: Virtual pixels. HDPI not 1:1\n");
		}
		else {
			fmt::print("HDPI is 1:1\n");
		}
	}
#endif

	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(0, &displayBounds);

	SDL_Window* window = SDL_CreateWindow(
		"LuRP",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		3 * displayBounds.w / 4, 3 * displayBounds.h / 4,
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
	fmt::print("SDL display = {}x{} window = {}x{} renderer= {}x{}\n", displayBounds.w, displayBounds.h, windowW, windowH, renderW, renderH);

	PLOG(plog::info) << "LuRP2D starting up.";
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	PLOG(plog::info) << "Compiled against SDL version " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch;
	PLOG(plog::info) << "Linked against SDL version " << (int)linked.major << "." << (int)linked.minor << "." << (int)linked.patch;
	int rc = -1;

#if defined(_DEBUG) && defined(_WIN32)
	// plog::init throws off memory tracking.
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		enki::TaskScheduler pool;
		pool.Initialize();

		RunTests2D();
		rc = TestReturnCode();
		LogTestResults();
		if (rc == 0)
			PLOG(plog::info) << "LuRP2D tests run successfully.";
		else
			PLOG(plog::error) << "LuRP2D tests failed.";

		if (scriptFile.empty()) {
			PLOG(plog::error) << "No script file specified.";
		}
		else {
			if (!std::filesystem::exists(scriptFile)) {
				PLOG(plog::error) << "Script file does not exist: " << scriptFile;
			}
			else {
				PLOG(plog::info) << "Script file: " << scriptFile;
			}
		}

		GameConfig gameConfig = GameConfig::demoConfig();
		gameConfig.assetsDir = "assets";
		gameConfig.validate();
		gameConfig.scriptFile = scriptFile.empty() ? "script/testzones.lua" : scriptFile;
		gameConfig.startingZone = startingZone;

		StateMachine machine(gameConfig);

		if (doAssetsTest) {
			gameConfig.virtualSize = { 800, 600 };
			machine.doTest();
		}

		XFormer xFormer(gameConfig.virtualSize.x, gameConfig.virtualSize.y);
		xFormer.setRenderSize(renderW, renderH);
		{
			SDL_Rect clip = xFormer.sdlClipRect();
			SDL_RenderSetClipRect(sdlRenderer, &clip);
			PLOG(plog::info) << fmt::format("SDL renderer = {}x{}", renderW, renderH);
		}

		SDL_RendererInfo rendererInfo;
		SDL_GetRendererInfo(sdlRenderer, &rendererInfo);
		PLOG(plog::info) << fmt::format("SDL renderer = {}  Texture max = {}x{}", rendererInfo.name, rendererInfo.max_texture_width, rendererInfo.max_texture_width);

		TextureManager textureManager(pool, sdlRenderer);
		std::shared_ptr<Texture> atlas = textureManager.loadTexture("assets/ascii.png");

		FontManager fontManager(sdlRenderer, pool, textureManager, SCREEN_WIDTH, SCREEN_HEIGHT);

		//AssetsTest assetsTest;
		//IDrawable* iAssetsTests = &assetsTest;

		nk_context* nukCtx = nk_sdl_init(window, sdlRenderer);
		const float nukFontBaseSize = 16.0f;
		NukFontAtlas nukFontAtlas(nukCtx);
		nukFontAtlas.load("assets/Roboto-Regular.ttf", { 12.f, 16.f, 24.f, 32.f, 48.f, 64.f });

		// ---------- Initialization done -----------
		lurp::RollingAverage<uint64_t, 48> innerAve;
		lurp::RollingAverage<uint64_t, 48> frameAve;

		FrameData frameData;
		Drawing drawing(sdlRenderer, textureManager, fontManager, gameConfig);

		//if (doAssetsTest) {
		//	iAssetsTests->load(drawing, frameData);
		//}
		//else {
			gameConfig.font = fontManager.loadFont("assets/Roboto-Regular.ttf", 24);	// fixme: hardcode. should be in config.
		//}

		bool done = false;
		SDL_Event e;
		uint64_t lastFramePC = SDL_GetPerformanceCounter();
		uint64_t lastFrameMillis = SDL_GetTicks64();

		// ---------- Main Loop ------------ //
		while (!done && !machine.done()) {
			uint64_t startFramePC = SDL_GetPerformanceCounter();
			uint64_t startMillis = SDL_GetTicks64();

			int oldRenderW = renderW, oldRenderH = renderH;
			SDL_GetRendererOutputSize(sdlRenderer, &renderW, &renderH);
			xFormer.setRenderSize(renderW, renderH);
			{
				SDL_Rect clip = xFormer.sdlClipRect();
				SDL_RenderSetClipRect(sdlRenderer, &clip);
				if (oldRenderW != renderW || oldRenderH != renderH) {
					fmt::print("SDL renderer= {}x{}\n", renderW, renderH);
					PLOG(plog::info) << fmt::format("SDL renderer = {}x{}", renderW, renderH);
				}
			}
			
			frameData.sceneTime += startMillis - frameData.timeMillis;
			frameData.timeMillis = startMillis;
			frameData.time = frameData.timeMillis / 1000.0;
			frameData.dt = (startMillis - lastFrameMillis) / 1000.0;

			std::shared_ptr<Scene> scene = machine.tick(&frameData);
			if (scene)
				scene->load(drawing, frameData);

			textureManager.update();
			fontManager.update(xFormer);

			nk_input_begin(nukCtx);
			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					done = true;
				}
				else if (e.type == SDL_KEYDOWN) {
					switch (e.key.keysym.sym) {
					case SDLK_t:
						fontManager.toggleQuality();
						break;
					case SDLK_1:	SDL_SetWindowSize(window, 800, 600);	break;
					case SDLK_2:	SDL_SetWindowSize(window, 960, 540);	break;
					case SDLK_3:	SDL_SetWindowSize(window, 1024, 768);	break;
					case SDLK_4:	SDL_SetWindowSize(window, 1920, 1080);	break;
					case SDLK_f: {
						uint32_t flag = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
						SDL_SetWindowFullscreen(window, flag ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
						break;
					}
					default:
						break;
					}
				}
				else if (e.type == SDL_MOUSEMOTION) {
					Point screen = { e.motion.x, e.motion.y };
					Point virt = xFormer.screenToVirtual(screen);
					scene->mouseMotion(fontManager, screen, virt);
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
					if (e.button.button == 1) {
						Point screen = { e.button.x, e.button.y };
						Point virt = xFormer.screenToVirtual(screen);
						scene->mouseButton(fontManager, screen, virt, e.type == SDL_MOUSEBUTTONDOWN);
					}
				}
				nk_sdl_handle_event(&e);
			}
			//nk_sdl_handle_grab();	// FIXME: do we want grab? why isn't it defined?
			nk_input_end(nukCtx);

			/* GUI */

			// FIXME: figure out the gui font size
			if (doAssetsTest) {
				float realFontSize = 0;
				nk_font* nukFontBest = nukFontAtlas.select(xFormer.s(nukFontBaseSize), &realFontSize);
				nk_style_set_font(nukCtx, &nukFontBest->handle);
				scene->layoutGUI(nukCtx, realFontSize, xFormer);
			}
			else {
				float realFontSize = 0;
				nk_font* nukFontBest = nukFontAtlas.select(xFormer.s(48.f), &realFontSize);
				nk_style_set_font(nukCtx, &nukFontBest->handle);
				scene->layoutGUI(nukCtx, realFontSize, xFormer);
			}

			//const SDL_Color drawColor = { 0, 179, 228, 255 };
			SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
			SDL_RenderClear(sdlRenderer);	// ignores clipping (which is good)

			if (doAssetsTest) {
				DrawTestPattern(sdlRenderer,
					800, SCREEN_HEIGHT, 16,
					SDL_Color{ 192, 192, 192, 255 }, SDL_Color{ 128, 128, 128, 255 },
					xFormer);
			}
			scene->draw(drawing, frameData, xFormer);

			// Sample *before* the present to exclude vsync. Also exclude the time to render the debug text.
			uint64_t endFrameTime = SDL_GetPerformanceCounter();
			innerAve.add(endFrameTime - startFramePC);
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
			frameData.frame++;
			frameData.sceneFrame++;
			lastFrameMillis = startMillis;

			// Sample *after* the present to include vsync
			uint64_t now = SDL_GetPerformanceCounter();
			frameAve.add(now - lastFramePC);
			lastFramePC = now;
		}
		// ---------- Tear Down ------- //
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
	return rc;
}