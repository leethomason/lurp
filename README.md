# LuRP

![Build](https://github.com/leethomason/lurp/actions/workflows/c-cpp.yml/badge.svg)

A story engine for Windows. Mac and Linux support coming soon.

LuRP is a story engine. It provides more services than a scripting engine, but less than a game
engine. It strives to "play well with others" so you can integrate LuRP into your engine or
framework of choice.

The Lua Role Playing game engine is a framework for creating role playing, adventure, and visual 
novel games. 

The idea of LuRP is to write the parts of a game - characters, items, rooms, story,
choices, etc. - seperate from how the game is drawn. An entire LuRP game could be written and
played in text basically just an old-school adventure game. But with a little
more meta-data, the game could be played in a 2D or even 3D engine. Not all those pieces are
in place yet. But the core of the idea - that the story part of the game is seperate from the
presentation - is in place.

LuRP games are written in a very declarative style of Lua. While there is some scripting,
it's mostly about describing the story and what can happen. The core engine in C++ then
tracks all the state and runs the story.

What presents that story to the user (or the developer) is a "driver". The driver could be
a console driver (text), a 2D driver, a 3D driver, or even a web driver. The driver is responsible
for showing the story and getting input. The interface from the driver to LuRP is a simple
one.

A command line (console) driver is provided. A 2D visual driver is in the works.

## Give it a Try

### Windows

Unzip the release somewhere convenient. (You can just unzip it into the "Downloads" direcory.)

You can double-click on `lurp.exe` from the Windows Explorer, and it will run the console
and present you with a list of (simple) games to try.

From the command line,

```
lurp <path/to/lua/file> <starting-zone>
```

Will run the game. You need to be in directory of the `lurp.exe` file, but the file you
run can be anywhere. The `starting-zone` is optional.

## A Simple Example

## Console Driver

## 2D Driver

## Getting Started Writing a Game

