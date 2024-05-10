#include "statemachine.h"
#include "titlescene.h"
#include "debug.h"
#include "mainscene.h"

#include "nuk.h"

#include <fmt/core.h>
#include <SDL2/SDL.h>

StateMachine::StateMachine(const GameConfig& gameConfig) : _gameConfig(gameConfig)
{
}

std::shared_ptr<Scene> StateMachine::tick(FrameData* frameData)
{
	bool newScene = false;
	if (!_currentScene) {
		newScene = true;
		_currentScene = std::make_shared<TitleScene>();
		_type = Type::kTitle;
	}
	else {
		if (_currentScene->state() == Scene::State::kDone) {
			switch (_type) {
			case Type::kNone: {
				FATAL_INTERNAL_ERROR();
				break;
			}
			case Type::kTitle: {
				_currentScene = std::make_shared<MainScene>();
				_type = Type::kMain;
				newScene = true;
				break;
			}
			default:
				FATAL_INTERNAL_ERROR();
				break;
			}
		}
	}
	if (newScene) {
		if (frameData) {
			frameData->sceneFrame = 0;
			frameData->sceneTime = 0;
		}
	}
	assert(_currentScene.get());
	return _currentScene;
}
