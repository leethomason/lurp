#pragma once

#include "texture.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;
class Texture;
class XFormer;

class FontManager {
public:
	FontManager(TextureManager& textureManager) : _textureManager(textureManager) {}
	~FontManager();

	// At startup; fixed for the run of the program
	void loadFont(const std::string& name, const std::string& path, int size);

	void drawText(const std::string& font, Texture* textField, const std::string& text, SDL_Color color);

private:
	struct Font {
		TTF_Font* font = nullptr;
		int size = 0;
		std::string name;
	};
	TextureManager& _textureManager;
	std::vector<Font> _fonts;
};

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf);