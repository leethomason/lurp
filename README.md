# LuRP

![Build](https://github.com/leethomason/lurp/actions/workflows/c-cpp.yml/badge.svg)

LuRP is a story engine. It provides more services than a scripting engine, but less than a game 
engine. It strives to "play well with others" so you can integrate LuRP into your engine or
framework of choice.

The Lua Role Playing game engine is a framework for creating role playing, adventure, and visual 
novel games. 

The idea of LuRP is to write the parts of a game - characters, items, rooms, story,
choices, etc. - seperate from how the game is renderer. An entire LuRP game could be written and
played on the "console driver" which is just the command line interface. But with a little
more meta-data, the game could be played in a 2D or even 3D engine. Not all those pieces are
int place yet. But the core of the idea - that the story part of the game is seperate from the
presentation - is in place.

LuRP games are written in a very declarative style of Lua. While there is some scripting,
it's mostly about describing the story and what can happen. The core engine in C++ then
tracks all the state and runs the story.

What presents that story to the user (or the developer) is a "driver". The driver could be
a console driver, a 2D driver, a 3D driver, or even a web driver. The driver is responsible
for showing the story and getting input. The interface from the driver to LuRP is a simple
one.

A command line (console) driver is provided. A 2D visual driver is in the works.

## A Simple Example

## Console Driver

## 2D Driver

## Getting Started Writing a Game


