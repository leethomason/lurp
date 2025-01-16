require "_board"

local function assertIsType(t, expected)
    assert(t ~= nil, "Expected non-nil value")
    assert(type(t) == expected, "Expected type " .. expected .. ", got " .. type(t))
end

local function dTable(n, t)
    print(n, t)
    print("  meta:", getmetatable(t))
    for k, v in pairs(t) do
        print("  ", k, v)
    end
end

local function initState()
    Game = _Game:new()
    Box = _Box:new()
    Players = _Players:new()
end

local function meepleTests()
    initState()

    -- new a Meeple
    local m0 = Meeple:new("player", "P1", "red")
    m0.w = 7

    assert(type(m0) == "table")
    assert(m0.name == "player") -- set on m0
    assert(m0.label == "P1")
    assert(m0.x == -1) -- set on Meeple
    assert(m0.color == "red")
    assert(m0.z == nil)
    assert(m0.w == 7)

    -- loading a Meeple - no new()
    local m1data = {
        name = "player",
        label = "P2",
        color = "blue",
    }

    local m1 = Meeple:load(m1data)
    m1.w = 8

    assert(type(m1) == "table")
    assert(m1.name == "player") -- set on m0
    assert(m1.label == "P2")
    assert(m1.x == -1) -- set on Meeple
    assert(m1.color == "blue")
    assert(m1.z == nil)
    assert(m1.w == 8)

    m0:moveTo(7, 8)
    assert(m0.x == 7)
    assert(m0.y == 8)
    m1:moveTo(9, 10)
    assert(m1.x == 9)
    assert(m1.y == 10)
end

local function cycleTest()
    initState()
    local data = dofile("boardsave.lua")
    _deserialize(data)

    assertIsType(Game, "table")
    assertIsType(Box, "table")
    assertIsType(Players, "table")

    local s = _serialize()
    local fp = io.open("boardsaveCycle.lua", "w")
    fp:write(s)
    fp:close()
end

local function loadTest()
    initState()
    local data = dofile("boardsave.lua")
    assert(type(data) == "table")

    initState()
    _deserialize(data)
end

meepleTests()
cycleTest()
loadTest()

print("Board tests complete.")