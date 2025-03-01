require "_board"

local function assertIsType(t, expected, n)
    assert(t ~= nil, "Expected non-nil value")
    assert(type(t) == expected, "Expected type " .. expected .. ", got " .. type(t))
    if n then
        assert(#t == n, "Expected table length " .. n .. ", got " .. #t)
    end
end

local function structCompare(t1, t2)
    if type(t1) ~= type(t2) then
        return false
    end

    -- skip functions
    if (type(t1) == "function") then
        return true
    end

    if type(t1) ~= "table" then
        return t1 == t2
    end

    for k, v in pairs(t1) do
        if not structCompare(v, t2[k]) then
            print("#1 k:", k, "v:", v, "t2[k]:", t2[k])
            return false
        end
    end

    for k, v in pairs(t2) do
        if not structCompare(v, t1[k]) then
            print("#2 k:", k, "v:", v, "t2[k]:", t2[k])
            return false
        end
    end

    return true
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

local function serializeTest1()
    local test = {
        value = 1,
        valueStr = "str",
        valueBool = true
    }
    local s = serialize(test, {}, 0)
    local tbl = load("return " .. s)()
    assert(type(tbl) == "table")
    local t = deserialize(tbl, {})

    assert(t.value == 1)
    assert(t.valueStr == "str")
    assert(t.valueBool == true)
end

local function serializeTest2()
    local test = {
        value = 1,
        valueStr = "str",
        valueBool = true,
        inner = {
            value = 2,
            valueStr = "str2",
            valueBool = false
        }
    }
    local s = serialize(test, {}, 0)
    local tbl = load("return " .. s)()
    assert(type(tbl) == "table")
    local t = deserialize(tbl, {})

    assert(t.value == 1)
    assert(t.valueStr == "str")
    assert(t.valueBool == true)

    assert(t.inner.value == 2)
    assert(t.inner.valueStr == "str2")
    assert(t.inner.valueBool == false)
end

local function cycleTestAssert(t)
    assertIsType(t, "table", 2)
    assertIsType(t[1], "table")
    assertIsType(t[1].meeples, "table", 1)
    assertIsType(t[1].fear, "table")
end

local function cycleTest()
    initState()
    local data = dofile("boardsave.lua")
    cycleTestAssert(data.Players)

    _deserialize(data)

    local s = _serialize()
    local fp = io.open("boardsaveCycle.lua", "w")
    fp:write(s)
    fp:close()

    assertIsType(Game, "table")
    assertIsType(Box, "table")
    assertIsType(Players, "table")
    assertIsType(Box.meeples, "table", 5)
    cycleTestAssert(Players)

    local data2 = dofile("boardsaveCycle.lua")
    assert(structCompare(data, data2))
end

local function loadTest()
    initState()
    local data = dofile("boardsave.lua")
    assert(type(data) == "table")

    initState()
    _deserialize(data)
end

function cardHandler()
    return 17
end

local function cardTest()
    initState()
    local card = Card:new("testCard", "A Test Card", "no desc", cardHandler)
    assert(card.handler() == 17)
end

meepleTests()
serializeTest1()
serializeTest2()
cycleTest()
loadTest()
--cardTest()

-- Tests needed:
--   - Turn test: skip, repeat, single player

print("Board tests complete.")
dofile("_boardtestmock.lua")