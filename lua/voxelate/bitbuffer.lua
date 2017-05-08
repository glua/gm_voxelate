local runtime,exports = ...

local UCHARPTR = UCHARPTR
local UCHARPTR_FromString = UCHARPTR_FromString
local sn_bf_read = sn_bf_read
local sn_bf_write = sn_bf_write

function exports.Writer(bytes)
    local ucptr = UCHARPTR(bytes)

    return sn_bf_write(ucptr)
end

function exports.Reader(data)
    local ucptr = UCHARPTR_FromString(data,#data)

    return sn_bf_read(ucptr)
end
