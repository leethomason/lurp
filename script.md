# Game Declaration and Scripting Reference

LuRP is a game engine written in C++ (the Core) that uses Lua to describe the game. Although Lua
is used as the programming language, it is used in a very declarative way. You can write a story
without any scripting. Even at intermediate complexity, the scripting is simple and very
"cut and paste".

This file is a reference the objects you can declare and what they do.

If you haven't already, you should start with the general README.md file. And take a look at
`example-zone`, `example-script`, or `example-battle` before diving into this reference.

## General

Generally speaking, the Lua code is declarative. You are describing the game, not writing a program.
(`eval()` and `code()` and `test` are important exceptions.) LuRP uses standard, unmodified Lua, so
that you can use the standard environment to develop your game. (I use Visual Studio Code, but any
editor with Lua support should work.)

The Lua game code is read when the game starts. Which implies an important point: changing
something in Lua code after start won't do anything, because it has already been read and processed.

Repeating this important point: changing the Lua tables after the game starts won't do anything.
It has already been read.

```lua
local myContainer = Container {
    name = "My Container",
    locked = true,
    items = {
        "GOLD",
    }
}
```

If in a `code()` function (for example) you write:

```lua
myContainer.locked = false  -- this won't do anything!!
```

Because that references the immutable, already read Lua data. This is what a working
version looks like:

```lua
Entity("myContainer").locked = false -- this will unlock the door
```

Because the `Entity()` function returns a `CoreTable` that is mutable and tied into the game state.

Which brings up the question: what is an Entity?

### Entity

An Entity is a thing in the game. It can be a room, an item, a character, a script, etc. It must have
a globally unique ID. You will get an error if you try to create two entities with the same
entityID. There are a few ways to specify an entityID:

#### Fully Specified

Here is a "fully specified" Entity:

```lua
Container {
  entityID = "CHEST_01",
  name = "Wooden Chest"
}
```

To unlock it, you would write:

```lua
Entity("CHEST_01").locked = false
```

#### Name Only

Sometimes, that is more info than needed. For a Actor (for example) they should have unique
names. So if a name is provided, and not an entityID, then the name is used as the entityID.

```lua
Actor {
  name = "Grom",
}
```

And this accesses the Actor:

```lua
if Entity("Grom").STR >= 18 then...
```

But generally, you won't need that, because the relevant Actor is set in the 'npc' variable:

```lua
if npc.STR >= 18 then...
```

#### Automatic

The engine will automatically create an entityID for you if you don't provide one. Handy!

```lua
Interaction {
    required = true,
    next = "COOL_INTERACTION_01",
}
```

BUT! There are drawbacks.

1. There's no way to get a core table to that entity. So you can't access it from `eval()` or `code()`.
2. The auto-generated entity IDs can change whenever you update the game. So if you save a game, and
   then update the game, the entity IDs will change. LuRP will try to do the best in can, but you
   can easily end up having the save reverted to an earlier state or just plane broken. More info
   in "Serialization" below.

## Core vs. Lua Declaration

## Lua calls

Lua is used to declare the game. However, you can declare functions that get called. These functions
are `eval()` and `code()`. When the Lua functions are called, certain core tables are set in the global
scope for you to use:

* `script` - a table that exists for the duration of the script and all sub-scripts. When the
  script is complete the `script` table is destroyed.
* `player` - the player Actor
* `npc` - the Actor that is specified in the Interaction. nil if not specified. In a conversation,
  you would expect `npc` to be a table, but if browsing books it's likely `nil`.
* `zone` - the Zone that the player is currently in.
* `room` - the Room that the player is currently in.

### eval()

`eval()` is called to determine in an entity, script, etc. is available. It should return `true` or
`false`/`nil`. If it returns `false` or `nil`, then the entity is hidden from the player.

WARNING: `eval()` can be called multiple times. It must be a pure function that doesn't change the
state of the game. It also needs to be consistent and return the same value for the same game state.

Example:

```lua
eval = function() return player:hasItem("ARTIFACT") end 
-- good! reads a value and turns true/false
```

```lua
eval = function() return Rand.random(1, 10) == 1 end 
-- BAD! returns a different value and will break the game logic
```

```lua
eval = function() player:addItem("ARTIFACT") return true end 
-- BAD! changes the game state. the player will have many ARTIFACTs...or none.
```

### code()

`code()` is called when an entity, script, etc. is activated. It is called once, and only once. Unlike
`eval()`, it can change the state of the game, and can use random numbers. It's return value is ignored.

Example:

```lua
code = function() zone.hadMorningCoffee = true end
```

## Global APIs

### Random

There are a few random number generators in LuRP. The Lua `math.random` is available, but it is not
shared with the Core LuRP random number generator. It's a good idea to use these APIs over the
Lua ones.

* `Rand.random(low, high)` -> number or integer - works just like Lua's `math.random`
* `Rand.dice(n, sides, bonus)` -> integer - rolls `n` dice with `sides` and adds `bonus`

### Inventory API

The inventory API allows you to add, remove, and query Items in an Actor or Container. Be careful
when removing an item from somewhere to add it somewhere else, so it isn't lost.

'item' is an Item table or Item entityID. 'n' is assumed to be 1 if not specified.

* `e:addItem(item, n)`
* `e:removeItem(item, n)`
* `e:hasItem(item) -> bool`
* `e:numItems(item) -> integer`

### Game Control

* `e:allTextRead() -> bool`
  Returns true if all the text at that Entity, and all its children,
  has been read. This is useful for determining if a tree of Text
  is 'done'. See 'Choices' for more description.
* `MovePlayer(roomEntityID, teleport)`
  Moves the player to the room specified by its EntityID. If teleport is false/nil, then
  locks etc. will be checked, and MovePlayer works just like the player moving to an
  adjacent room.

  If teleport is true, then the player is moved without checking locks. The destination
  room does not need to be adjacaent or even in the same Zone. This allows for travelling
  to a new Zone.
* `EndGame(reason)` - flags the driver the game is over, for the 'reason' string specified.
  The driver will then exit the game.

## General Entities

Entities associated with both the Zones and the Scripts.

### General Entity Notes

#### Die & Dice

When a die or dice needs to be specified, you can do so with a string or number.

* "6" - a six-sided die
* "d6" - a six-sided die
* "2d6" - two six-sided dice
* "d6+2" - a six-sided die with a +2 bonus

### Item

* entityID
* name - name as read. ex: "gold coin"
* desc - description of the item. ex: "A shiny gold coin."

IF the Item is a melee weapon:

* range = 0
* damage = "d6" - the damage die

IF the Item is a ranged weapon:

* range = 1 or more - the medium range of the weapon in yards/meters
* damage = "d6" - the damage die
* ap - the armor penetration of the weapon

IF the Item is armor:

* armor = 2 - the armor value

### Inventory

Some entitities have an inventory. (Actors, Combatants, and Containers). The inventory is a list of items that the Entity starts with. It is a Lua table.

```lua
    items = {
        { "GOLD", 10 },   -- 10 gold coins
        "SKELETON_KEY"    -- skeleton key (1 is assumed)
    }
```

### Actor

An Actor is an NPC, character, or player in the game.

* entityID
* name - name as it appears in game. ex: "Grom"
* items - an Inventory of (initial, starting) items for this Actor

## Zone Interface

The Zone interface is used to declare the Zones in the game. A Zone is a collection of Rooms.
How you interpret a "Zone" or "Room" is completely up to you: they could be a dungeon, a continent,
a planet, etc. The player can move around a Zone from Room to Room, but has to travel between
Zones (using the `MovePlayer()` API).

### Zone

The Zone is a top level group of Rooms. You should
interpret "Zone" and "Room" to make sense for your
game. Perhaps a Zone is a dungeon full of rooms...
or perhaps a Zone is a contintent with countries
you can visit.

* entityID
* name

### Room

A Room is a place your player can be, and they
can travel to other rooms. An Edge connects
Rooms together.

* entityID
* name
* desc

### Edge

An Edge allows travel between rooms.

* entityID
* name - "door", "window", etc.
* dir - an optional direction ('n', 'ne', 'e', 'w' etc.) optional from room1 to room2. If not specified,
the Rooms are still connected, but without a 
particular direction.
* room1 - entityID (required)
* room2 - entityID (required)
* locked - initial locked state
* key - entityID of the Item to unlock. A Room can also be locked
  or unlocked via script, but specifying the key
  is often easier.

### EdgeGroup

A tool to generate edges. Has no entityID. This allows you to connect
a group of Rooms to each other. For instance,
a central plaza may connect to many buildings,
and this allows you to set up all the connections at once.
Rooms can have other Edges that you set up.

* rooms - table of entityIDs to connect.

### Container

Containers hold stuff the player can interact with. Chests, barrels, etc.

* entityID
* name - "iron chest" for example
* `eval()` - is the container visible?
* locked - initial locked state
* key - itemID to unlock
* items - table of items initially present in the container

### Interaction

An Interaction is something on the map you can talk to, investigate, look at, or otherwise interact with.
A required Interaction will be activated when the player enters the Room. Otherwise, the driver will
allow the player to select Interactions in the Room.

* entityID
* name
* `eval()` - determines if the interaction is avaibable. If not, will be hidden.
* `code()` - called when the interaction is activated
* next - script (table or entityID) to call when the interaction is activated
* actor - npc to use for the Interaction, if appropriate
* required - if true this Interaction will automaticall be run when the player enters the room. If there are multiple 'required' Interactions, they will be run in the order they are defined.

## Script Interface

Entities that work inside Scripts.

### Script

A Script is a sequence of actions and events. A mini
story. Scripts can call other scripts, further the
plot, and are the building block of the game.

* entityID
* `code()` - called when this Script is activated

### Text

Many games contain lots of text, and Text is a very
flexible object. Start simple if you are learning.

* entityID
* `eval()` - determines if the text is available. If not, will be hidden.
* `code()` - called when the text is about to be read
* `test` - a test to determine if the text should be shown (see examples below)

The simplest example:

```lua
Text {
  "Hello, there."
}
```

Is a block of text, shown to the player. You can show
multiple lines of text:

```lua
Text {
  "Hello, there", "You've travelled far."
}
-- OR --
Text {
  { "Hello, there." },
  { "You've travelled far." }
}
```

The difference in how they are displayed is left
to the driver.

You may want to specify a speaker, which you can with `s`:

```lua
Text {
  s = "Ben", "Hello, there"
}
```

You can use `eval()` and `code()`

```lua
Text {
  eval = function() return player:hasItem("ARTIFACT") end,
  s = "Guardian",
  "You've found the artifact."
}
-- OR --
Text {
  { s = "Guardian", "Welcome to the temple." },
  {
      eval = function() return player:hasItem("ARTIFACT") end,
      s = "Guardian",
      "You've found the artifact."
  }  
}
-- OR even... ---
Text {
  eval = function() return zone.guardianAwake end
  { s = "Guardian", "Welcome to the temple." },
  {
      eval = function() return player:hasItem("ARTIFACT") end,
      s = "Guardian",
      "You've found the artifact."
  }  
}
```

Text supports a terse text that is sometimes useful. This doesn't do anything special, and does
less than `eval()`, but it's sometimes a nice shortcut.

```Lua
    Text {
        { test = "{player.isDruid}", "You see the rare bloom Evensky everywhere." },
        { test = "{~player.isDruid}", "You see pretty flowers." }
    }
```

If `isDruid` is true or "truthy" on the player object, then the first line will be shown. If it is
false, nil, or "falsy", then the second line will be shown.

### Choices

Options presented to the player, that can trigger other events. Also dialog trees.

Note: Dialog trees should be a special case of Choices. A future version of LuRP will have a
more specialized dialog tree.

* text - brief text describing the choice
* next - the Entityt to call when the choice is made. Scripts, Text, etc. 
  There are also special values:
  * 'done' - End the choices. Move to the next event in the script.
    Default action.
  * 'repeat' - repeats the choices. Common in dialog.
  * 'rewind': move to the beginning of the script, replay the events.
  * 'pop': end this script. Return to the caller.
* eval()
* code()

### Battle

Battles instert fights, conflicts, and action into your game. The Battle objects
is based on the Savage Worlds RPG system (SWADE). It is an excerpt of the rules,
but gear tables should work straight out of the rules, and the basic combat is
implemented as well.

See "example-battle" for a simple example. Battles aren't complex, but they take
setup beyond a single Entity.

### CallScript

Calls another script. Useful to re-use script, set up variable and code, test
before calling, and similar.

* entityID
* scriptID - the script to call
* npc - the npc used by the script (optional)
* code()
* eval()
