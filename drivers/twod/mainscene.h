#pragma once

#include "scene.h"
#include "texture.h"

class MainScene : public Scene
{
public:
	virtual ~MainScene() {}

	virtual void load(Drawing&, const FrameData& f) override;
	virtual void draw(Drawing&, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

private:
	std::shared_ptr<Texture> _texture;
};


