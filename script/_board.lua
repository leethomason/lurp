Table = {}

function Table:new()
    local o = {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function Table:add(key, value)
    assert(key)
    assert(value)
    self[key] = value
end

function Table:remove(key)
    self[key] = nil
end

function Table:has(key)
    return self[key] ~= nil
end

function Table:get(key)
    return self[key]
end

function Table:push(value)
    table.insert(self, value)
end

function Table:filter(func)
    local t = Table:new()
    local index = 1
    for _,v in ipairs(self) do
        if func(v) then
            t[index] = v
            index = index + 1
        end
    end
    return t
end

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

    return o
end

Box = {}
Box.meeples = Table:new()

Players = Table:new()

Player = {
    index = 0,
    meeples = {},
}

function Player:new(index)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    o.index = index
    o.meeples = Table:new()
    return o
end

function _createPlayers(nPlayers)
    for i = 1, nPlayers do
        local p = Player:new(i)
        assert(type(p) == "table")
        Players:push(p)
    end
    print("Players", #Players)
end
