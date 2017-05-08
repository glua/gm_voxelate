local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

function Router:__ctor(voxelate)
    self.voxelate = voxelate

    hook.Add("Tick","Voxelate.Networking.Polling",function()
        self.voxelate.module.networkPoll()
    end)
end

function Router:GeneratePUID()
    -- PUID = Pseudo-unique identifier

    return string.format(
        "%08x-%04x-%04x-%08x",
        math.random(2 ^ 32),
        math.random(2 ^ 16),
        math.random(2 ^ 16),
        math.random(2 ^ 32)
    )
end
