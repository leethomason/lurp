require "_board"
require "_boardmock"

local function sequenceTest1()
    -- new game
    local b = _onFetchBoard()
    assert(type(b) == "table")
    _onSetupBox()
    assert(Box.init == true)

    _createPlayers(2)
    assert(#Players == 2)
    assert(Players[1].index == 1)
    assert(Players[2].index == 2)

    _onSetupGame()
    assert(Game.setup == true)
    assert(#Players == 2)

    _onInit()
    assert(Game.init == true)
    assert(Game:currentPlayer() == nil)

    _nextTurn()
    assert(Game:currentPlayer())
    assert(Game:currentPlayer().index == 1)
    assert(Players[1].start == 1)
    assert(Players[2].start == nil)

    _nextTurn()
    assert(Players[1].start == 0)
    assert(Players[2].start == 1)
end

sequenceTest1()

print("Board mock tests complete.")