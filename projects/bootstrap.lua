local FILETABLE = FILETABLE
_G.FILETABLE = nil

local __args = __args or {}
_G.__args = nil

local sandbox = {}

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
            return fn()
        else
            print(fn)
            error(fn)
        end
    else
        print("File not found in compiled filetable!")
        error("File not found in compiled filetable!")
    end
end

function sandbox.loadfile(_path)
    if type(_path) ~= "string" then error("Argument #1 to loadfile needs to be a string!") end
    local path = normalizePath(_path)
    if FILETABLE[path] then
        local fn = CompileString(FILETABLE[path],path,false)

        if type(fn) == "function" then
            return fn
        else
            return false,fn
        end
    else
        print("File not found in compiled filetable!")
        error("File not found in compiled filetable!")
    end
end

function sandbox.fileContents(_path)
    if type(_path) ~= "string" then error("Argument #1 to loadfile needs to be a string!") end
    local path = normalizePath(_path)

    return FILETABLE[path]
end

local function cleanup()
    -- idk
end

setmetatable(sandbox,{__index = _G,__newindex = _G})

local mainFn,err = sandbox.loadfile(INSERT_ENTRY_POINT_FILENAME_HERE or "main.lua")

if mainFn then
    debug.setfenv(mainFn,sandbox)

    local suc,err = xpcall(mainFn,function(err)
        return debug.traceback(err,2)
    end,unpack(__args))

    cleanup()
    if not suc then
        error(err)
    end
else
    cleanup()
    error(err or "couldn't find entry point!")
end
