local asr = {}
function asr.equal(a, b) return a == b end
function asr.error(fn, ...) local ok, out = pcall(fn, ...) return (not ok) and out end
function asr.match(a, b) return type(a) == "string" and a:match(b) end
function asr.type(a, b) return type(a) == b end

local mt = {}
function mt.__index(t, k)
    if not asr[k] then return end
    return function(...)
        local ok = asr[k](...)
        if ok then return ok end
        local args = {}
        for i = 1, select('#', ...) do
            args[#args+1] = tostring(select(i, ...))
        end
        error(string.format("assert.%s failed; args: %s", k, table.concat(args, ", ")), 2)
    end
end
mt.__call = assert
return setmetatable({}, mt) 
