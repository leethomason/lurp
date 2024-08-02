#pragma once

#include "config2d.h"
#include "scene.h"

class StateMachine
{
public:
	StateMachine();

	std::shared_ptr<Scene> tick(FrameData* frameData);

	bool done() const { return _done; }

	enum class Type {
		kNone,
		kTitle,
		kMain,
		kGame,
		kTest
	};
	void setStart(Type t);

private:
	std::shared_ptr<Scene> makeNewScene(Type t);

	bool _done = false;
	std::shared_ptr<Scene> _currentScene;
	Type _type = Type::kNone;
};

