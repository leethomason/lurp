#include "statemachine.h"
#include "titlescene.h"
#include "debug.h"
#include "mainscene.h"
#include "gamescene.h"
#include "test2d.h"

#include "nuk.h"

#include <fmt/core.h>
#include <SDL2/SDL.h>

StateMachine::StateMachine(const lurp::GameConfig& gameConfig) : _gameConfig(gameConfig)
{
}

void StateMachine::setStart(StateMachine::Type t)
{
	_type = t;
	_currentScene = nullptr;
}

std::shared_ptr<Scene> StateMachine::makeNewScene(Type t)
{
	switch (t) {
	case Type::kNone: FATAL_INTERNAL_ERROR(); break;
	case Type::kTitle: _type = Type::kTitle;  return std::make_shared<TitleScene>();
	case Type::kMain: _type = Type::kMain;  return std::make_shared<MainScene>();
	case Type::kGame: _type = Type::kGame;  return std::make_shared<GameScene>();
	case Type::kTest: _type = Type::kTest;  return std::make_shared<AssetsTest>();
	}
	FATAL_INTERNAL_ERROR();
	return nullptr;
}

std::shared_ptr<Scene> StateMachine::tick(FrameData* frameData)
{
	bool newScene = false;
	if (!_currentScene) {
		if (_type == Type::kNone)
			_type = Type::kTitle;
		newScene = true;
		_currentScene = makeNewScene(_type);
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
			_currentScene = makeNewScene(Type::kMain);
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
			_currentScene = makeNewScene(Type::kGame);
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
