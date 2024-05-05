#include "test.h"
#include "test2d.h"

#include "xform.h"

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

void RunTests2D()
{
	TestXFormer();
}
