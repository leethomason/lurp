#include "test.h"
#include "test2d.h"

#include "xform.h"
#include "nuk.h"

#include <fmt/format.h>

static void TestXFormer()
{
	SDL_Rect sr = { 0, 0, 800, 600 };
	{
		XFormer xf(800, 600);
		TEST(xf.scale() == 1.0f);
		TEST(xf.offset() == Point());
		sr = xf.sdlClipRect();
		TEST(sr.x == 0);
		TEST(sr.y == 0);
		TEST(sr.w == 800);
		TEST(sr.h == 600);

		xf.setRenderSize(800, 600);
		TEST(xf.scale() == 1.0f);
		TEST(xf.offset() == Point(0, 0));

		xf.setRenderSize(400, 300);
		sr = xf.sdlClipRect();
		TEST(xf.scale() == 0.5f);
		TEST(xf.offset() == Point(0, 0));
		TEST(sr.x == 0);
		TEST(sr.y == 0);
		TEST(sr.w == 800 / 2);
		TEST(sr.h == 600 / 2);

		xf.setRenderSize(1600, 1200);
		sr = xf.sdlClipRect();
		TEST(xf.scale() == 2.0f);
		TEST(xf.offset() == Point(0, 0));
		TEST(sr.x == 0);
		TEST(sr.y == 0);
		TEST(sr.w == 800 * 2);
		TEST(sr.h == 600 * 2);
	}
	{
		XFormer xf(800, 600);		 // too wide:
		xf.setRenderSize(800, 400);	 // -> 533, 400 ouchie. 800 - 533 = 267 / 2 = 133
		TEST_FP(xf.scale(), 2.0f / 3.0f);
		TEST(xf.offset() == Point(133, 0));
		sr = xf.sdlClipRect();
		TEST(sr.x == 133);
		TEST(sr.y == 0);
		TEST(sr.w == 533);
		TEST(sr.h == 400);

		xf.setRenderSize(800, 800);	 // too tall & square. -> 800, 800, y = 100
		TEST_FP(xf.scale(), 1.0f);
		TEST(xf.offset() == Point(0, 100));
		sr = xf.sdlClipRect();
		TEST(sr.x == 0);
		TEST(sr.y == 100);
		TEST(sr.w == 800);
		TEST(sr.h == 600);

		xf.setRenderSize(400, 800); // too narrow -> 400, 300. y = 250
		TEST_FP(xf.scale(), 0.5f);
		TEST(xf.offset() == Point(0, 250));
		sr = xf.sdlClipRect();
		TEST(sr.x == 0);
		TEST(sr.y == 250);
		TEST(sr.w == 400);
		TEST(sr.h == 300);
	}
	{
		XFormer xf(600, 800);
		xf.setRenderSize(800, 400);	 // -> 300, 400 y = 250
		TEST_FP(xf.scale(), 0.50f);
		TEST(xf.offset() == Point(250, 0));
		sr = xf.sdlClipRect();
		TEST(sr.x == 250);
		TEST(sr.y == 0);
		TEST(sr.w == 300);
		TEST(sr.h == 400);

		xf.setRenderSize(800, 800);	 // -> 600, 800
		TEST_FP(xf.scale(), 1.0f);
		TEST(xf.offset() == Point(100, 0));
		sr = xf.sdlClipRect();
		TEST(sr.x == 100);
		TEST(sr.y == 0);
		TEST(sr.w == 600);
		TEST(sr.h == 800);

		xf.setRenderSize(400, 800);	 // -> 400, 533 y = 133
		TEST_FP(xf.scale(), 2.0f / 3.0f);
		TEST(xf.offset() == Point(0, 133));
		sr = xf.sdlClipRect();
		TEST(sr.x == 0);
		TEST(sr.y == 133);
		TEST(sr.w == 400);
		TEST(sr.h == 533);
	}
	{
		XFormer xf(1024, 768);
		xf.setRenderSize(1024*2, 768*2);
		TEST(xf.scale() == 2.0f);
		TEST(xf.offset() == Point(0, 0));

		xf.setRenderSize(1024/2, 768/2);
		TEST(xf.scale() == 0.5f);
		TEST(xf.offset() == Point(0, 0));
	}
}

static void XFormerAlignment()
{
	XFormer xf(800, 600);

	const int N = 2;
	Point sizes[N] = { { 2194, 1171 }, { 681, 429 } };

	for (int i = 0; i < N; i++) {
		xf.setRenderSize(sizes[i].x, sizes[i].y);

		for (int y = 0; y < 10; y++) {
			for (int x = 0; x < 10; x++) {
				SDL_Rect r00 = { x * 16, y * 16, 16, 16 };
				SDL_Rect r10 = { (x + 1) * 16, y * 16, 16, 16 };
				SDL_Rect r01 = { x * 16, (y + 1) * 16, 16, 16 };
				SDL_Rect r11 = { (x + 1) * 16, (y + 1) * 16, 16, 16 };

				SDL_Rect r00t = xf.t(r00);
				SDL_Rect r10t = xf.t(r10);
				SDL_Rect r01t = xf.t(r01);
				SDL_Rect r11t = xf.t(r11);

				TEST(r00t.x + r00t.w == r10t.x);
				TEST(r00t.y + r00t.h == r01t.y);
				TEST(r00t.x + r00t.w == r11t.x);
				TEST(r00t.y + r00t.h == r11t.y);
			}
		}
	}
}

void RunTests2D()
{
	TestXFormer();
	XFormerAlignment();
}

void AssetsTest::load()
{
	portrait11 = _textureManager.loadTexture("portrait11", "assets/portraitTest11.png");
	ps0 = _textureManager.loadTexture("ps-back", "assets/back.png");
	ps1 = _textureManager.loadTexture("ps-layer1", "assets/layer1_100.png");
	ps2 = _textureManager.loadTexture("ps-layer2", "assets/layer2_100.png");
	ps3 = _textureManager.loadTexture("ps-layer3", "assets/layer3_100.png");
	ps4 = _textureManager.loadTexture("ps-layer4", "assets/layer4_100.png");
	ps5 = _textureManager.loadTexture("ps-layer5", "assets/layer5_100.png");
	tree = _textureManager.loadTexture("tree", "assets/tree.png");

	// fixme: path not font name
	tf0 = _fontManager.createTextField("textField0", "roboto16", 300, 600);
	tf1 = _fontManager.createTextField("textField1", "roboto16", 300, 300, true, clearColor);
}

void AssetsTest::draw(const XFormer& xFormer, uint64_t frame)
{
	// Draw a texture. Confirm sRGB is working.
	if (portrait11->ready()) {
		SDL_Rect dest = xFormer.t(SDL_Rect{ 0, 0, portrait11->width(), portrait11->height() });
		SDL_RenderCopy(_renderer, portrait11->sdlTexture(), nullptr, &dest);
	}

	// Test against Ps
	if (Texture::ready({ ps0, ps1, ps2, ps3, ps4, ps5 })) {
		SDL_Rect dest = xFormer.t(SDL_Rect{ 300, 0, 256, 256 });
		//SDL_RenderBlend
		SDL_RenderCopy(_renderer, ps0->sdlTexture(), nullptr, &dest);
		SDL_RenderCopy(_renderer, ps1->sdlTexture(), nullptr, &dest);
		SDL_SetTextureAlphaMod(ps2->sdlTexture(), 128);
		SDL_RenderCopy(_renderer, ps2->sdlTexture(), nullptr, &dest);
		SDL_RenderCopy(_renderer, ps3->sdlTexture(), nullptr, &dest);
		SDL_SetTextureAlphaMod(ps4->sdlTexture(), 128);
		SDL_RenderCopy(_renderer, ps4->sdlTexture(), nullptr, &dest);
		SDL_SetTextureAlphaMod(ps5->sdlTexture(), 128);
		SDL_RenderCopy(_renderer, ps5->sdlTexture(), nullptr, &dest);
	}
	if (tree->ready()) {
		SDL_Rect dest = xFormer.t(SDL_Rect{ 400, 300, 400, 400 });
		SDL_RenderCopy(_renderer, tree->sdlTexture(), nullptr, &dest);

		for (int i = 0; i < 3; i++) {
			SDL_Rect r = xFormer.t(SDL_Rect{ i * 100, 400, 50 + 50 * i, 50 + 50 * i });
			SDL_RenderCopy(_renderer, tree->sdlTexture(), nullptr, &r);
		}
	}
	{
		std::string text = fmt::format("Hello, world! This is some text that will need to be wrapped to fit in the box. frame/60={}", frame / 60);
		tf0->Render(text, 400, 300, SDL_Color{ 255, 255, 255, 255 });

		std::string text2 = "This is some fancy pants hq text.";
		tf1->Render(text2, 20, 550, SDL_Color{ 255, 255, 255, 255 });
	}
}

void AssetsTest::drawGUI(nk_context* nukCtx, float fs, const XFormer& xFormer, uint64_t /*frame*/)
{
	RectF guiRect = xFormer.t(RectF{ 560, 20, 230, 250 });
	const float height = fs * 1.8f;

	if (nk_begin(nukCtx, "Demo", nk_rect(guiRect.x, guiRect.y, guiRect.w, guiRect.h),
		NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		enum { EASY, HARD };
		static int op = EASY;
		static int property = 20;
		nk_colorf bg{ 0, 0, 0, 0 };

		nk_layout_row_static(nukCtx, height, 80, 1);
		if (nk_button_label(nukCtx, "button"))
			fprintf(stdout, "button pressed\n");
		nk_layout_row_dynamic(nukCtx, height, 2);
		if (nk_option_label(nukCtx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(nukCtx, "hard", op == HARD)) op = HARD;
		nk_layout_row_dynamic(nukCtx, height, 1);
		// fixme: doesn't slide with the correct scale
		nk_property_int(nukCtx, "Compression:", 0, &property, 100, 10, 1.0f);
	}
	nk_end(nukCtx);
}
