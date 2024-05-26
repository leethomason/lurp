#include "gamescene.h"
#include "config.h"
#include "zonedriver.h"
#include "text.h"

void GameScene::load(Drawing& d, const FrameData& f)
{
	if (f.sceneFrame != 0) return;

	if (const GameRegion* r = getRegion(GameRegion::Type::kImage, d.config.regions)) {
		if (!r->imagePath.empty()) {
			_imageTexture = d.textureManager.loadTexture(d.config.assetsDir / r->imagePath);
		}
	}

	if (const GameRegion* r = getRegion(GameRegion::Type::kText, d.config.regions)) {
		bool opaque = r->bgColor.a > 0;
		
		_mainText = d.fontManager.createTextBox(d.config.font, r->position.w, r->position.h, opaque);
		_mainText->setColor(d.config.textColor);
		_mainText->setBgColor(r->bgColor);

		_mainOptions.boxes.resize(kMaxOptions);
		const int height = 2 * r->position.h / kMaxOptions;

		for (int i = 0; i < kMaxOptions; ++i) {
			_mainOptions.boxes[i] = d.fontManager.createTextBox(d.config.font, r->position.w, height, opaque);
			_mainOptions.boxes[i]->setColor(d.config.optionColor);
			_mainOptions.boxes[i]->setBgColor(r->bgColor);
			_mainOptions.boxes[i]->enableInteraction(true);
		}
	}

	if (const GameRegion* r = getRegion(GameRegion::Type::kInfo, d.config.regions)) {
		bool opaque = r->bgColor.a > 0;

		_infoText = d.fontManager.createTextBox(d.config.font, r->position.w, r->position.h, opaque);
		_infoText->setColor(d.config.textColor);
		_infoText->setBgColor(r->bgColor);
	}

	_csassets = _bridge.readCSA(d.config.scriptFile);
	_assets = new lurp::ScriptAssets(_csassets);
	_zoneDriver = new lurp::ZoneDriver(*_assets, _bridge, d.config.startingZone);
	_needProcess = true;
}

GameScene::~GameScene()
{
	delete _zoneDriver;
	delete _assets;
}

void GameScene::draw(Drawing& d, const FrameData&, const XFormer& x)
{
	if (_needProcess) {
		process(d);
		_needProcess = false;
	}

	const GameRegion* imageRegion = getRegion(GameRegion::Type::kImage, d.config.regions);
	if (imageRegion) {
		Rect rDst = x.t(imageRegion->position);
		SDL_Rect dst{ rDst.x, rDst.y, rDst.w, rDst.h };
		Draw(d.renderer, _imageTexture, nullptr, &dst, RenderQuality::kBlit);
	}

	const GameRegion* textRegion = getRegion(GameRegion::Type::kText, d.config.regions);
	if (textRegion) {
		_mainText->pos = x.t(Point{ textRegion->position.x, textRegion->position.y });
		d.fontManager.Draw(_mainText);
		
		int y = _mainText->pos.y + _mainText->surfaceSize().h;
		Point p = { _mainText->pos.x, y };
		d.fontManager.Draw(_mainOptions, p);
	}

	const GameRegion* infoRegion = getRegion(GameRegion::Type::kInfo, d.config.regions);
	if (infoRegion) {
		_infoText->pos = x.t(Point{ infoRegion->position.x, infoRegion->position.y });
		d.fontManager.Draw(_infoText);
	}

}

void GameScene::layoutGUI(nk_context*, float, const XFormer&)
{

}

const GameRegion* GameScene::getRegion(GameRegion::Type type, const std::vector<GameRegion>& regions)
{
	for (size_t i = 0; i < regions.size(); ++i) {
		const GameRegion& r = regions[i];
		if (r.type == type) {
			return &r;
		}
	}
	return nullptr;
}

void GameScene::processNavigation(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (region) {
		for (auto& tb : _mainOptions.boxes)
			tb->setText("");
		_options.assign(_mainOptions.boxes.size(), Option{});

		std::vector<lurp::DirEdge> dirEdges = _zoneDriver->dirEdges();

		std::string text;
		int optIdx = 0;
		for (size_t i = 0; i < dirEdges.size() && optIdx < _mainOptions.boxes.size(); ++i) {
			const lurp::DirEdge& de = dirEdges[i];
			if (de.dir == lurp::Edge::Dir::kUnknown)
				text = fmt::format("{}", de.name);
			else
				text = fmt::format("{}:\t{}", de.dirLong, de.name);

			Option opt;
			opt.type = Option::Type::kDir;
			opt.index = static_cast<int>(i);
			opt.entityID = de.dstRoom;

			_options[optIdx] = opt;
			auto& tb = _mainOptions.boxes[optIdx];
			tb->setText(i, text);
			tb->setColor(i, { 0, 255, 255, 255 });
			tb->setFont(i, d.config.font);

			optIdx++;
		}
	}
}

void GameScene::process(Drawing& d)
{
	lurp::ZoneDriver::Mode mode = _zoneDriver->mode();

	if (mode == lurp::ZoneDriver::Mode::kNavigation) {
		addNews(d);
		processNavigation(d);
	}
	else if (mode == lurp::ZoneDriver::Mode::kText) {
		const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
		if (region) {
			std::shared_ptr<TextBox>& tb = _mainText;
			tb->resize(1);
			tb->setText(_zoneDriver->text().text);
		}
	}

	// Side panel
	{
		const GameRegion* region = getRegion(GameRegion::Type::kInfo, d.config.regions);
		if (region) {
			const lurp::Zone& zone = _zoneDriver->currentZone();
			const lurp::Room& room = _zoneDriver->currentRoom();

			std::shared_ptr<TextBox>& tb = _infoText;
			tb->resize(3);

			tb->setText(0, zone.name);
			tb->setColor(0, {192, 192, 192, 255});

			tb->setText(1, room.name);
			tb->setColor(1, { 255, 255, 255, 255 });

			tb->setText(2, room.desc);
			tb->setColor(2, { 192, 192, 192, 255 });
		}
	}
}

void GameScene::addNews(Drawing& d)
{
	//const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	//if (region) {
	//	while(_zoneDriver->qu)
	//	
	//}
}

void GameScene::mouseMotion(FontManager& fm, const Point& screen, const Point& virt)
{
	fm.doMove(screen, virt);
}

void GameScene::mouseButton(FontManager& fm, const Point& screen, const Point& virt, bool down)
{
	std::shared_ptr<TextBox> clicked = fm.doButton(screen, virt, down);
	if (clicked) {
		for (size_t i = 0; i < _mainOptions.boxes.size(); ++i) {
			if (clicked == _mainOptions.boxes[i]) {
				if (_options[i].type == Option::Type::kDir) {
					_zoneDriver->move(_options[i].entityID);
					_needProcess = true;
				}
				break;
			}
		}
	}
}
