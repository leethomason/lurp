#pragma once

#include "geom.h"
#include <SDL.h>

inline SDL_Rect toSDL(const lurp::Rect& r) { return SDL_Rect{ r.x, r.y, r.w, r.h }; };
inline SDL_Color toSDL(const lurp::Color& c) { return SDL_Color{ c.r, c.g, c.b, c.a }; };

class XFormer
{
public:
	// 16:9
	// sd    : 960 x 540
	// 1080p : 1920 x 1080
	// 4k    : 3840 x 2160

	XFormer(int virtualW, int virtualH) {
		_virtualSize = lurp::Size{virtualW, virtualH };
		setUnity(virtualW, virtualH);
	}
	~XFormer() {}

	void setUnity(int x, int y) {
		setRenderSize(x, y);
	}
	void setRenderSize(int w, int h);

	SDL_Rect sdlClipRect() const;

	double scale() const { return _scale; }
	lurp::Point offset() const { return _offset; }

	lurp::Size renderSize() const { return _renderSize; }
	lurp::Size virtualSize() const { return _virtualSize; }

	int s(int x) const { return (int)(x * _scale); }
	float s(float x) const { return float(x * _scale); }
	double s(double x) const { return x * _scale; }

	lurp::Size t(const lurp::Size& s) const;

	lurp::Point t(const lurp::Point& p) const;
	lurp::Point t(int x, int y) const { return t(lurp::Point(x, y)); }
	lurp::Rect t(const lurp::Rect& r) const;
	SDL_Rect t(const SDL_Rect& r) const {
		lurp::Rect out = t(lurp::Rect{ r.x, r.y, r.w, r.h });
		return SDL_Rect{ out.x, out.y, out.w, out.h };
	}
	lurp::PointF t(const lurp::PointF& p) const;
	lurp::RectF t(const lurp::RectF& r) const;

	lurp::Point screenToVirtual(const lurp::Point& p) const;

private:
	lurp::Size _renderSize;
	lurp::Size _virtualSize;
	double _scale = 1.0;
	lurp::Point _offset;
};
