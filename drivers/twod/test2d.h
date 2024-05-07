#pragma once

#include "test.h"
#include "texture.h"
#include "text.h"
#include "drawable.h"

class TextureManager;
class FontManager;
struct nk_context;

void RunTests2D();


class AssetsTest : public IDrawable
{
public:
	AssetsTest(SDL_Renderer* renderer, TextureManager& textureManager, FontManager& fontManager) : _renderer(renderer), _textureManager(textureManager), _fontManager(fontManager) {}

	virtual void load() override;
	virtual void draw(const XFormer& xformer, uint64_t frame) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer, uint64_t frame) override;

private:
	SDL_Renderer* _renderer;
	TextureManager& _textureManager;
	FontManager& _fontManager;

	const SDL_Color clearColor = { 0, 179, 228, 255 };

	std::shared_ptr<Texture> portrait11, ps0, ps1, ps2, ps3, ps4, ps5, tree;
	std::shared_ptr<TextField> tf0, tf1;
};
