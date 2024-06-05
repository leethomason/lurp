#pragma once

#include "scene.h"
#include "texture.h"
#include "tween.h"

#include <vector>

class TitleScene : public Scene
{
public:
	virtual ~TitleScene() {}

	virtual void load(Drawing&, const FrameData& f) override;
	virtual void draw(Drawing&, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

private:
	std::vector<std::shared_ptr<Texture>> _textures;
	std::vector<tween::Tween> _tweens;
};


