#include "text.h"
#include "texture.h"

#include <SDL.h>

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize)
{
	if (!tex->ready())
		return;

	constexpr int WIDTH = 512;
	constexpr int HEIGHT = 256;
	constexpr int NX = 16;
	constexpr int NY = 8;
	constexpr int DX = WIDTH / NX;
	constexpr int DY = HEIGHT / NY;

	const int xStep = DX * fontSize / 32;
	const int yStep = DY * fontSize / 32;

	for (size_t i = 0; i < text.size(); i++)
	{
		SDL_Rect src;
		int pos= text[i];
		src.x = (pos % NX) * DX;
		src.y = (pos / NX) * DY;
		src.w = DX;
		src.h = DY;

		SDL_Rect dst;
		dst.x = x + int(i) * xStep;
		dst.y = y;
		dst.w = xStep;
		dst.h = yStep;

		SDL_RenderCopy(renderer, tex->sdlTexture(), &src, &dst);
	};
}