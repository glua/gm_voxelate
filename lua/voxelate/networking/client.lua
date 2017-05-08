local runtime,exports = ...

local Router = runtime.oop.create("Router")
exports.Router = Router

function Router:__ctor(voxelate)
    self.voxelate = voxelate
end
