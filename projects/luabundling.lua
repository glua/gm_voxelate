-- from one of my old projects

_LUA_EXEC_DIRECTORY = _SCRIPT_DIR
if _LUA_EXEC_DIRECTORY:sub(1,1):match("[/\\]") then _LUA_EXEC_DIRECTORY = _LUA_EXEC_DIRECTORY:sub(1,-2) end

local function normalizePath(path)
    path = string.gsub(path,"\\","/")

    local segments = {}
    for segment in string.gmatch(path,"[^/]+") do
        if segment == "." then
            -- do nothing
        elseif segment == ".." then
            if segments[#segments] == nil then
                error("A project file is outside the root project dir! ("..path..")")
            end
            segments[#segments] = nil -- traverse backwards
        else
            segments[#segments + 1] = segment
        end
    end

    return table.concat(segments,"/")
end

local function compileLuaProject(p)
    local fileTable = {}

    for _,toInclude in ipairs(p.luaFiles) do
        for _,filePath in ipairs(os.matchfiles(toInclude)) do
            filePath = os.realpath(filePath)
            local f = io.open(filePath,"rb")
            if f then
                local relPath = path.getrelative(p.rootLuaDirectory,filePath)
                print("Reading "..relPath.."...")
                fileTable[normalizePath(relPath)] = f:read("*a")
                f:close()
            else
                -- uh oh!
                error("Cannot find file "..f)
            end
        end
    end

    local function cppString(str)
        local body = {"{\n"}

        str = str.."\0"
        for i = 1, #str do
            body[#body + 1] = string.format("%3d, ", str:byte(i))
            if (i % 32 == 0) then
                body[#body + 1] = "\n"
            end
        end

        body[#body + 1] = "\n}"
        return table.concat(body,"")
    end

    local serializedFileTable = {}
    serializedFileTable[#serializedFileTable + 1] = "void addFile(const unsigned char path[],int pathlen,const unsigned char contents[],int contlen);\n"

    local filesSeen = 0
    for filePath,contents in pairs(fileTable) do
        filesSeen = filesSeen + 1
        print("Compiling as "..filePath.."...")
        serializedFileTable[#serializedFileTable + 1] = string.format(
            "const unsigned char file%d_path[] = %s; int file%d_path_len = %d;\n",
            filesSeen,
            cppString(filePath),
            filesSeen,
            #filePath
        )
        serializedFileTable[#serializedFileTable + 1] = string.format(
            "const unsigned char file%d_contents[] = %s; int file%d_contents_len = %d;\n",
            filesSeen,
            cppString(contents),
            filesSeen,
            #contents
        )
    end

    serializedFileTable[#serializedFileTable + 1] = "void setupFiles() {\n"
    for i=1,filesSeen do
        serializedFileTable[#serializedFileTable + 1] = string.format(
            "    addFile(file%d_path,file%d_path_len,file%d_contents,file%d_contents_len);\n",
            i,i,i,i
        )
    end
    serializedFileTable[#serializedFileTable + 1] = "}\n"
    print("Serializing...")
    serializedFileTable = table.concat(serializedFileTable,"")
    print("Serialized!")

    local BOOTSTRAP_CODE
    do
        local f = io.open(os.realpath(_LUA_EXEC_DIRECTORY.."/bootstrap.lua"),"rb")
        if f then
            BOOTSTRAP_CODE = f:read("*a")
            f:close()
        else
            error("Cannot find lua-exec bootstrap!")
        end
    end

    local code = BOOTSTRAP_CODE
    if p.luaEntryPoint then
        code = code:gsub("INSERT_ENTRY_POINT_FILENAME_HERE",function() return string.format("%q",p.luaEntryPoint) end)
    end

    print("Generating header...")
    local header = ""

    header = header..serializedFileTable
    header = header.."\n"

    header = header.."static const unsigned char BOOTSTRAP_CODE[] = {\n"

    local body = {}

    for i = 1, #code do
        body[#body + 1] = string.format("%3d, ", code:byte(i))
        if (i % 32 == 0) then
            body[#body + 1] = "\n"
        end
    end

    header = header..table.concat(body,"")
    header = header.."};\n"

    header = header.."const char* grabBootstrap() {\n"
    header = header.."    return (const char*)BOOTSTRAP_CODE;\n"
    header = header.."}\n"

    if LUA_DEBUG then
        header = header.."\n\n"

        local debugBody = {}
        for line,nl in code:gmatch("([^\n\r]+)([\r\n]*)") do
            debugBody[#debugBody + 1] = "// "
            debugBody[#debugBody + 1] = line
            debugBody[#debugBody + 1] = (nl:gsub("\r\n","\n"))
        end

        header = header..table.concat(debugBody,"")
    end

    print("Header generated!")
    os.writefile_ifnotequal(header,p.headerPath)
    print(p.headerPath)
    -- print(header)
end

local luaProjects = {}

function luaProject(name,rootDir,headerPath)
    assert(name,"Lua project name required")
    assert(rootDir,"Root project dir required")
    project(name)

    kind "ConsoleApp"

    return luaProjectEx(name,rootDir,headerPath)
end

function luaProjectEx(name,rootDir,headerPath)
    assert(name,"Lua project name required")
    assert(rootDir,"Root project dir required")

    local p = project()
    luaProjects[#luaProjects + 1] = p
    p.luaProjectID = #luaProjects
    p.luaProjectIDFull = string.format("lexe_%s_%02d",name,p.luaProjectID)
    p.isLuaExecutable = true
    p.luaFiles = {}
    p.headerPath = os.realpath(headerPath)
    p.rootLuaDirectory = os.realpath(rootDir)

    newaction {
        trigger = "__premake_lua_exec_pre_"..p.luaProjectID,
        execute = function()
            return compileLuaProject(p)
        end,
        description = "internal lua packager",
    }

    filter 'files:**.lua'
        -- buildmessage 'Compiling %{file.relpath}'
        buildoutputs {
            os.realpath(p.headerPath),
        }
    filter({})

    prebuildcommands {
        "cd "..os.realpath("."),
        _PREMAKE_COMMAND.." __premake_lua_exec_pre_"..p.luaProjectID
    }
end

function luaEntryPoint(entryPoint)
    local proj = project()

    if not proj.isLuaExecutable then
        error("luaEntryPoint() not called in the scope of a lua executable project!")
    end

    proj.luaEntryPoint = entryPoint
end

function includeLua(toInclude)
    local proj = project()

    if not proj.isLuaExecutable then
        error("includeLua() not called in the scope of a lua executable project!")
    end

    if type(toInclude) == "string" then
        -- for _,filePath in ipairs(os.matchfiles(toInclude)) do
            -- proj.luaFiles[#proj.luaFiles + 1] = filePath
        -- end
        proj.luaFiles[#proj.luaFiles + 1] = toInclude

        files(toInclude)
    elseif type(toInclude) == "table" then
        for _,entry in ipairs(toInclude) do
            -- for _,filePath in ipairs(os.matchfiles(entry)) do
            --     proj.luaFiles[#proj.luaFiles + 1] = filePath
            -- end
            proj.luaFiles[#proj.luaFiles + 1] = entry
            files(entry)
        end
    else
        error("Bad param `toInclude` in `includeLua`")
    end

    -- files(toInclude)
end
