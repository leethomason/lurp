#pragma once

#include "test.h"
#include "texture.h"
#include "text.h"
#include "scene.h"

class TextureManager;
class FontManager;
struct nk_context;

void RunTests2D();


class AssetsTest : public Scene
{
public:
	AssetsTest() = default;

	virtual void load(Drawing& d, const FrameData& f) override;
	virtual void draw(Drawing& d, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

	virtual void mouseMotion(FontManager&, const Point& screen, const Point& virt) override;
	virtual void mouseButton(FontManager&, const Point& screen, const Point& virt, bool down) override;

private:
	const SDL_Color clearColor = { 0, 179, 228, 255 };

	std::shared_ptr<Texture> portrait11, ps0, ps1, ps2, ps3, ps4, ps5, tree;
	std::shared_ptr<TextBox> tf0, tf1, tf2;
	VBox vbox;
};
