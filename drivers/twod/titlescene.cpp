#include "titlescene.h"
#include "config.h"
#include "tween.h"

void TitleScene::load(Drawing& d, const FrameData& f)
{
	constexpr double RAMP = 0.2;
	constexpr double HALF = RAMP / 2.0;
	constexpr double HOLD = 1.0;

	std::function<double(double)> func0 = tween::cosine;
	std::function<double(double)> func1 = tween::cosine;

	if (f.sceneFrame == 0 && d.config.openingTitles.size() > 0) {
		_textures.push_back(d.textureManager.loadTexture(d.config.assetsDir / d.config.openingTitles[0]));

		tween::Tween t(0.0);
		t.addASR(RAMP, HOLD, RAMP, 0.0, 1.0, func0, func1);
		_tweens.push_back(t);
	}
	else if (f.sceneFrame == 2) {
		for(size_t i=1; i<d.config.openingTitles.size(); i++) {
			_textures.push_back(d.textureManager.loadTexture(d.config.assetsDir / d.config.openingTitles[i]));

			tween::Tween t(0.0);
			t.add((RAMP * 2.0 + HOLD) * i - HALF, 0.0);
			t.addASR(RAMP, HOLD, RAMP, 0.0, 1.0, func0, func1);
			_tweens.push_back(t);
		}
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
	if (_tweens.back().done())
		setState(State::kDone);
}

void TitleScene::layoutGUI(nk_context*, float, const XFormer&)
{
	// No GUI in the title scene
}
