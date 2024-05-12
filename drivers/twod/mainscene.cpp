#include "mainscene.h"
#include "config.h"
#include "drawable.h"

#include "nuk.h"

void MainScene::load(Drawing& d, const FrameData& f)
{
	constexpr double RAMP = 0.2;

	if (f.sceneFrame == 0) {
		_texture = d.textureManager.loadTexture(d.config.assetsDir / d.config.mainBackground);
		_tween.add(RAMP, 1.0, tween::cosine);
	}
}

void MainScene::draw(Drawing& d, const FrameData& f, const XFormer& xf)
{
	if (!_texture->ready()) return;
	SDL_Rect dst = xf.sdlClipRect();
	_tween.tick(f.dt);
	double alpha = _tween.value();
	Draw(d.renderer, _texture, nullptr, &dst, RenderQuality::kFullscreen, alpha);
}

void MainScene::layoutGUI(nk_context* ctx, float realFontSize, const XFormer& xf)
{
	const float height = realFontSize * 1.8f;
	const float realWidth = height * 5.0f;
	const float realHeight = 1000.0f;
	const PointF center = xf.tf(0.5f, 0.5f);
	RectF guiRect{center.x - realWidth / 2, center.y, realWidth, realHeight};

	struct nk_style* s = &ctx->style;
	nk_style_push_color(ctx, &s->window.background, nk_rgba(0, 0, 0, 0));
	nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));

	if (nk_begin(ctx, "Main", nk_rect(guiRect.x, guiRect.y, guiRect.w, guiRect.h), NK_WINDOW_NO_SCROLLBAR /* | NK_WINDOW_BACKGROUND*/))
	{
		nk_layout_row_dynamic(ctx, height, 1);
		if (nk_button_label(ctx, "New Game")) {
			setState(State::kNewGame);
		}
		if (nk_button_label(ctx, "Continue")) {
			//setState(State::kSaveLoad);
		}
		if (nk_button_label(ctx, "Settings")) {
			//setState(State::kSettings);
		}
		if (nk_button_label(ctx, "Quit")) {
			setState(State::kQuit);
		}
	}
	nk_end(ctx);

	nk_style_pop_style_item(ctx);
	nk_style_pop_color(ctx);
}
