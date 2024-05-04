#include <SDL.h>
#include <stdio.h>
#include <SDL_image.h>

#include "texture.h"

/*
	Build is working! At least on windows.
	Test render.
	x basic event loop
	- basic window
		1080p:  1920 x 1080
		4k: 	3840 x 2160 
		16:9

		- manage coordinates
		- manage aspect ratio
	- draw a texture or two.
		- get sRGB correct
		x get thread pool working
		x get async loading
	- draw debug text
	    - time to render
		- time between frames
	- draw text
		- font loading
		- render on a thread 
		- sync eveything up
		- manage resources
	- memory tracking
*/

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

int main(int argc, char* args[]) 
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Window* window = SDL_CreateWindow(
		"LuRP",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN
	);
	if (window == NULL) {
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawColor(sdlRenderer, 128, 128, 128, 255);

	enki::TaskScheduler pool;
	pool.Initialize();

	TextureManager textureManager(pool, sdlRenderer);
	const Texture* atlas = textureManager.loadTexture("ascii", "assets/ascii.png");

	bool done = false;
	SDL_Event e;
	while (!done) {
		textureManager.update();

		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				done = true;
			}
		}

		SDL_RenderClear(sdlRenderer);

		if (atlas->ready())
			SDL_RenderCopy(sdlRenderer, atlas->getSDLTexture(), NULL, NULL);

		//Update screen
		SDL_RenderPresent(sdlRenderer);
	}
	textureManager.freeAll();

	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}