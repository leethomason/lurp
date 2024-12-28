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
    x = 0,
    y = 0,
    pos = "",
    type = "player",
    id = "",
    color = "blue",
}

function Meeple:new(type, str, color)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    o.type = type
    o.id = str
    o.color = color

    return o
end

Box = {}
Box.meeples = Table:new()

Players = Table:new()

Player = {}

function Player:new(index)
    local o = {}
    setmetatable(o, self)
    self.__index = self

    o.index = index
    return o
end

function _createPlayers(nPlayers)
    for i = 1, nPlayers do
        Players:push(Player:new(i))
    end
end
