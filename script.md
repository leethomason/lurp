# Lua Interface

LuRP is a game engine written in C++ (the Core) that uses Lua to describe the game. All the parts of the game
you write will be in Lua. This document is the reference and explanation for declaring the game and
how the Lua interface works.

## General

Generally speaking, the Lua code is declarative. You are describing the game, not writing a program.
(`eval()` and `code()` and `test` are important exceptions.) LuRP uses standard, unmodified Lua, so
that you can use the standard environment to develop your game. (I use Visual Studio Code, but any
editor with Lua support should work.)

The Lua game code is read when the game starts. Which implies an important point: naively changing
something in Lua code after start won't do anything, because it has already been read and processed.

```lua
local myContainer = Container {
    name = "My Container",
    locked = true,
    items = {
        "GOLD",
    }
}
```

If in a `code()` function for example you later write:

```lua
myContainer.locked = false  -- this won't do anything!!
```

Because that references the immutable, already read Lua data. There are convenience objects (below)
to help, but this will work:

```lua
Entity("myContainer").locked = false
```

Because the `Entity()` function returns a "core table" that is mutable and tied into the game state.

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
  name = "Wooded Chest"
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
are eval() and code(). When the Lua functions are called, certain core tables are set in the global
scope for you to use:

* `script` - a table that exists for the duration of the script and all sub-scripts. When the
    script is complete the `script` table is destroyed.
* `player` - the player Actor
* `npc` - the Actor that is specified in the Interaction. nil if not specified. (Contextual: you
    expect the player to be talking to someone, but this can be nil if the player is perusing a
    bookcase.)
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

# Entities

## General Entities

Entities associated with both the Zones and the Scripts.

### Die & Dice

When a die or dice needs to be specified, you can do so with a string or number.

* 6 - a six-sided die
* "d6" - a six-sided die
* "2d6" - two six-sided dice
* "d6+2" - a six-sided die with a +2 bonus

### Item

* entityID - required for an Item. ex: "GOLD"
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

Some entitities have an inventory. (Actors and Containers). The inventory is a list of items
that the Entity starts with. It is a Lua table.

```lua
    items = {
        { "GOLD", 10 },   -- 10 gold coins
        "SKELETON_KEY"    -- skeleton key (1 is assumed)
    }
```

### Actor

An Actor is an NPC, character, or player in the game.

* entityID - usually the name, but can be any string. ex: "Grom" or "GROM"
* name - name as it appears in game. ex: "Grom"
* items - an Inventory of (initial, starting) items for this Actor

## Zone Interface

The Zone interface is used to declare the Zones in the game. A Zone is a collection of Rooms.
How you interpret a "Zone" or "Room" is completely up to you: they could be a dungeon, a continent,
a planet, etc. The player can move around a Zone from Room to Room, but has to travel between
Zones (using the `MovePlayer()` API).

### Zone

* entityID
* name

### Room

* entityID
* name
* desc

### Container

* entityID
* name
* locked: *initial* locked state
* key: itemID to unlock
* items[]: list of itemIDs or {itemID, count} in *initial* state
* eval()

### Edge

* entityID
* dir: 'n', 's', 'e', 'w' etc., optional direction from room1 to room2
* name: "door", "window", etc.
* room1: entityID
* room2: entityID
* locked: *initial* locked state
* key: itemID to unlock

### EdgeGroup

A tool to generate edges. Has no entityID, although the edges it generates do.

* name to display - the same for each room, but {dest} can be used to specialize.
* rooms[] list of entityID for each room in the group

### Interaction

An Interaction is something on the map you can talk to, investigate, look at, or otherwise interact with.
A required Interaction will be activated when the player enters the Room. Otherwise, the driver will
allow the player to select Interactions in the Room.
(Very similar to a CallScript, but with some extra features for working with Maps/Zones.)

* entityID/name
* eval() - determines if the interaction is avaibable. If not, will be hidden.
* code() - called when the interaction is activated
* next - script (table on entityID) to call when the interaction is activated
* actor - npc to use for the Interaction, if appropriate
* required - if true this Interaction will automaticall be run when the player enters the room. If there are multiple 'required' Interactions, they will be run in the order they are defined.

## Script Interface

### Script

* entityID
* code()
* events[] in the script, which can be other scripts or the types below.

### Text

* entityID
* eval()
* code()
* lines[]

### Lines

* {test}
* speaker
* text []

### Choices

* text
* next: EntityID, Script, or CallScript
  * 'done': End the choices. Move to the next event in the script.
    Default action.
  * 'repeat': repeats the choices.
  * 'rewind': move to the beginning of the script, replay the events.
  * 'pop': end this script. Return to the caller.
* eval()
* code()

### Battle

* entityID
* enemy: Actor

### CallScript

* entityID
* scriptID: the script to call
* npc: the npc used by the script
* params: lua object copied to the script context
* code()
* eval()

## Variable Resolution

Take 3, maybe?
Globals in script scope:

* script
* player
* npc
* zone
* room

Script:         LUA:                  CoreData:         EntityID:    path:          Notes
-------------   -----------------     ----------------  -----------  -------------  ----------------
script.flag     script.flag           _ScriptEnv.flag   _ScriptEnv   flag           there is no Entity() lookup! this is a temporary object.
                                                                                    although script.entityID == "_ScriptEnv"
player.hp       player.hp             player.hp         player       hp             noting that "player" == player.entityID so this one is odd
                Entity("player").hp
zone.name       zone.name             ZONE1.name        ZONE1        name           zone.entityID == "ZONE1"
                Entity(zone).name     
npc.traits.str  npc.traits.str        Grom.traits.str   Grom         traits.str     npc.entityID == "Grom"
                Entity("Grom").traits.str

# Serialization

Saved:
* CoreData
* Changed assets
* Map status
* Zone status

