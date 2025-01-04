-- Rules / assumptions
-- 1. a. The board is a graph of rooms. OR
--    b. The board is a grid of rooms. 
--    Can't mix graphs and grids.
-- 2. The board is static. Rooms don't move. Although the can be blocked. See onMoveMeeple()
-- 3. A player can have n meeples. TBD: move each meeple? actions per meeple?

-- Pieces:
--   - Meeple - markers, mini-figs, etc.
--   - Card 
--   - Tiles - TBD. "My First Carcassonne" has tiles. etc.
--   - Plate - character card? What to call this?
--   - Counter - some number tracker. e.g. money, points, etc.

------ List ------

List = {}

function List:new()
    local o = {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function List:add(key, value)
    assert(key)
    assert(value)
    self[key] = value
end

function List:remove(key)
    self[key] = nil
end

function List:has(key)
    return self[key] ~= nil
end

function List:get(key)
    return self[key]
end

function List:push(value)
    table.insert(self, value)
end

function List:filter(func)
    local t = List:new()
    local index = 1
    for _,v in ipairs(self) do
        if func(v) then
            t[index] = v
            index = index + 1
        end
    end
    return t
end

------ Utility ------

local uidCounter = 0

function getUID()
    uidCounter = uidCounter + 1
    return uidCounter
end

------ Meeple ------

Meeple = {
    -- Location.
    -- x,y for grid, pos for graph/room based boards
    -- note that x,y is 0 based. -1 means not on board
    x = -1,
    y = -1,
    pos = "",

    name = "",          -- name of the meeple
    label = "",         -- label to display
    color = "blue",
}

function Meeple:new(name, label, color)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    o.name = name
    o.label = label
    o.color = color
    o.uid = getUID()

    return o
end

------ Box ------

Box = {}
Box.meeples = List:new()

------ Player ------

Players = List:new()

Player = {
    index = 0,
    inPlay = true,
    meeples = List:new(),
    counters = List:new(),
}

function Player:new(index)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    o.index = index
    o.meeples = List:new()
    return o
end

------ Board ------

Board = {}

------ Counter ------

Counter = {
    --name = "",
    value = 0,
    min = 0,
    max = 1,
    incValue = 1,
}

function Counter:new(value, min, max, incValue)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    --o.name = name
    o.value = value 
    o.min = min
    o.max = max
    o.incValue = inc or 1

    assert(o.value >= o.min)
    assert(o.value <= o.max)
    return o
end

function Counter:inc()
    self.value = self.value + self.incValue
    if self.value >= self.max then
        self.value = self.max
        if self.onMax then self.onMax() end
    end
end

function Counter:dec()
    self.value = self.value - self.incValue
    if self.value <= self.min then
        self.value = self.min
        if self.onMin then self.onMin() end
    end
end

------ API Functions ------

local gameOver = false

function GameOver()
    gameOver = true
end

function PlayerOver(index)
    Players[index].inPlay = false
    local anyInPlay = false
    for _, v in ipairs(Players) do
        if v.inPlay then
            anyInPlay = true
            break
        end
    end
    if not anyInPlay then
        GameOver()
    end
end

------ Internal API Functions ------

function _isGameOver()
    return gameOver
end

function _createPlayers(nPlayers)
    for i = 1, nPlayers do
        local p = Player:new(i)
        assert(type(p) == "table")
        Players:push(p)
    end
    --print("Players", #Players)
end

function _onFetchBoard()
    Board = onFetchBoard()
    return Board;
end

function _queryPlayerFromIndex(index)
    assert(index > 0 and index <= #Players)
    return Players[index]
end

function _queryMeepleFromUID(uid)
    for _, v in ipairs(Box.meeples) do
        if v.uid == uid then
            return v
        end
    end
    assert(false)
    return nil
end

function _queryCellFromName(name)
    for _, v in ipairs(Board) do
        if v.name == name then
            return v
        end
    end
    assert(false)
    return nil
end
