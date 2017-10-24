
local function print(...)
	local args = {...}

	for i=1,#args do
		args[i] = tostring(args[i])
	end

	MsgC( Color(100,255,100), "[Voxelate] ", SERVER and Color(0x91,0xdb,0xe7) or Color(0xe7,0xdb,0x74), table.concat(args,"\t") , "\n" )
end

local modules = {}
local INTERNALS = G_VOX_IMPORTS
voxelate_internals = G_VOX_IMPORTS
_G.G_VOX_IMPORTS = nil

local FILETABLE = FILETABLE
_G.FILETABLE = nil

local sandbox = {}
sandbox.print = print
sandbox.internals = INTERNALS

local function normalizePath(path)
	path = path:gsub("\\","/")

	local stack = {}

	for segment in path:gmatch("[^/]+") do
		if segment == "." or segment == "" then
			-- nothing
		elseif (segment == "..") and (stack[#stack] ~= nil) then
			stack[#stack] = nil
		else
			stack[#stack + 1] = segment
		end
	end

	return table.concat(stack,"/")
end

local endings = { ".lua", "/init.lua" }

-- My version of require is not as smart as I thought. Beware of tailcalls!
sandbox.require = function(name)
	local base_path = debug.getinfo(2).short_src:GetPathFromFilename()

	local abs_path = normalizePath(base_path..name)

	if modules[abs_path] then
		return modules[abs_path]
	end

	for _,ending in pairs(endings) do
		local full_path = abs_path..ending
		local source = FILETABLE[full_path]

		if source then
			local func = CompileString(source,full_path)
			setfenv(func, sandbox)

			local module = func()
			if not module then module = true end

			modules[abs_path] = module

			return module
		end
	end

	error("Module not found: "..name)
end

setmetatable(sandbox,{__index = _G,__newindex = _G})

sandbox.require("voxelate")

print("Lua initialization complete.")
