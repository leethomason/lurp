#include "statemachine.h"
#include "titlescene.h"
#include "debug.h"
#include "mainscene.h"
#include "gamescene.h"
#include "test2d.h"

#include "nuk.h"

#include <fmt/core.h>
#include <SDL2/SDL.h>

StateMachine::StateMachine(const GameConfig& gameConfig) : _gameConfig(gameConfig)
{
}

void StateMachine::doTest()
{
	_type = Type::kTest;
	_currentScene = nullptr;
}

std::shared_ptr<Scene> StateMachine::tick(FrameData* frameData)
{
	bool newScene = false;
	if (!_currentScene) {
		newScene = true;
		if (_type == Type::kTest) {
			_currentScene = std::make_shared<AssetsTest>();
		}
		else {
			_currentScene = std::make_shared<TitleScene>();
			_type = Type::kTitle;
		}
		if (frameData) {
			frameData->sceneFrame = 0;
			frameData->sceneTime = 0;
		}
		return _currentScene;
	}

	Scene::State state = _currentScene->state();
	if (state == Scene::State::kActive) {
		// Scene is processing, do nothing
		return _currentScene;
	}

	if(_type == Type::kTitle) {
		if (state == Scene::State::kDone) {
			_currentScene = std::make_shared<MainScene>();
			_type = Type::kMain;
			newScene = true;
		}
		else {
			FATAL_INTERNAL_ERROR();
		}
	}
	else if (_type == Type::kMain) {
		if (state == Scene::State::kQuit) {
			_done = true;
		}
		else if (state == Scene::State::kNewGame) {
			_currentScene = std::make_shared<GameScene>();
			_type = Type::kGame;
			newScene = true;
		}
		// FIXME: add other states
		//else {
		//	FATAL_INTERNAL_ERROR();
		//}
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
