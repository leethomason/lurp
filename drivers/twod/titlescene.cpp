#include "titlescene.h"
#include "config.h"

void TitleScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame == 0) {
		if (d.config.openingTitles.size() > 0)
			_textures.push_back(d.textureManager.loadTexture(d.config.assetsDir / d.config.openingTitles[0]));
	}
	else if (f.sceneFrame == 2) {
		for(size_t i=1; i<d.config.openingTitles.size(); i++) {
			_textures.push_back(d.textureManager.loadTexture(d.config.assetsDir / d.config.openingTitles[i]));
		}
	}
}

void TitleScene::draw(Drawing& d, const FrameData& f, const XFormer& xf)
{
	if (_textures.empty()) return;
	std::shared_ptr<Texture> texture = _textures[std::min(f.sceneTime / 1000, _textures.size() - 1)];
	if (texture->ready()) {
		SDL_Rect dst = xf.sdlClipRect();
		SDL_RenderCopy(d.renderer, texture->sdlTexture(), nullptr, &dst);
	}
	if (f.sceneTime / 1000 > _textures.size())
		setState(State::kDone);
}

void TitleScene::layoutGUI(nk_context*, float, const XFormer&)
{
	// No GUI in the title scene
}
