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
choices, etc. - separate from how the game is drawn. An entire LuRP game could be written and
played in text basically just an old-school adventure game. But with a little
more meta-data, the game could be played in a 2D or even 3D engine. Not all those pieces are
in place yet. But the core of the idea - that the story part of the game is separate from the
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

The example games are a good place to start to see how to write a game. You should
start with `example-zone` and then `example-script`. `example-battle` can wait until
you wish to add combat.

### Basic Concepts

The world of your game is made up of *Zones*. Each Zone has *Rooms* that are connected
to each other by *Edges*. The Zone, Rooms, and Edges create the basic map of your game.

There are *Actors* in the game. Actors can be in a Room. There are *Items* in the world,
that can be in containers or held by Actors. Items may be weapons, keys, gold, or almost
anything.

*Interaction*s are things or people or events that you can talk to or interact with
through *Script*s. Scripts are the heart of the game, and allow for *Text*, *Choices*,
and branching logic.

Zones, Scripts, Interactions, Items, and many others are Entities.
[script.md](script.md) is a reference to the various game entities.

### Battle System

The battle system is based on the [Savage Worlds Adventure Edition](https://peginc.com/savage-settings/savage-worlds/) (SWADE) rules.
It is a subset - the full rule system goes well beyond the scope of combat -
but does provide ready-to-go combat system so you don't have to roll your own.

There's lots to love about SWADE, but where it matches well for LuRP is that it
is generic rules that don't assume a setting. Modern, Future, and Ancient settings
work just as well as Medieval Fantasy. You can take the values from the "Gear" section
straight into LuRP items, which gives a good reference for consistent definitions.

The Savage World Adventure Edition Core Rules cover everything. LuRP is not affiliated
with SWADE or Pinnacle in any way. (Except being a fan of the rule set.)

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

## Future Work

### Usability

Writing some games and using that to inform how to improve, streamline, and make the API
more powerful will help. The simple games are too small to really stress the API usability.

Will be investigating this area, and very open to feedback on how to improve!

### 2D Driver

Having a 2D driver (a la Ren'Py for example) is the other short term missing component.
Games need visuals! Even simple ones. The other main area of improvement going forward.

### Linux and OSX

The code runs on these platforms already, but needs to be packaged up and the deployment
sorted out. (Where are save games placed? How is the binary bundled for release?)

## Thanks

Thank you for checking out LuRP - I hope you find it useful!
