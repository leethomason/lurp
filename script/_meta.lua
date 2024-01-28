local a = {
    entityID = "ENTITY_A",
    name = "Entity A",
}

local function CoreTable(e)
    -- Set to nil to allow the core to overwrite. The core already has copied these values.
    e.locked = nil
    e.items = nil
  
    local t = setmetatable(e, {
        __index = function(t, key)
            local okay, v = CCoreGet(t.entityID, key)
            if okay then
                return v
            end
            return nil
        end,
        __newindex = function(t, key, value)
            CCoreSet(t.entityID, key, value, true)
        end,
    })
    return t
end

print("start")
local aPrime = CoreTable(a)
print("aPrime", aPrime)
print("aPrime.entityID", aPrime.entityID)
print("aPrime.name", aPrime.name)
print("aPrime.foo", aPrime.foo)
aPrime.bar = "bar"
