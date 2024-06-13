#include "xform.h"
#include <fmt/format.h>

#define DEBUG_XFORM 0

void XFormer::setRenderSize(int rw, int rh)
{
	if (_renderSize.w == rw && _renderSize.h == rh) 
		return;

	_renderSize = lurp::Size{rw, rh};
#if DEBUG_XFORM
	fmt::print("setRenderSize {} x {}\n", rw, rh);
#endif

	// Test for unity
	if (_virtualSize == _renderSize) {
		_scale = 1.0;
		_offset = lurp::Point{0, 0};
#if DEBUG_XFORM
		fmt::print("unity\n");
#endif
		return;
	}

	// Test for degenerate
	if (_renderSize.w == 0 || _renderSize.h == 0 || _virtualSize.w == 0 || _virtualSize.h == 0) {
		_scale = 1.0;
		_offset = lurp::Point{ 0, 0 };
		return;
	}

	// Test for integer scale.
	if (_renderSize.w > _virtualSize.w) {
		int scale = _renderSize.w / _virtualSize.w;
		if (_renderSize.w == _virtualSize.w * scale && _renderSize.h == _virtualSize.h * scale) {
			_scale = (double)scale;
			_offset = lurp::Point{0, 0};
#if DEBUG_XFORM
			fmt::print("integer scale {}\n", scale);
			#endif
			return;
		}
	}
	else {
		int scale = _virtualSize.w / _renderSize.w;
		if (_virtualSize.w == _renderSize.w * scale && _virtualSize.h == _renderSize.h * scale) {
			_scale = 1.0f / scale;
			_offset = lurp::Point{0, 0};
#if DEBUG_XFORM
			fmt::print("integer scale {}\n", scale);
#endif
			return;
		}
	}

	double scaleOnWidth = (double)_renderSize.w / (double)_virtualSize.w;
	double scaleOnHeight = (double)_renderSize.h / (double)_virtualSize.h;

	if (scaleOnWidth < scaleOnHeight) {
		_scale = scaleOnWidth;
		_offset = lurp::Point{0, int((_renderSize.h - _virtualSize.h * _scale) / 2)};
	}
	else {
		_scale = scaleOnHeight;
		_offset = lurp::Point{int((_renderSize.w - _virtualSize.w * _scale) / 2), 0};
	}
#if DEBUG_XFORM
	fmt::print("_offset={},{} _scale={}\n", _offset.x, _offset.y, _scale);
#endif
}

SDL_Rect XFormer::sdlClipRect() const
{
	SDL_Rect r = { _offset.x, _offset.y, int(_virtualSize.w * _scale), int(_virtualSize.h * _scale) };
	// patch up for rounding errors
	if (_offset.x == 0)
		r.w = _renderSize.w;
	if (_offset.y == 0)
		r.h = _renderSize.h;
	return r;
}

lurp::Point XFormer::t(const lurp::Point& in) const
{
	lurp::Point p;
	p.x = _offset.x + s(in.x);
	p.y = _offset.y + s(in.y);
	return p;
}

lurp::Size XFormer::t(const lurp::Size& sz) const
{
	lurp::Size out;
	out.w = s(sz.w);
	out.h = s(sz.h);
	return out;
}

lurp::Rect XFormer::t(const lurp::Rect& r) const
{
	// Be careful on this one - it is used to tile rectangles.
	// Basically, you can't transform width. (Maybe in float?) But
	// integer transforms are right out. We need to base the width/height
	// on the *next* point, and compute back.

	lurp::Point p0 = t(lurp::Point{r.x, r.y});
	lurp::Point p1 = t(lurp::Point{r.x + r.w, r.y + r.h});

	lurp::Rect out;
	out.x = p0.x;
	out.y = p0.y;
	out.w = p1.x - p0.x;
	out.h = p1.y - p0.y;
	return out;
}

lurp::PointF XFormer::t(const lurp::PointF& p) const
{
	lurp::PointF out;
	out.x = float(_offset.x) + s(p.x);
	out.y = float(_offset.y) + s(p.y);
	return out;
}

lurp::RectF XFormer::t(const lurp::RectF& r) const
{
	// Unclear if the same algorithm is needed
	// for floating point, but err on the side of caution.
	lurp::PointF p0 = t(lurp::PointF{ r.x, r.y });
	lurp::PointF p1 = t(lurp::PointF{ r.x + r.w, r.y + r.h });

	lurp::RectF out;
	out.x = p0.x;
	out.y = p0.y;
	out.w = p1.x - p0.x;
	out.h = p1.y - p0.y;
	return out;
}

lurp::Point XFormer::screenToVirtual(const lurp::Point& p) const
{
	lurp::Point out;
	out.x = int((p.x - _offset.x) / _scale);
	out.y = int((p.y - _offset.y) / _scale);
	return out;
}
