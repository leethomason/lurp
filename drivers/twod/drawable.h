#pragma once

#include <stdint.h>

class XFormer;
class TextureManager;
class FontManager;
struct nk_context;
struct SDL_Renderer;
struct GameConfig2D;

struct Drawing {
	Drawing(SDL_Renderer* renderer, TextureManager& textureManager, FontManager& fontManager, const GameConfig2D& config) : renderer(renderer), textureManager(textureManager), fontManager(fontManager), config(config) {}

	SDL_Renderer* renderer;
	TextureManager& textureManager; 
	FontManager& fontManager;
	const GameConfig2D& config;
};

struct FrameData {
	uint64_t frame = 0;
	uint64_t sceneFrame = 0;
	uint64_t timeMillis = 0;
	uint64_t sceneTime = 0;
	double time = 0.0;
	double dt = 0.0;
};

class IDrawable
{
public:
	virtual void load(Drawing& d, const FrameData& f) = 0;
	virtual void draw(Drawing& d, const FrameData& f, const XFormer& xformer) = 0;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) = 0;
};

