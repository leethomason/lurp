#pragma once

#include "config.h"
#include "scene.h"

class StateMachine
{
public:
	StateMachine(const lurp::GameConfig& gameConfig);

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
	lurp::GameConfig _gameConfig;
	std::shared_ptr<Scene> _currentScene;
	Type _type = Type::kNone;
};

