local runtime,exports = ...

local ClientRouter = runtime.require("./networking/client").Router
local ServerRouter = runtime.require("./networking/server").Router

local VoxelWorldInitChannel = runtime.require("./channels/voxelworldinit").VoxelWorldInitChannel

local IO = runtime.require("./io").IO

local Voxelate = runtime.oop.create("Voxelate")

local module = G_VOX_IMPORTS
G_VOX_IMPORTS = nil
exports.module = module

function Voxelate:__ctor()
    self.module = module

    self.voxelWorldEnts = {}
    self.voxelWorldConfigs = {}

    self.channels = {}

    if CLIENT then
        self.io = IO:__new("Voxelate.CL",{r=255,g=255,b=0},{r=155,g=155,b=255})

        self.router = ClientRouter:__new(self)
    else
        self.io = IO:__new("Voxelate.SV",{r=0,g=155,b=255},{r=155,g=155,b=255})

        self.router = ServerRouter:__new(self)
    end

    self:AddChannel(VoxelWorldInitChannel,"voxelWorldInit",2)
end

function Voxelate:AddChannel(class,name,id)
    self.channels[name] = class:__new(name,id)
    self.channels[name]:BindToRouter(self.router)
end

function Voxelate:AddVoxelWorld(index,ent,config)
    self.voxelWorldEnts[index] = ent
    self.voxelWorldConfigs[index] = config
end

function Voxelate:GetWorldConfig(index)
    return self.voxelWorldConfigs[index]
end

function Voxelate:SetWorldConfig(index,config)
    self.voxelWorldConfigs[index] = config
end

exports.Voxelate = Voxelate:__new()
