#pragma once

#include <string>

struct SDL_Renderer;
struct SDL_Texture;
class Texture;

void DrawDebugText(const std::string& text, SDL_Renderer* renderer, const Texture* tex, int x, int y, int fontSize = 16);