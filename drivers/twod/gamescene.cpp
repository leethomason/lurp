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

void GameScene::addNavigation(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (region) {
		std::vector<lurp::DirEdge> dirEdges = _zoneDriver->dirEdges();

		std::string text;
		for (size_t i = 0; i < dirEdges.size() && _options.size() < _mainOptions.boxes.size(); ++i) {
			const lurp::DirEdge& de = dirEdges[i];
			if (de.dir == lurp::Edge::Dir::kUnknown)
				text = fmt::format("{}", de.name);
			else
				text = fmt::format("{}:\t{}", de.dirLong, de.name);

			Option opt;
			opt.type = Option::Type::kDir;
			opt.index = static_cast<int>(i);
			opt.entityID = de.dstRoom;
			
			auto& tb = _mainOptions.boxes[_options.size()];
			tb->setText(text);
			tb->setColor({ 0, 255, 255, 255 });
			tb->setFont(d.config.font);

			_options.push_back(opt);
		}
	}
}

void GameScene::addContainers(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (!region) return;

	lurp::ContainerVec vec = _zoneDriver->getContainers();
	int i = 0;
	for (const lurp::Container* c : vec) {
		if (_options.size() >= _mainOptions.boxes.size()) break;

		bool locked = _zoneDriver->isLocked(c->entityID);
		std::string text;
		if (locked) {
			text = fmt::format("{} (locked)", c->name);
		}
		else {
			const lurp::Inventory& inv = _zoneDriver->getInventory(*c);
			std::string istr = inventoryString(inv);
			text = fmt::format("{} ({})", c->name, istr);
		}

		Option opt;
		opt.type = Option::Type::kContainer;
		opt.index = i;
		opt.entityID = c->entityID;

		auto& tb = _mainOptions.boxes[_options.size()];
		tb->setText(text);
		tb->setColor({ 0, 255, 255, 255 });
		tb->setFont(d.config.font);

		_options.push_back(opt);
		i++;
	}
}

void GameScene::addChoices(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (!region) return;

	const lurp::Choices& choices = _zoneDriver->choices();
	for (size_t i = 0; i < choices.choices.size() && _options.size() < _mainOptions.boxes.size(); ++i) {
		const lurp::Choices::Choice& c = choices.choices[i];
		Option opt;
		opt.type = Option::Type::kChoice;
		opt.index = static_cast<int>(i);

		auto& tb = _mainOptions.boxes[_options.size()];
		tb->setText(c.text);
		tb->setColor({ 0, 255, 255, 255 });
		tb->setFont(d.config.font);

		_options.push_back(opt);
	}
}

void GameScene::addInteractions(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (!region) return;

	lurp::InteractionVec vec = _zoneDriver->getInteractions();
	for (size_t i = 0; i < vec.size(); i++) {
		const lurp::Interaction* iact = vec[i];

		Option opt;
		opt.type = Option::Type::kInteraction;
		opt.index = static_cast<int>(i);

		auto& tb = _mainOptions.boxes[_options.size()];
		tb->setText(iact->name);
		tb->setColor({ 0, 255, 255, 255 });
		tb->setFont(d.config.font);

		_options.push_back(opt);
	}
}

void GameScene::addNews(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (!region) return;

	while (!_zoneDriver->news().empty()) {
		lurp::NewsItem news = _zoneDriver->news().pop();
		size_t idx = _mainText->size();
		_mainText->resize(idx + 1);
		_mainText->setText(idx, news.text);
	}
}

void GameScene::addText(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kText, d.config.regions);
	if (!region) return;

	while (_zoneDriver->mode() == lurp::ZoneDriver::Mode::kText) {
		lurp::TextLine text = _zoneDriver->text();
		size_t idx = _mainText->size();
		_mainText->resize(idx + 1);
		// FIXME speaker variant
		_mainText->setText(idx, text.text);
		_mainText->setSpace(idx, d.config.font->pointSize);
		_zoneDriver->advance();
	}
}

void GameScene::addRoom(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kInfo, d.config.regions);
	if (!region) return;

	const lurp::Zone& zone = _zoneDriver->currentZone();
	const lurp::Room& room = _zoneDriver->currentRoom();

	std::shared_ptr<TextBox>& tb = _infoText;
	
	size_t start = tb->size();
	if (!zone.name.empty()) {
		tb->resize(start + 1);
		tb->setText(start, zone.name);
		tb->setColor(start, { 192, 192, 192, 255 });
	}

	start = tb->size();
	if (!room.name.empty()) {
		tb->resize(start + 1);
		tb->setText(start, room.name);
		tb->setColor(start, { 255, 255, 255, 255 });
		tb->setSpace(start, d.config.font->pointSize);
	}

	start = tb->size();
	if (!room.desc.empty()) {
		tb->resize(start + 1);
		tb->setText(start, room.desc);
		tb->setColor(start, { 192, 192, 192, 255 });
	}
	tb->setSpace(start, d.config.font->pointSize);
}

void GameScene::addInventory(Drawing& d)
{
	const GameRegion* region = getRegion(GameRegion::Type::kInfo, d.config.regions);
	if (!region) return;

	const lurp::Actor& player = _zoneDriver->getPlayer();
	const lurp::Inventory& inv = _zoneDriver->getInventory(player);

	for (const auto& item : inv.items()) {
		size_t idx = _infoText->size();
		_infoText->resize(idx + 1);
		std::string t = itemString(item);
		_infoText->setText(idx, t);
	}
}

void GameScene::process(Drawing& d)
{
	_mainText->resize(0);
	for (auto& tb : _mainOptions.boxes)
		tb->setText("");
	_options.clear();
	_infoText->resize(0);

	addNews(d);
	addText(d);

	lurp::ZoneDriver::Mode mode = _zoneDriver->mode();
	if (mode == lurp::ZoneDriver::Mode::kNavigation) {
		addInteractions(d);
		addContainers(d);
		addNavigation(d);
	}
	else if (mode == lurp::ZoneDriver::Mode::kChoices) {
		addChoices(d);
	}
	else {
		assert(false);
	}

	// Side panel
	addRoom(d);
	addInventory(d);
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
				else if (_options[i].type == Option::Type::kContainer) {
					_zoneDriver->transferAll(_options[i].entityID, "player");
					_needProcess = true;
				}
				else if (_options[i].type == Option::Type::kChoice) {
					_zoneDriver->choose(_options[i].index);
					_needProcess = true;
				}
				else if (_options[i].type == Option::Type::kInteraction) {
					_zoneDriver->startInteraction(_zoneDriver->getInteractions()[_options[i].index]);
					_needProcess = true;
				}
				break;
			}
		}
	}
}

std::string GameScene::itemString(const lurp::ItemRef& ref)
{
	std::string text;
	if (ref.count == 1)
		text = ref.pItem->name;
	else
		text = fmt::format("{} x{}", ref.pItem->name, ref.count);
	return text;
}

std::string GameScene::inventoryString(const lurp::Inventory& inv)
{
	std::string text;
	for (const auto& item : inv.items()) {
		if (!text.empty()) text += " | ";
		text += itemString(item);
	}
	return text;
}
