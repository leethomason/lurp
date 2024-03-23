# Issues

## Open Issues

* Delete: Simple: minimal but complete
* Docs
  * Readme
  * Script docs (done)
* Deployment
* Auto-save before battle
* Test: area of effect spells
* Examples:
  * simplest zone (done)
  * simplest script
  * simplest battle (done)

## Ideas / Icebox

* error if calling random in eval() or test()
* multi-threaded driver

## Done

* npc is broken everywhere
* read from Entities
* testing
* get / set is unclear. Immutable?
* get / set needs testing. Does set do...nothing?
* tests suite should have a flag (for verbose output)
* Turn battle driver on
* Better looking console output
* EdgeGroup is a mess
* Reasonable save/load location
* NewsQueue and BattleQueue should be the same class
* Container: key
* CallScript: with the current approach to Script (sharing a table) does this do anything?
  It barely does - scriptID vs. entityID
* Battle: what is the interace? Savage worlds here we come.
* test: cycle
* Auto-EntityID: will stability be a problem?
* start, end, dying, winning
* save, load, cycle
* save everything & warn on generated IDs
* sort out interaction complete
* zone:move(), zone:warp()
* debug mode
* trace mode
* require a map for data structures, even if empty
* metatables
* Test and fix text eval(), code(), test
* Saving. 'persist' is too broad. Flip on as needed? Compare to const form?
* postCode() -- ?
* Support simple text: Text { "me", "hello there" }
* Choices v2
* Serialization
* fix: Script Text
* inventory
  * inventory API
  * is it the right model?
* eval() and code(): which goes first? Answer: eval(). confirm with test.
* pl -> Player?
* Decent looking drivers. Clean up usability.
* 'pop' shouldn't be a choice
* seperate game parts into a 2nd directory
* action.b.entityID should work with action.b and doesn't (fixed - how? just started working)
* NextRand() should be a method
* Drivers: start somewhere sane
