require "_board"

local function dTable(n, t)
    print(n, t)
    print("  meta:", getmetatable(t))
    for k, v in pairs(t) do
        print("  ", k, v)
    end
end

-- new a Meeple
local m0 = Meeple:new("player", "P1", "red")
--m0.w = 7

assert(type(m0) == "table")
assert(m0.name == "player") -- set on m0
assert(m0.label == "P1")
assert(m0.x == -1) -- set on Meeple
assert(m0.color == "red")
assert(m0.z == nil)
--assert(m0.w == 7)

-- loading a Meeple - no new()
local m1data = {
    name = "player",
    label = "P2",
    color = "blue",
}

local m1 = Meeple:load(m1data)
--m1.w = 8

dTable("m0", m0)
dTable("m1", m1)

assert(type(m1) == "table")
assert(m1.name == "player") -- set on m0
assert(m1.label == "P2")
assert(m1.x == -1) -- set on Meeple
assert(m1.color == "blue")
assert(m1.z == nil)
--assert(m1.w == 8)

print("m0", m0, "m0.x", m0.x, "m0.y", m0.y)
m0:moveTo(7, 8)
assert(m0.x == 7)
assert(m0.y == 8)
print("m1")
m1:moveTo(9, 10)
assert(m1.x == 9)
assert(m1.y == 10)

print("Board tests complete.")