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
	SDL_SetTextureScaleMode(_texture->sdlTexture(), SDL_ScaleMode::SDL_ScaleModeBest);
	SDL_RenderCopy(d.renderer, _texture->sdlTexture(), nullptr, &dst);
}

void MainScene::layoutGUI(nk_context* ctx, float realFontSize, const XFormer& xf)
{
#if 1
	RectF guiRect = xf.tf(0.4, 0.5, 0.2, 0.4);
	const float height = realFontSize * 1.8f;

	// NK_WINDOW_BORDER 

	struct nk_style* s = &ctx->style;
	nk_style_push_color(ctx, &s->window.background, nk_rgba(0, 0, 0, 0));
	nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));

	if (nk_begin(ctx, "Main", nk_rect(guiRect.x, guiRect.y, guiRect.w, guiRect.h), NK_WINDOW_NO_SCROLLBAR /* | NK_WINDOW_BACKGROUND*/))
	{
		/* 0.2 are a space skip on button's left and right, 0.6 - size of the button */
		//static const float ratio[] = { 0.2f, 0.6f, 0.2f };  /* 0.2 + 0.6 + 0.2 = 1 */

		nk_layout_row_dynamic(ctx, height, 1);
		if (nk_button_label(ctx, "Settings")) {
			//setState(State::kSettings);
		}
		if (nk_button_label(ctx, "Save/Load")) {
			//setState(State::kSaveLoad);
		}
		if (nk_button_label(ctx, "Quit")) {
			//setState(State::kQuit);
		}
	}
	nk_end(ctx);

	nk_style_pop_style_item(ctx);
	nk_style_pop_color(ctx);
#endif
}
