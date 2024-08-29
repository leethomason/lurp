#include "titlescene.h"
#include "config2d.h"
#include "tween.h"
#include "../platform.h"

void TitleScene::load(Drawing& d, const FrameData&)
{
	constexpr double RAMP = 0.2;
	//constexpr double HALF = RAMP / 2.0;
	constexpr double HOLD = 1.0;

	std::function<double(double)> func0 = tween::cosine;
	std::function<double(double)> func1 = tween::cosine;

	for(const auto& title : d.config.openingTitles) {
		std::filesystem::path p = lurp::ConstructAssetPath(d.config.config.assetsDirs, { title });
		if (p.empty())
			continue;
		d.textureManager.loadTexture(p);

		tween::Tween t(0.0);
		t.addASR(RAMP, HOLD, RAMP, 0.0, 1.0, func0, func1);
		_tweens.push_back(t);
	}
}

void TitleScene::draw(Drawing& d, const FrameData& f, const XFormer& xf)
{
	if (_textures.empty()) return;

	SDL_Rect dst = xf.sdlClipRect();

	for (size_t i = 0; i < _textures.size(); i++) {
		_tweens[i].tick(f.dt);
		std::shared_ptr<Texture> texture = _textures[i];
		double alpha = _tweens[i].value();
		Draw(d.renderer, texture, nullptr, &dst, RenderQuality::kFullscreen, alpha);
	}
	if (_tweens.empty() || _tweens.back().done())
		setState(State::kDone);
}

void TitleScene::layoutGUI(nk_context*, float, const XFormer&)
{
	// No GUI in the title scene
}
