# LuRP

![Build](https://github.com/leethomason/lurp/actions/workflows/c-cpp.yml/badge.svg)

A story engine for Windows. (Mac and Linux support is planned.)

LuRP is a story engine. It provides more services than a scripting engine, but less than a game
engine. It strives to "play well with others" so you can integrate LuRP into your engine or
framework of choice.

So much of what an adventure or role playing game is - whether it is a modern RPG or a
classic Infocom text game - is a story telling engine. There are actors, dialog, objects,
locations, and events. LuRP extracts those elements and is a framework for creating role
playing, adventure, and visual novel games.

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

## Status

LuRP is in development. It is quite usable for writing games and playing them on the console.
A graphical 2D driver is very important. Documentation gets fixed as needed, but there are
certainly still errors. The API will evolve as the ideas evolve. How much LuRP develops
is realistically dependent on how much interest there is in it, of course. Features and PRs
drive priorities.

## Feedback

Is most welcome. Please open an issue for any problems, questions, or suggestions. If needed
and desired, I may open a "discussion" or "wiki" pages.

### Games (Chullu, example-xyz)

Happy to take improvements to the writing. The `example` games should remain small and focused,
so I don't want too many words there. `chullu` would be fun to improve and expand into a proper
game. (The new location should be in separate files, so there's some work to do there.)

All content should remain appropriate for general audiences.

### Engine

Pull requests for the C++ / Lua code appreciated as PRs. Changes to the API surface will be
required but require some thought and balancing concerns. (Usability, limitations of the
C++/Lua interface, etc.)

### Console / 2D Driver

Always looking to make that look better and be more usable.

## Installation: Give it a Try

### Windows

Unzip the release somewhere convenient. (You can just unzip it into the "Downloads" direcory.)

You can double-click on `lurp.exe` from the Windows Explorer, and it will run the console
and present you with a list of (simple) games to try.

If you are running LuRP from the command line, you can just run `lurp.exe` from the installation
directory. If you want to specify a game:

```shell
lurp <path/to/lua/file> <starting-zone>
```

You need to be in directory of the `lurp.exe` file, but the file you
run can be anywhere. The `starting-zone` is optional.

## Writing a Game

### Basic Concepts

### Battle System

## Future Work: 2D Driver
