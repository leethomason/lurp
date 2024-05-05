#include "xform.h"

void XFormer::setRenderSize(int rw, int rh)
{
	if (_renderSize.w == rw && _renderSize.h == rh) 
		return;
	_renderSize = Rect{0, 0, rw, rh};

	// Test for unity
	if (_virtualSize == _renderSize) {
		_scale = 1.0;
		_offset = Point{0, 0};
		return;
	}

	// Test for degenerate
	if (_renderSize.w == 0 || _renderSize.h == 0 || _virtualSize.w == 0 || _virtualSize.h == 0) {
		_scale = 1.0;
		_offset = Point{ 0, 0 };
		return;
	}

	// Test for integer scale.
	if (_renderSize.w > _virtualSize.w) {
		int scale = _renderSize.w / _virtualSize.w;
		if (_renderSize.w == _virtualSize.w * scale && _renderSize.h == _virtualSize.h * scale) {
			_scale = (float)scale;
			_offset = Point{0, 0};
			return;
		}
	}
	else {
		int scale = _virtualSize.w / _renderSize.w;
		if (_virtualSize.w == _renderSize.w * scale && _virtualSize.h == _renderSize.h * scale) {
			_scale = 1.0f / scale;
			_offset = Point{0, 0};
			return;
		}
	}

	float scaleOnWidth = (float)_renderSize.w / (float)_virtualSize.w;
	float scaleOnHeight = (float)_renderSize.h / (float)_virtualSize.h;

	if (scaleOnWidth < scaleOnHeight) {
		_scale = scaleOnWidth;
		_offset = Point{0, int((_renderSize.h - _virtualSize.h * _scale) / 2)};
	}
	else {
		_scale = scaleOnHeight;
		_offset = Point{int((_renderSize.w - _virtualSize.w * _scale) / 2), 0};
	}
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

Point XFormer::t(const Point& in) const
{
	Point p;
	p.x = _offset.x + s(in.x);
	p.y = _offset.y + s(in.y);

	// patch up for rounding errors
	if (in.x == 0) p.x = _offset.x;
	if (in.y == 0) p.y = _offset.y;
	if (in.x == _virtualSize.w) p.x = _offset.x + _renderSize.w;
	if (in.y == _virtualSize.h) p.y = _offset.y + _renderSize.h;

	return p;
}

SDL_Rect XFormer::t(const SDL_Rect& r) const
{
	if (r.x == 0 && r.y == 0 && r.w == _virtualSize.w && r.h == _virtualSize.h) {
		return sdlClipRect();
	}
	Point p = t(Point{r.x, r.y});
	SDL_Rect out = { p.x, p.y, s(r.w), s(r.h) };
	return out;
}
