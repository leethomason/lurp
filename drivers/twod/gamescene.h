#pragma once

#include "scene.h"
#include "texture.h"
#include "text.h"
#include "config2d.h"

#include "scriptbridge.h"
#include "scriptasset.h"

#include <vector>
#include <string>
#include <memory>

namespace lurp {
	class ZoneDriver;
	class Inventory;
}

class GameScene : public Scene
{
public:
	GameScene() = default;
	virtual ~GameScene();

	virtual void load(Drawing&, const FrameData& f) override;
	virtual void draw(Drawing&, const FrameData& f, const XFormer& xformer) override;
	virtual void layoutGUI(nk_context* nukCtx, float fontSize, const XFormer& xformer) override;

	virtual void reload(const GameConfig2D& config) override;
	virtual void mouseMotion(FontManager&, const lurp::Point& /*screen*/, const lurp::Point& /*virt*/) override;
	virtual void mouseButton(FontManager&, const lurp::Point& /*screen*/, const lurp::Point& /*virt*/, bool /*down*/) override;

private:
	using TextureRef = std::pair<int, std::shared_ptr<Texture>>;

	// mutable version of GameRegion
	struct Region {
		std::string name;
		lurp::Rect position;
		std::vector<TextureRef> images;
	};

	std::vector<Region> _regions;

	// Text region (main i/o)
	static constexpr int kMaxOptions = 16;
	std::shared_ptr<TextBox> _mainText;
	VBox _mainOptions;

	struct Option {
		enum class Type { kDir, kContainer, kChoice, kInteraction };
		Type type = Type::kDir;
		int index = 0;
		std::string entityID;
	};
	std::vector<Option> _options;

	// Auxillary info region
	std::shared_ptr<TextBox> _infoText;

	const GameRegion* getRegion(const std::string& name, const std::vector<GameRegion>& regions);

	// Game -------------------
	lurp::ScriptBridge _bridge;
	lurp::ConstScriptAssets _csassets;
	lurp::ScriptAssets* _assets = nullptr;
	lurp::ZoneDriver* _zoneDriver = nullptr;

	bool _needProcess = false;
	void process(Drawing& d);

	void addNavigation(Drawing& d);
	void addNews(Drawing& d);
	void addContainers(Drawing& d);
	void addChoices(Drawing& d);
	void addInteractions(Drawing& d);
	void addText(Drawing& d);

	void addRoom(Drawing& d);
	void addInventory(Drawing& d);

	std::string itemString(const lurp::ItemRef& itemRef);
	std::string inventoryString(const lurp::Inventory& inv);
};
