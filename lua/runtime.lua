-- LuaExtended-compatible Lua Runtime Helper Library

assert(jit and jit.version == "LuaJIT 2.0.4","This runtime library is built for LuaJIT 2.0.4!")

local runtime = {}
runtime.version = 20170509

local debug = debug

local load = load

if CompileString then
	load = function(code,ident)
		local fn = CompileString(code,ident,false)

		if type(fn) == "function" then
			return fn
		else
			return false,fn
		end
	end
end

runtime.sources = {}
runtime.__rtlFn = debug.getinfo(1,"f").func
runtime.sources[runtime.__rtlFn] = "runtime"

function runtime.load(code,ident)
	return runtime.bind(load(code,ident),runtime)
end

function runtime.source(extraDepth)
	extraDepth = extraDepth or 1
	local depth,info = 0

	repeat
		info = debug.getinfo(extraDepth + depth,"f")
		depth = depth + 1
	until info and (info.func ~= runtime.__rtlFn) and runtime.sources[info.func] or not info

	return info and runtime.sources[info.func] and runtime.sources[info.func]:match("^(.+[/\\]).-$") or ""
end

function runtime.setSource(src)
	local fn = debug.getinfo(2,"f").func
	runtime.sources[fn] = src
end

function runtime.loadfile(path)
	local fullPath = runtime.source(3)..path
	local func = loadfile(fullPath,fullPath)
	if func then
		runtime.sources[func] = fullPath
		return runtime.bind(func,runtime)
	end

	func = loadfile(path,path)
	if func then
		runtime.sources[func] = path
		return runtime.bind(func,runtime)
	end
end

function runtime.bind(fn,self)
	return function(...) return fn(self,...) end
end

-- [[ package ]]

runtime.__packages = {}

runtime.internal = {}

runtime.internal.filePaths = {
	"%s",
	"%s.lua",
	"%s/init.lua",
	"%s/%s.lua",
}

runtime.internal.installedPaths = {
	"__modules/%s",
	"__modules/%s.lua",
	"__modules/%s/%s.lua",
	"__modules/%s/init.lua",
}

if os.getenv and os.getenv("LUA_EXTENDED_PACKAGE") then
	for path in os.getenv("LUA_EXTENDED_PACKAGE"):gmatch("[^;]+") do
		runtime.internal.installedPaths[#runtime.internal.installedPaths + 1] = path
	end
end

function runtime.internal.require(name)
	local suc1,fn = pcall(runtime.loadfile,name)
	if not suc1 then return false,false,fn end
	local mod = {}
	local suc2,err = xpcall(fn,function(err)
		print("Error loading module "..name)
		print(debug.traceback(err))
	end,mod)
	if not suc2 then return false,true,err end
	return true,true,mod
end

function runtime.normalizePath(name)
	name = name:gsub("\\","/")

	local stack = {}
	local isRelative = false
	if name:sub(1,1) == "." then isRelative = true end

	for segment in name:gmatch("[^/]+") do
		if segment == "." then
			-- nothing
		elseif (segment == "..") and (stack[#stack] ~= nil) then
			stack[#stack] = nil
		else
			stack[#stack + 1] = segment
		end
	end

	return table.concat(stack,"/"),stack[#stack],isRelative
end

function runtime.require(name)
	local name,filename,isRelative = runtime.normalizePath(name)

	if runtime.__packages[name] then return runtime.__packages[name] end

	if isRelative then
		-- file module
		for i,pathfmt in ipairs(runtime.internal.filePaths) do
			local fmt = string.format(pathfmt,name,filename)

			local found,suc,ret = runtime.internal.require(fmt)

			if found or (not tostring(ret):match("File not found in compiled filetable!")) then
				if suc then
					runtime.__packages[name] = ret
					return ret
				else
					error(ret)
				end
			end
		end
	else
		-- installed module
		for i,pathfmt in ipairs(runtime.internal.installedPaths) do
			local fmt = string.format(pathfmt,name,filename)

			local found,suc,ret = runtime.internal.require(fmt)

			if found or (not tostring(ret):match("File not found in compiled filetable!")) then
				if suc then
					runtime.__packages[name] = ret
					return ret
				else
					error(ret)
				end
			end
		end
	end

	error("Cannot find package '"..name.."'")
end

runtime.oop = {}

runtime.oop.ptr = 0

function runtime.oop.uuid()
	runtime.oop.ptr = runtime.oop.ptr + 1

	return runtime.oop.ptr
end

function runtime.oop.superBind(origSelf,inputSelf,fn)
	return setmetatable({},{
		__call = function(_,self,...)
			if self == inputSelf then
				self = origSelf
			end

			return fn(self,...)
		end,
		__tostring = function(self)
			return string.format("SuperFunction: %p",self)
		end,
	})
end

function runtime.oop.construct(meta,...)
	local instance = setmetatable({},meta)

	instance.__uuid = runtime.oop.uuid()

	-- HACK: table:__gc is not supported in LuaJIT 2.0.3
	instance.__gcproxy = newproxy(true)
	instance.__gcproxymeta = {}
	debug.setmetatable(instance.__gcproxy,instance.__gcproxymeta)

	-- HACK: table:__gc is not supported in LuaJIT 2.0.3
	instance.__gcproxymeta.__gc = function() return instance:__gc() end

	if instance.__parent then
		instance.super = {}
		setmetatable(instance.super,{
			__index = function(self,k)
				local v = instance.__parent[k]

				if type(v) == "function" then
					return runtime.oop.superBind(instance,self,v)
				else
					return v
				end
			end,
		})
	end

	instance:__ctor(...)

	return instance
end

function runtime.oop.create(name,secure)
	local meta = {}

	meta.__name = name
	meta.__secure = secure and true
	-- meta.__parent = nil

	function meta:__gc()
		-- if not self.__destroyed then
		--	 self.__destroyed = true
		--	 self:__dtor()
		-- end
	end
	function meta:__ctor(...)
		if self.__parent and self.__parent.__ctor then return self.__parent.__ctor(self,...) end
	end
	function meta:__dtor(...)
		if self.__parent and self.__parent.__dtor then return self.__parent.__dtor(self,...) end
	end

	function meta:__tostring()
		return string.format("%s [%d]",self.__name,self.__uuid)
	end

	-- meta.__index = function(self,k)
	--	 if rawget(self,k) ~= nil then return rawget(self,k) end
	--	 if meta[k] ~= nil then return meta[k] end
	--	 if meta.__parent then return meta.__parent[k] end
	-- end

	meta.__index = meta

	function meta:__new(...)
		return runtime.oop.construct(meta,...)
	end

	return meta
end

function runtime.oop.checkCircular(meta,deps)
	if not meta then return end
	deps = deps or {}
	deps[meta] = true
	if deps[meta.__parent] then error("Class "..meta.__name.." has a circular dependency!") end
	runtime.oop.checkCircular(meta.__parent,deps)
end

function runtime.oop.extend(meta,source)
	meta.__parent = source
	if meta.__parent then
		setmetatable(meta,meta.__parent)
	end
	runtime.oop.checkCircular(meta)
end

function runtime.oop.destroy(instance)
	return instance:__dtor()
end

return runtime
