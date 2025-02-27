# Haunted House Demo Game

A demo game of wandering around a haunted house, to test the basic game engine.

## Gameplay

Each Player has a "fear" level (0 - FEAR_MAX). If it reaches FEAR_MAX they
run out of the house and lose the game.

Players can explore the house. Each room has an event card that is flipped
when you enter the room. The event cards are usually ghosts and events
that increase fear.

Each room has one or two item (chits? cards?) that can be picked up for
various benefits. (Chits are just small cards?). They are flipped over when a
room is entered, even if they aren't picked up.

You win by collecting the magic book or being the last player standing.

## Setup

1. Each player starts with a fear level of 0 in the Foyer
2. Shuffle the Event cards and put one face down in each room.
3. Shuffle the Green Item cards and put one per each green circle. 
4. Shuffle the Red Item cards and put one per each red circle.

## Turns

For each player, you get 2 Actions.
* Move to an adjacent room (which turns over the Event card)
* Pick up an item (turn over and take the Item card)
* End turn

Let's make this simple!
