Zones = {}
Items = {}
Powers = {}
Combatants = {}
Scripts = {}
Actors = {}
Texts = {}
Battles = {}
AllChoices = {}
Rooms = {}
Containers = {}
Edges = {}
Interactions = {}
CallScripts = {}
Entities = {}

local soloMode = false

Rand = {}

Rand.random = function(low, high)
    local x = CRandom()
    if (low == nil) and (high == nil) then
        return (x % 10001) / 10000.0
    end
    if (high == nil) then
        high = low
        low = 1
    end
    return low + (x % (high - low + 1))
end

Rand.dice = function(n, d, b)
    if (b == nil) then b = 0 end
    local sum = 0
    for i = 1, n do
        sum = sum + (1 + CRandom() % d)
    end
    return sum + b
end

local function doFilePath(filename)
    if not DIR then
        dofile(filename)
    else
        dofile(DIR .. filename)
    end
end

function PrintTable(t, d)
    if d == nil then d = 0 end
    for k, v in pairs(t) do
        if type(v) == "table" then
            print(k, " = table")
            PrintTable(v, d + 1)
        else
            for i = 0, d do
                io.write(". ")
            end
            print(k, v)
        end
    end
end

-- Move the player to a new room.
-- @param dstID The entityID of the room to move to.
-- @param teleport If true, the move always succeeds. If false, it's a regular move
--         and the player must pass any locks on the edge.
function MovePlayer(dstID, teleport)
    if type(dstID) == "table" then
        dstID = dstID.entityID
    end
    CMove(dstID, teleport == true)
end

function EndGame(reason, bias)
    bias = bias or 0
    CEndGame(reason, bias)
end

local poolID = {}
local function createEntityID(entityID, base)
    if entityID then return entityID end

    assert(base ~= nil, "base required")

    if not poolID[base] then poolID[base] = 0 end
    poolID[base] = poolID[base] + 1

    return "_GEN_"..base.."_"..poolID[base]
end

local function addToEntities(t)
    if Entities[t.entityID] then
        print("[ERROR] The entity "..t.entityID.." already exists.")
    end
    assert(t.entityID ~= nil, "entityID required")
    assert(Entities[t.entityID] == nil, "entityID already exists")
    Entities[t.entityID] = t
end

local function addItemMethods(t)

    t.addItem = function(self, itemID, n)
        if n == nil then n = 1 end
        if type(itemID) == "table" then
            itemID = itemID.entityID
        end
        assert(type(itemID) == "string", "itemID must be string")
        CDeltaItem(self.entityID, itemID, n);
    end

    t.removeItem = function(self, itemID, n)
        if n == nil then n = 1 end
        if type(itemID) == "table" then
            itemID = itemID.entityID
        end
        assert(type(itemID) == "string", "itemID must be string")
        CDeltaItem(self.entityID, itemID, -n);
    end

    t.numItems = function(self, itemID)
        assert(type(itemID) == "string" or type(itemID) == "table", "input must be entity or entityID")
        if type(itemID) == "table" then
            itemID = itemID.entityID
        end
        assert(type(itemID) == "string", "itemID must be string")
        return CNumItems(self.entityID, itemID);
    end

    t.hasItem = function(self, itemID)
        return self:numItems(itemID) > 0
    end
end

function Script(d)
    d.type = "Script"
    d.entityID = createEntityID(d.entityID, "SCRIPT")
    Scripts[d.entityID] = d
    addToEntities(d)
    return d
end

function Actor(actor)
    actor.type = "Actor"
    actor.entityID = createEntityID(actor.entityID, "ACTOR")
    Actors[actor.entityID] = actor
    addToEntities(actor)
    addItemMethods(actor)
    return actor
end

function Text(t)
    t.type = "Text"
    t.entityID = createEntityID(t.entityID, "TEXT")
    Texts[t.entityID] = t
    addToEntities(t)
    return t
end

function Battle(b)
    b.type = "Battle"
    b.entityID = createEntityID(b.entityID, "BATTLE")
    Battles[b.entityID] = b
    addToEntities(b)
    return b
end

function Combatant(c)
    c.type = "Combatant"
    c.entityID = createEntityID(c.entityID, "COMBATANT")
    Combatants[c.entityID] = c
    addToEntities(c)
    return c
end

function Choices(c)
    c.type = "Choices"
    c.entityID = createEntityID(c.entityID, "CHOICES")
    AllChoices[c.entityID] = c
    addToEntities(c)
    c.allTextRead = function(self)
        return CAllTextRead(self.entityID)
    end
    return c
end

function Item(i)
    i.type = "Item"
    assert(i.entityID ~= nil, "Items require an 'entityID'")
    i.entityID = createEntityID(i.entityID, nil)
    Items[i.entityID] = i
    addToEntities(i)
    return i
end

function Power(p)
    p.type = "Power"
    assert(p.entityID ~= nil, "Powers require an 'entityID'")
    p.entityID = createEntityID(p.entityID, nil)
    Powers[p.entityID] = p
    addToEntities(p)
    return p
end

function Zone(z)
    z.type = "Zone"
    assert(z.entityID ~= nil, "Zones require an 'entityID'")
    z.entityID = createEntityID(z.entityID, nil)
    Zones[z.entityID] = z
    addToEntities(z)
    return z
end

function Room(r)
    r.type = "Room"
    assert(r.entityID ~= nil, "Rooms require an 'entityID'")
    r.entityID = createEntityID(r.entityID, nil)
    Rooms[r.entityID] = r
    addToEntities(r)
    return r
end

function Edge(e)
    e.type = "Edge"
    if e.entityID == nil then
        e.entityID = "_EDGE::" .. e.room1 .. "-" .. e.room2  -- stable entityID
    end
    Edges[e.entityID] = e
    addToEntities(e)
    --addLockMethods(e)
    return e
end

function EdgeGroup(g)
    for i = 1, #g.rooms - 1 do
        for j = i + 1, #g.rooms do
            local e = {}
            e.type = "Edge"
            e.entityID = "_GEN_EDGE::" .. g.rooms[i] .. "-" .. g.rooms[j]
            e.room1 = g.rooms[i]
            e.room2 = g.rooms[j]
            e.name = "{dest}"       -- small hack; used to assign dest name in dirEdges. magic value.
            Edges[e.entityID] = e
            addToEntities(e)
            -- can't lock edges in a group
        end
    end
end

function Container(c)
    c.type = "Container"
    c.entityID = createEntityID(c.entityID, "CONTAINER")
    Containers[c.entityID] = c
    addToEntities(c)
    addItemMethods(c)
    return c
end

function Interaction(c)
    c.type = "Interaction"
    local base = "INTERACTION"
    local append = ""

    if not c.entityID and not c.name then
        if type(c.next) == "table" then
            if c.next.entityID then
                append = c.next.entityID
            elseif c.next.name then
                append = c.next.name
            end
        elseif type(c.next) == "string" then
            append = c.next
        end
    end

    if string.len(append) > 0 and string.sub(append, 1, 5) ~= "_GEN_" then
        base = base.."::"..append
    end

    c.entityID = createEntityID(c.entityID, base)
    Interactions[c.entityID] = c
    addToEntities(c)
    return c
end

function CallScript(cs)
    cs.type = "CallScript";
    cs.entityID = createEntityID(cs.entityID, "CALLSCRIPT")
    CallScripts[cs.entityID] = cs
    return cs
end

-- sentinels

Script { entityID = "done" }
Script { entityID = "repeat" }
Script { entityID = "rewind" }
Script { entityID = "pop" }

local function CoreTable(e)
    -- CoreTables are constructed AFTER the file is read & parsed.
    -- So the actual C++ entities are set up.

    local ePrime = {}

    -- Immutable Entity Prop - might as well just copy
    ePrime.entityID = e.entityID
    ePrime.name = e.name
    ePrime.type = e.type
    ePrime.desc = e.desc

    for k, v in pairs(e) do
        if (type(k) == "string") then
            -- Immutable User Prop & Methods
            if type(v) == "table" or type(v) == "function" then
                ePrime[k] = v
            end
            -- Mutable User Prop
            if type(v) == "number" or type(v) == "string" or type(v) == "boolean" then
                if not ePrime[k] then
                    CCoreSet(e.entityID, k, v, true)
                end
            end
        end
    end

    -- Mutable Entity Prop - need to reach into core
    ePrime.locked = nil
    ePrime.items = nil

    -- Special values
    ePrime._src = e -- the original source table, which is used to fall back from a CoreGet
    ePrime._isCoreTable = true -- general debugging check

    assert(ePrime.entityID, "Must have an entityID to construct a core table")
    return setmetatable(ePrime, {
        __index = function(t, key)
            -- print("__index", t.entityID, key)
            -- the core can have a value set to nil, and that
            -- overrides the default. this necessitates returning
            -- 'okay' if the core has the value separate from the value
            local okay, v = CCoreGet(t.entityID, key)
            if okay then return v end
            -- if not in the core, return the immutable source value
            return t._src[key]
        end,
        __newindex = function(t, key, value)
            -- print("__newindex", t.entityID, key, value)
            CCoreSet(t.entityID, key, value, false)
        end,
    })
end

local coreCache = {}
function Entity(entityID)
    if Entities[entityID] == nil then
        print("[ERROR] Entity '"..entityID.."' not found.")
    end
    assert(entityID ~= nil, "entityID required")
    assert(Entities[entityID] ~= nil, "entityID not found")

    local t = coreCache[entityID]
    if not t then
        t = CoreTable(Entities[entityID])
        coreCache[entityID] = t
    end
    assert(t._isCoreTable, "coreCache returned table that is not a core table")
    return t
end

function SetupScriptEnv(_script, _npc, _zone, _room)
    --print("SetupScriptEnv", _script, _player, _npc, _zone, _room)
    script = CoreTable({ entityID = _script })
    player = Entity("player")

    npc = {}
    if _npc then
        npc = Entity(_npc)
    else
        npc = nil
    end

    zone = {}
    if _zone then
        zone = Entity(_zone)
    end

    room = nil
    if (_room) then
        room = Entity(_room)
    end
    --print("Script", script, script._isCoreTable)
    assert(script._isCoreTable, "script is not a core table")   
end

function SetupNPCEnv(_npc)
    --print("SetupNPCEnv", _npc)
    if script == nil then
        print("WARNING: SetupNPCEnv() script is not set")
        return false
    end
    if npc ~= nil then
        print("WARNING: SetupNPCEnv() npc is already set")
        return false
    end
    npc = Entity(_npc)
    assert(npc._isCoreTable, "npc is not a core table")
    return true
end

function ClearScriptEnv()
    --print("ClearScriptEnv", script, player, npc, zone, room)
    assert(script._isCoreTable, "script is not a core table")
    coreCache[script.entityID] = nil
    script = nil
    player = nil
    npc = nil
    zone = nil
    room = nil
end

if DIR == nil then
    DIR = ""
    RUN_TESTS = true
    soloMode = true

    CRandom = function()
        return math.random(0, 1024 * 1024 * 1024)
    end

    CDeltaItem = function(containerID, itemID, n)
        print("CDeltaItem", containerID, itemID, n)
    end

    CNumItems = function(containerID, itemID)
        print("CNumItems", containerID, itemID)
        return 0
    end

    CCoreSet = function(scopeID, varID, value, mutUser)
        print("CCoreSet", scopeID, varID, value, mutUser)
    end

    -- returns present, value
    CCoreGet = function(entityID, key)
        print("CCoreGet", entityID, key)
        return true, 0
    end

    CAllTextRead = function(entityID)
        print("CAllTextRead", entityID)
        return true
    end

    CMove = function(dstID, teleport)
        print('CMove', dstID, teleport)
    end

    CEndGame = function(reason, bias)
        print('CEndGame', reason, bias)
    end

    script = {}
    player = {}
    npc = {}
    zone = {}
    room = {}

    print("Running map.")
end

if RUN_TESTS then
    doFilePath("testscript.lua")
    doFilePath("testzones.lua")
end

if soloMode then
    for index, value in pairs(Scripts) do
        print("->", index, value, value.entityID, type(value.entityID))
    end
end

_TestFuncCallbacks = function ()
    --print("TestFuncCallbacks")
    CRandom()
    --CIsLocked("none")
    --CSetLocked("none", true)
    CDeltaItem("none", "none", 3)
    CDeltaItem("none", "none", -2)
    CNumItems("none", "none")
    CCoreSet("none", "none", 3, false)
    CCoreGet("none", "none")
end

