#pragma once

#include "config.h"
#include "scene.h"

class StateMachine
{
public:
	StateMachine(const GameConfig& gameConfig);

	std::shared_ptr<Scene> tick(FrameData* frameData);

	bool done() const { return _done; }

private:
	enum class Type {
		kNone,
		kTitle,
		kMain,
		kGame
	};
	bool _done = false;
	GameConfig _gameConfig;
	std::shared_ptr<Scene> _currentScene;
	Type _type = Type::kNone;
};

