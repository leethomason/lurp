require "_board"
require "_boardmock"

local function initState()
    Game = _Game:new()
    Box = _Box:new()
    Players = _Players:new()
end

local function sequenceTest1()
    initState()

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

    _nextTurn() -- p1
    assert(Game:currentPlayer())
    assert(Game:currentPlayer().index == 1)
    assert(Players[1].start == 1)
    assert(Players[2].start == nil)

    _nextTurn() -- p2
    assert(Players[1].start == 0)
    assert(Players[2].start == 1)

    _nextTurn() -- p1
    assert(Game:currentPlayer().index == 1)
    assert(Players[1].start == 1)
    assert(Players[2].start == 0)

    -- Does skip and repeat work?
    -- simple case
    Players[2].skipTurn = 1
    _nextTurn()
    assert(Game:currentPlayer().index == 1)

    -- Complex case
    Players[1].skipTurn = 2
    Players[1].repeatTurn = 1
    Players[2].skipTurn = 1
    -- that should:
    --    P1 is current turn
    --    P1 repeats (repeat = 0)
    --      P1 turn
    --    P2 skips (skip = 0)
    --    P1 skips (skip = 1)
    --      P2 turn
    --    P1 skips (skip = 0)
    --      P2 turn

    _nextTurn()
    assert(Game:currentPlayer().index == 1)
    _nextTurn()
    assert(Game:currentPlayer().index == 2)
    _nextTurn()
    assert(Game:currentPlayer().index == 2)
    _nextTurn()
    assert(Game:currentPlayer().index == 1)

    assert(Players[1].start == 1)
    assert(Players[2].start == 0)

    -- now a player leaves (on their own turn)
    assert(not GameOver())
    SetPlayerOver(Players[1])
    _nextTurn()
    assert(Game:currentPlayer().index == 2)
    _nextTurn()
    assert(Game:currentPlayer().index == 2)

    SetPlayerOver(Players[2])
    assert(GameOver())
end

sequenceTest1()

print("Board mock tests complete.")