local lume = require "lume"
require "_util"

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

function serialize(x, stk, depth)
    stk = stk or {}
    depth = depth or 0

    local t = type(x)

    if t == "number" or t == "boolean" then
        return tostring(x)
    elseif t == "string" then    
        return string.format("%q", x)
    elseif t == "table" then
        if stk[x] then
            return string.rep("  ", depth) .. "nil --[[ circular reference ]]"
        else
            stk[x] = true
            local s = "{\n"

            -- The unsorted k values are annoying. Sort them.
            local keys = {}
            for k, _ in pairs(x) do
                table.insert(keys, k)
            end
            table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)

            for i, k in ipairs(keys) do
                local v = x[k]
                s = s .. string.rep("  ", depth + 1) .. "[" .. serialize(k, stk, depth + 1) .. "] = " .. serialize(v, stk, depth + 1) .. ",\n"
            end
            return s .. string.rep("  ", depth) .. "}"
        end
    else
        return "nil --[[ " .. t .. " ]]"
    end
end

-- serialize goes from a values to a string.
-- deserialize, on the other hand, has a table loaded in memory,
-- and needs to copy to a different one.
function deserialize(s, stk)
    local t = type(s)
    
    if t == "number" or t == "boolean" or t == "string" then
        return s
    elseif t == "table" then
        if s.uid and stk[s.uid] then
            return stk[s.uid]
        end
        local o = {}
        for k,v in pairs(s) do
            o[k] = deserialize(v, stk)
        end
        if s.uid then
            stk[s.uid] = o
        end
        return o
    else
        return nil
    end
    assert(false)
end

------ Game ------
_Game = {}

function _Game.init(o)
    -- struct
    o.struct = "Game"

    -- data 
    o._uidCounter = 0
    o._currentTurn = 1

    -- methods
    o.new = _Game.new
    o.load = _Game.load
    o.getUID = _Game.getUID
    o.currentPlayer = _Game.currentPlayer
    o.getPlayerFromIndex = _Game.getPlayerFromIndex
    o.nextTurn = _Game.nextTurn

    return o
end

function _Game:new()
    local o = {}
    _Game.init(o)
    return o
end

function _Game:load(g)
    local o = _Game.init({})

    assert(type(g) == "table")
    for k, v in pairs(g) do
        o[k] = deserialize(v, _tableCache)
    end
    return o
end

function _Game:getUID()
    self._uidCounter = self._uidCounter + 1
    return self._uidCounter
end

-- returns the current Player or nil if the current player isn't inPlay
function _Game:currentPlayer()
    return self:getPlayerFromIndex(self._currentTurn)
end

function _Game:getPlayerFromIndex(index)
    assert(index > 0 and index <= #Players)
    local p = Players[index]
    if p.inPlay then
        return p
    end
    return nil
end

function _Game:nextTurn()
    local current = self:currentPlayer()
    local startIdx = self._currentTurn

    if current then
        assert(current.skipTurn >= 0)
        assert(current.repeatTurn >= 0)

        onEndTurn(current)

        if current.repeatTurn > 0 then
            current.repeatTurn = current.repeatTurn - 1
            onStartTurn(current)
            return
        end
    end

    while true do
        self._currentTurn = incMod(self._currentTurn, #Players)
        if self._currentTurn == startIdx then
            -- No need to skip: BUT, may need to call actors. TBD.
            local p = self:currentPlayer()
            if p then
                p.skipTurn = 0
            end
            break
        end
        local p = self:currentPlayer()
        assert(p)
        if p.skipTurn > 0 then
            p.skipTurn = p.skipTurn - 1
        else
            break
        end
    end
    onStartTurn(self:currentPlayer())
end

Game = _Game:new()

------ Meeple ------

_Meeple = {}

function _Meeple.init(o)
    o.struct = "Meeple"

    -- Location.
    -- x,y for grid, pos for graph/room based boards
    -- note that x,y is 0 based. -1 means not on board
    o.x = -1
    o.y = -1
    o.pos = ""

    o.name = ""      -- name of the meeple
    o.label = ""     -- label to display
    o.color = nil    -- color of the meeple (optional)

    o.new = _Meeple.new
    o.load = _Meeple.load
    o.moveTo = _Meeple.moveTo

    return o
end

function _Meeple:load(obj)
    assert(type(self) == "table")
    assert(type(obj) == "table")

    local o = _Meeple.init({})
    for k, v in pairs(obj) do
        o[k] = v
    end
    return o
end

function _Meeple:new(name, label, color)
    assert(type(self) == "table")

    local o = Meeple.init({})

    o.name = name
    o.label = label
    o.color = color
    o.uid = Game:getUID()

    return o
end

function _Meeple:moveTo(x, y)
    assert(type(self) == "table")
    self.x = x
    self.y = y
end

Meeple = _Meeple

------ Box ------

_Box = {}

function _Box.init(o)
    o.struct = "Box"

    o.meeples = {}

    o.new = _Box.new
    o.load = _Box.load

    return o
end

function _Box:new()
    assert(type(self) == "table")
    local o = {}
    _Box.init(o)
    return o
end

function _Box:load(obj)
    assert(type(self) == "table")
    assert(type(obj) == "table")
    local o = _Box.init({})

    for k, v in ipairs(obj.meeples) do
        local m = Meeple:load(v)
        lume.push(o.meeples, m)
    end
    return o
end

Box = _Box:new()

------ Players ------

_Players = {}

function _Players.init(o)
    o.struct = "Players"

    o.new = _Players.new
    o.load = _Players.load

    return o
end

function _Players:new()
    assert(type(self) == "table")
    local o = {}
    _Players.init(o)
    return o
end

function _Players:load(loader)
    assert(type(self) == "table")
    assert(type(loader) == "table")

    local o = {}
    o.struct = "Players"

    for _, v in ipairs(loader) do
        local p = _Player:load(v)
        lume.push(o, p)
    end
    return o
end

Players = _Players:new()

------ Player ------

_Player = {}

function _Player.init(n)
    n.struct = "Player"

    n.index = 0
    n.inPlay = true
    -- Note that both skip and repeat can be set. Repeat takes precedence.
    -- and then skip will kick in when the turn comes around.
    n.skipTurn = 0  -- skip turn counter
    n.repeatTurn = 0  -- repeat turn counter
    n.meeples = {}
    
    n.new = _Player.new
    n.load = _Player.load
    return n
end

function _Player:load(obj)
    assert(type(self) == "table")
    local o = {}
    _Player.init(o)

    for k, v in pairs(obj) do
        o[k] = deserialize(v, _tableCache)
    end
    return o
end

function _Player:new(index)
    assert(type(self) == "table")
    local o = {}
    _Player.init(o)
    return o
end

Player = _Player

------ Board ------

Board = {}
Board.struct = "Board"

------ Counter ------

_Counter = {}

function _Counter.init(o)
    o.struct = "Counter"

    o.value = 0
    o.min = 0
    o.max = 1
    o.incValue = 1

    o.init = _Counter.init
    o.load = _Counter.load
    o.inc = _Counter.inc
    o.dec = _Counter.dec

    return o
end

function _Counter:new(value, min, max, incValue)
    assert(type(self) == "table")

    local o = _Counter.init({})

    o.value = value
    o.min = min
    o.max = max
    o.incValue = incValue or 1

    assert(o.value >= o.min)
    assert(o.value <= o.max)
    return o
end

function _Counter:load(obj)
    assert(type(self) == "table")
    local o = _Counter.init({})
    for k, v in pairs(obj) do
        o[k] = v
    end
    return o
end

function _Counter:inc()
    assert(type(self) == "table")
    self.value = self.value + self.incValue
    if self.value >= self.max then
        self.value = self.max
        if self.onMax then self.onMax() end
    end
end

function _Counter:dec()
    assert(type(self) == "table")
    self.value = self.value - self.incValue
    if self.value <= self.min then
        self.value = self.min
        if self.onMin then self.onMin() end
    end
end

Counter = _Counter

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

function _onSetupBox()
    assert(Box)
    
    if onSetupBox then
        return onSetupBox(Box)
    end
    return 0
end

function _onSetupGame()
    assert(Players)
    assert(Box)

    if onSetupGame then
        onSetupGame(Players, Box)
    end
end

function _onInit()
    assert(Players)
    assert(Box)

    if onInit then
        onInit(Players, Box)
    end
end

function _isGameOver()
    return gameOver
end

function _createPlayers(nPlayers)
    for i = 1, nPlayers do
        local p = Player:new(i)
        assert(type(p) == "table")
        lume.push(Players, p)
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

function _queryCurrentPlayerIndex()
    return Game._currentTurn
end

function _serialize()
    local s = "local loader = {}\n"

    s = s .. "loader.Game =\n"
    s = s .. serialize(Game) .. "\n"
    s = s .. "\nloader.Box =\n"
    s = s .. serialize(Box) .. "\n"
    s = s .. "\nloader.Players =\n"
    s = s .. serialize(Players) .. "\n"
    s = s .. "\nreturn loader"
    return s
end

function _deserialize(loader)
    assert(type(loader) == "table")
    assert(type(loader.Game) == "table")
    assert(type(loader.Box) == "table")
    assert(type(loader.Players) == "table")

    _tableCache = {}
    Game = _Game:load(loader.Game)
    Box = _Box:load(loader.Box)
    Players = _Players:load(loader.Players)
end
