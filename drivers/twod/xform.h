#pragma once

#include "geom.h"
#include <SDL.h>

inline SDL_Rect toSDL(const Rect& r) { return SDL_Rect{ r.x, r.y, r.w, r.h }; };
inline SDL_Color toSDL(const Color& c) { return SDL_Color{ c.r, c.g, c.b, c.a }; };

class XFormer
{
public:
	// 16:9
	// sd    : 960 x 540
	// 1080p : 1920 x 1080
	// 4k    : 3840 x 2160

	XFormer(int virtualW, int virtualH) {
		_virtualSize = Size{virtualW, virtualH };
		setUnity(virtualW, virtualH);
	}
	~XFormer() {}

	void setUnity(int x, int y) {
		setRenderSize(x, y);
	}
	void setRenderSize(int w, int h);

	SDL_Rect sdlClipRect() const;

	double scale() const { return _scale; }
	Point offset() const { return _offset; }

	Size renderSize() const { return _renderSize; }
	Size virtualSize() const { return _virtualSize; }

	int s(int x) const { return (int)(x * _scale); }
	float s(float x) const { return float(x * _scale); }
	double s(double x) const { return x * _scale; }

	Size t(const Size& s) const;

	Point t(const Point& p) const;
	Point t(int x, int y) const { return t(Point(x, y)); }
	Rect t(const Rect& r) const;
	SDL_Rect t(const SDL_Rect& r) const {
		Rect out = t(Rect{ r.x, r.y, r.w, r.h });
		return SDL_Rect{ out.x, out.y, out.w, out.h };
	}
	PointF t(const PointF& p) const;
	RectF t(const RectF& r) const;

	Point screenToVirtual(const Point& p) const;

private:
	Size _renderSize;
	Size _virtualSize;
	double _scale = 1.0;
	Point _offset;
};
