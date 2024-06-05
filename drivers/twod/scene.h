#pragma once

#include "drawable.h"
#include "xform.h"


class Scene : public IDrawable
{
public:
	virtual ~Scene() = default;

	enum class State {
		kActive,
		kDone,
		kQuit,

		kNewGame,

	};
	void setState(State s) { _state = s; }
	State state() const { return _state; }

	virtual void mouseMotion(FontManager&, const Point& /*screen*/, const Point& /*virt*/) {}
	virtual void mouseButton(FontManager&, const Point& /*screen*/, const Point& /*virt*/, bool /*down*/) {}

private:
	State _state = State::kActive;
};

