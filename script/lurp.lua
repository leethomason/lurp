local lurp = {}

-- fixme: move in serialize and deserialize

function lurp.deepClone(src, dst)
    assert(src)
    dst = dst or {}

    for k,v in pairs(src) do
        if type(v) == "table" then
            dst[k] = lurp.deepClone(v)
        else
            dst[k] = v
        end
    end
end

return lurp