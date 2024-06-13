#pragma once

#include "texture.h"
#include "xform.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;
class Texture;
class XFormer;
class FontManager;

struct Font {
	TTF_Font* font = nullptr;	// this is kept current on the main thread and is always valid (the pool will be flushed before this changes)
	int pointSize = 0;
	int realSize = 0;
	std::string path;			// for reload, which has to be done on resize (keeping fonts 1:1)
};

enum class MouseState {
	none,
	over,
	down,
};

/*
* A TextBox is one texture, to which there can be any number of strings, fonts, and colors.
* All the text must by HQ or blended. It uses memory as efficiently as possible.
*/
class TextBox {
	friend class FontManager;
	friend class RenderFontTask;

public:
	TextBox();
	~TextBox();

	lurp::Point pos;	// in screen!

	size_t size() const { return _row.size(); }
	void resize(size_t s);

	void setFont(const Font* font) { setFont(0, font);  }
	void setText(const std::string& text) { setText(0, text); }
	void setColor(SDL_Color color) { setColor(0, color); }
	void setBgColor(SDL_Color color);

	void setFont(size_t i, const Font* font);
	void setText(size_t i, const std::string& text);
	void setColor(size_t i, SDL_Color color);
	void setBgColor(size_t i, SDL_Color color);
	void setSpace(size_t i, int virtualSpaceAfter);

	const Font* font(size_t i = 0) const { return _row[i].font; }
	const std::string& text(size_t i = 0) const { return _row[i].text; }
	SDL_Color color(size_t i = 0) const { return _row[i].color; }
	int spacing(size_t i = 0) const { return _row[i].virtualSpace; }
	SDL_Color bgColor() const { return _bg; }

	lurp::Size virtualSize() const { return _virtualSize;	}
	// Note that surface is in screen, not virtual.
	lurp::Size surfaceSize() const { return _texture->surfaceSize(); }

	void enableInteraction(bool enable) { _interactive = enable; }
	bool hitTest(const lurp::Point& screen) const;
	MouseState mouseState() const { return _mouseState; }

private:
	bool _needUpdate = false;
	bool _interactive = false;

	std::shared_ptr<Texture> _texture;
	lurp::Size _virtualSize;

	bool _hqOpaque = false;
	SDL_Color _bg = SDL_Color{ 0, 0, 0, 255 };
	MouseState _mouseState = MouseState::none;

	const Font* _font0 = nullptr;
	struct Row {
		const Font* font = nullptr;
		std::string text;
		SDL_Color color = { 255, 255, 255, 255 };
		int virtualSpace = 0;
	};
	std::vector<Row> _row;
};

/*
* A VBox is a collection of TextBoxes, and does the same layout.
* The advantage is that a VBox can be hit tested for individual TextBoxes.
* The disadvantage is that it uses more video memory.
*/
class VBox {
	friend class FontManager;

public:
	VBox() = default;
	~VBox() = default;

	std::vector<std::shared_ptr<TextBox>> boxes;

	void setState(SDL_Color disabled, SDL_Color up) {
		_disabledColor = disabled;
		_upColor = up;
	}

private:
	SDL_Color _disabledColor{ 255, 255, 255, 255 }, _upColor{ 255, 255, 255, 255 };
};

class FontManager {
public:
	FontManager(SDL_Renderer* renderer, enki::TaskScheduler& pool, TextureManager& textureManager, int virtualW, int virtualH) : _renderer(renderer), _pool(pool), _textureManager(textureManager), _xf(virtualW, virtualH) {}
	~FontManager();

	// Fast to get an existing font.
	// New fonts or resizing is expensive - keep the font count low.
	// Note that a 'const Font*' is returned. The font is never destroyed, and owned by the manager.
	const Font* loadFont(const std::string& path, int virtualSize);

	void update(const XFormer& xf);

	std::shared_ptr<TextBox> createTextBox(const Font*, int vWidth, int vHeight, bool useOpaqueHQ);

	// Note it draws in real pixels (like ::Draw)
	void Draw(const std::shared_ptr<TextBox>& tf) const;
	void Draw(const VBox& vbox, const lurp::Point& p) const;

	void doMove(const lurp::Point& screen, const lurp::Point& virt);
	std::shared_ptr<TextBox> doButton(const lurp::Point& screen, const lurp::Point& virt, bool down);

	void toggleQuality();

private:
	Font* getFont(const Font*);

	SDL_Renderer* _renderer;
	enki::TaskScheduler& _pool;
	TextureManager& _textureManager;
	std::vector<Font*> _fonts;
	std::vector<std::shared_ptr<TextBox>> _textFields;
	std::shared_ptr<TextBox> _mouseBox;
	XFormer _xf;
};

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize, const XFormer& xf);