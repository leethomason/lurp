#pragma once

#include "drawable.h"

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

private:
	State _state = State::kActive;
};

