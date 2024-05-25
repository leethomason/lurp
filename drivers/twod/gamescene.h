#pragma once

#include "scene.h"
#include "texture.h"
#include "text.h"
#include "config.h"

#include "scriptbridge.h"
#include "scriptasset.h"

#include <vector>
#include <string>
#include <memory>

namespace lurp {
	class ZoneDriver;
}

class GameScene : public Scene
{
public:
	GameScene() = default;
	virtual ~GameScene();

	virtual void load(Drawing&, const FrameData& f) override;
	virtual void draw(Drawing&, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

private:
	// Image region
	std::shared_ptr<Texture> _imageTexture;

	// Text region (main i/o)
	std::shared_ptr<TextBox> _mainText;

	// Auxillary info region
	std::shared_ptr<TextBox> _infoText;

	const GameRegion* getRegion(GameRegion::Type type, const std::vector<GameRegion>& regions);

	// Game -------------------
	lurp::ScriptBridge _bridge;
	lurp::ConstScriptAssets _csassets;
	lurp::ScriptAssets* _assets = nullptr;
	lurp::ZoneDriver* _zoneDriver = nullptr;

	bool _needProcess = false;
	void process(Drawing& d);
};
