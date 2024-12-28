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

Meeple = {
    x = 0,
    y = 0,
    pos = "",
    type = "player-meeple",
    color = "white",

    isPlaced = false
}

function Meeple:new()
    local o = {}
    setmetatable(o, self)
    self.__index = self
    return o
end

Box = {}
Box.meeples = Table:new()
