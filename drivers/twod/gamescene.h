#pragma once

#include "scene.h"
#include "texture.h"

#include "scriptbridge.h"
#include "scriptasset.h"

#include <vector>
#include <string>
#include <memory>

class GameScene : public Scene
{
public:
	GameScene() = default;
	virtual ~GameScene();

	virtual void load(Drawing&, const FrameData& f) override;
	virtual void draw(Drawing&, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

private:
	// Grapics ----------------
	struct GSData {
		std::string text;
		std::shared_ptr<Texture> texture;
	};
	std::vector<GSData> _data;

	// Game -------------------
	lurp::ScriptBridge _bridge;
	lurp::ConstScriptAssets _csassets;
	lurp::ScriptAssets* _assets = nullptr;
	lurp::ZoneDriver* _zoneDriver = nullptr;
};
