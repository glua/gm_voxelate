local runtime,exports = ...

local ClientRouter = runtime.require("./networking/client").Router
local ServerRouter = runtime.require("./networking/server").Router

local IO = runtime.require("./io").IO

local Voxelate = runtime.oop.create("Voxelate")

local module = G_VOX_IMPORTS
G_VOX_IMPORTS = nil

function Voxelate:__ctor()
    self.module = module

    if CLIENT then
        self.io = IO:__new("Voxelate.CL",{r=255,g=255,b=0},{r=155,g=155,b=255})

        self.router = ClientRouter:__new(self)
    else
        self.io = IO:__new("Voxelate.SV",{r=0,g=155,b=255},{r=155,g=155,b=255})

        self.router = ServerRouter:__new(self)
    end
end

exports.Voxelate = Voxelate:__new()
