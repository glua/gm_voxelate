do
	-- hotloading
	local function getHotloadingPath()
		local f = file.Open(".vox_hotloading","rb","DATA")
		if not f then return false end

		local str = f:Read(f:Size())
		f:Close()

		if not str then return false end
		return str
	end

	local hotloadBasePath = getHotloadingPath()

	if hotloadBasePath then
		print("[Voxelate Bootstrap] Attempting to hot-load code...")

		local OLD_FILETABLE = FILETABLE
		local module = G_VOX_IMPORTS

		local suc = pcall(require,"luaiox")

		if suc then
			print("[Voxelate Bootstrap] Hot-loading code using luaiox!")
		else
			print("[Voxelate Bootstrap] Hot-loading code using module.readFileUnrestricted!")
		end

		local function readFileUnrestricted(path)
			if suc then
				local f = io.open(hotloadBasePath..path,"rb")

				if f then
					local contents = f:read("*a")
					f:close()

					return contents
				end
			elseif module.readFileUnrestricted then
				return module.readFileUnrestricted(hotloadBasePath..path)
			end

			return OLD_FILETABLE[path]
		end

		FILETABLE = setmetatable({},{
			__index = function(_,path)
				return readFileUnrestricted(path)
			end
		})
	end
end

local entry_point = INSERT_ENTRY_POINT_FILENAME_HERE or "main.lua"

local source = FILETABLE[entry_point]

if source == nil then
	error("Bootstrap could not find entry point.")
end

RunString(source,entry_point)


--local __args = __args or {}
--_G.__args = nil

--[[local sandbox = {}

sandbox.internals = { module = G_VOX_IMPORTS }
sandbox.print = print

G_VOX_IMPORTS = nil

local function normalizePath(path)
	path = string.gsub(path,"\\","/")

	local segments = {}
	for segment in string.gmatch(path,"[^/]+") do
		if segment == "." then
			-- do nothing
		elseif segment == ".." then
			segments[#segments] = nil -- traverse backwards
		else
			segments[#segments + 1] = segment
		end
	end

	return table.concat(segments,"/")
end

function sandbox.dofile(_path)
	if type(_path) ~= "string" then error("Argument #1 to dofile needs to be a string!") end
	local path = normalizePath(_path)
	if FILETABLE[path] then
		local fn = CompileString(FILETABLE[path],path,false)

		if type(fn) == "function" then
			debug.setfenv(fn,sandbox)
			return fn()
		else
			print(fn)
			error(fn)
		end
	else
		-- print("File not found in compiled filetable!")
		error("File not found in compiled filetable!")
	end
end

function sandbox.loadfile(_path)
	if type(_path) ~= "string" then error("Argument #1 to loadfile needs to be a string!") end
	local path = normalizePath(_path)
	print(">",_path,path)
	if FILETABLE[path] then
		local fn = CompileString(FILETABLE[path],path,false)

		if type(fn) == "function" then
			debug.setfenv(fn,sandbox)
			return fn
		else
			print("Error compiling voxelate Lua file "..path..":\n"..fn)
			return false,fn
		end
	else
		-- print("File not found in compiled filetable!")
		error("File not found in compiled filetable!")
	end
end

function sandbox.fileContents(_path)
	if type(_path) ~= "string" then error("Argument #1 to loadfile needs to be a string!") end
	local path = normalizePath(_path)

	return FILETABLE[path]
end

setmetatable(sandbox,{__index = _G,__newindex = _G})

local mainFn,err = sandbox.loadfile(INSERT_ENTRY_POINT_FILENAME_HERE or "main.lua")
print("entry",INSERT_ENTRY_POINT_FILENAME_HERE,mainFn)

if mainFn then
	debug.setfenv(mainFn,sandbox)

	local suc,err = xpcall(mainFn,function(err)
		return debug.traceback(err,2)
	end)

	if not suc then
		error(err)
	end
else
	error(err or "couldn't find entry point!")
end
]]