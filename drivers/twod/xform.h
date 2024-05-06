#pragma once

#include <SDL.h>

struct Point {
	Point() {}
	Point(int x, int y) : x(x), y(y) {}

	int x = 0;
	int y = 0;

	bool operator==(const Point& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
	bool operator!=(const Point& rhs) const {
		return !(*this == rhs);
	}
};

struct PointF {
	PointF() {}
	PointF(float x, float y) : x(x), y(y) {}

	float x = 0;
	float y = 0;

	bool operator==(const PointF& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
	bool operator!=(const PointF& rhs) const {
		return !(*this == rhs);
	}
};

struct Rect {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;

	bool operator==(const Rect& rhs) const {
		return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
	}
	bool operator!=(const Rect& rhs) const {
		return !(*this == rhs);
	}
};

struct RectF {
	float x = 0;
	float y = 0;
	float w = 0;
	float h = 0;

	bool operator==(const RectF& rhs) const {
		return x == rhs.x && y == rhs.y && w == rhs.w && h == rhs.h;
	}
	bool operator!=(const RectF& rhs) const {
		return !(*this == rhs);
	}
};

class XFormer
{
public:
	// 16:9
	// sd    : 960 x 540
	// 1080p : 1920 x 1080
	// 4k    : 3840 x 2160

	XFormer(int virtualW, int virtualH) {
		_virtualSize = Rect{0, 0, virtualW, virtualH };
		setUnity(virtualW, virtualH);
	}
	~XFormer() {}

	void setUnity(int x, int y) {
		setRenderSize(x, y);
	}
	void setRenderSize(int w, int h);

	SDL_Rect sdlClipRect() const;

	float scale() const { return _scale; }
	Point offset() const { return _offset; }

	int renderW() const { return _renderSize.w; }
	int renderH() const { return _renderSize.h; }

	int s(int x) const { return (int)roundf(x * _scale); }
	float s(float x) const { return x * _scale; }
	Point s(const Point& p) const { return Point(s(p.x), s(p.y)); }	

	Point t(const Point& p) const;
	Point t(int x, int y) const { return t(Point(x, y)); }
	SDL_Rect t(const SDL_Rect& r) const;
	PointF t(const PointF& p) const;
	RectF t(const RectF& r) const;

private:
	Rect _renderSize;
	Rect _virtualSize;
	float _scale = 1.0;
	Point _offset;
};
