local runtime,exports = ...

local UCHARPTR = UCHARPTR
local UCHARPTR_FromString = UCHARPTR_FromString
local sn_bf_read = sn_bf_read
local sn_bf_write = sn_bf_write

function exports.Writer(bytes)
    local gap = 4 - bytes % 4

    local ucptr = UCHARPTR(bytes + gap)

    return sn_bf_write(ucptr)
end

function exports.Reader(data)
    local gap = 4 - #data % 4

    data = data..("\x00"):rep(gap)

    local ucptr = UCHARPTR_FromString(data,#data)

    return sn_bf_read(ucptr)
end
