local runtime,exports = ...

exports.networking = {}
exports.networking.ServerRouter = runtime.require("./networking/server").Router
exports.networking.ClientRouter = runtime.require("./networking/client").Router

local Voxelate = runtime.oop.create("Voxelate")

function Voxelate:__ctor()
    if CLIENT then
        self.router = exports.networking.ClientRouter:__new(self)
    else
        self.router = exports.networking.ServerRouter:__new(self)
    end
end

exports.Voxelate = Voxelate:__new()
