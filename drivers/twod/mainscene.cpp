#include "mainscene.h"
#include "config.h"
#include "drawable.h"

#include "nuk.h"

void MainScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame == 0)
		_texture = d.textureManager.loadTexture(d.config.assetsDir / d.config.mainBackground);
}

void MainScene::draw(Drawing& d, const FrameData&, const XFormer& xf)
{
	if (!_texture->ready()) return;
	SDL_Rect dst = xf.sdlClipRect();
	SDL_RenderCopy(d.renderer, _texture->sdlTexture(), nullptr, &dst);
}

void MainScene::layoutGUI(nk_context* ctx, float fontSize, const XFormer& xf)
{
#if 0
	RectF guiRect = xf.t(RectF{ 560, 20, 230, 250 });
	const float height = fontSize * 1.8f;

	if (nk_begin(ctx, "Demo", nk_rect(guiRect.x, guiRect.y, guiRect.w, guiRect.h),
		NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{

	}
#endif
}
