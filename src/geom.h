#pragma once

#include <stdint.h>

namespace lurp {

struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;
};

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

struct Size {
	Size() {}
	Size(int w, int h) : w(w), h(h) {}

	int w = 0;
	int h = 0;

	bool operator==(const Size& rhs) const {
		return w == rhs.w && h == rhs.h;
	}
	bool operator!=(const Size& rhs) const {
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
	bool contains(const Point& p) const {
		return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
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

} // namespace lurp