newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
if gmcommon == nil then
	error("you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
end

include(gmcommon)

include("luabundling.lua")

newoption({
	trigger = "luahotloading",
	description = "Exposes a seriously dangerous read-any-file-on-OS function to Lua."
})

local ENET_DIRECTORY = "../enet"

CreateWorkspace({name = "voxelate"})
	configurations { "Debug", "Release" }
	defines({
		"BT_USE_DOUBLE_PRECISION",
		"RAD_TELEMETRY_DISABLED"
	})

	targetdir("bin")

	project("gm_sourcenet")
		kind("StaticLib")
		language("C++")
		cppdialect("C++11")

		warnings("Off")

		files({
			"../gm_sourcenet/*.h",
			"../gm_sourcenet/*.hpp",
			"../gm_sourcenet/*.hxx",
			"../gm_sourcenet/*.c",
			"../gm_sourcenet/*.cpp",
			"../gm_sourcenet/*.cxx",
		})
		includedirs({
			"../gm_sourcenet",
		})

		vpaths({
			["Header files/*"] = {
				"../gm_sourcenet/*.h",
				"../gm_sourcenet/*.hpp",
				"../gm_sourcenet/*.hxx",
			},
			["Source files/*"] = {
				"../gm_sourcenet/*.c",
				"../gm_sourcenet/*.cpp",
				"../gm_sourcenet/*.cxx",
			}
		})

		IncludeLuaShared()
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeScanning()
		IncludeDetouring()

	project("gm_adv_vectors")
		kind("StaticLib")
		language("C++")
		cppdialect("C++11")


		includedirs({"../bullet3-2.87/src"})
		links({"BulletDynamics","BulletCollision","LinearMath"})

		files({
			"../gm_adv_vectors/*.h",
			"../gm_adv_vectors/*.hpp",
			"../gm_adv_vectors/*.hxx",
			"../gm_adv_vectors/*.c",
			"../gm_adv_vectors/*.cpp",
			"../gm_adv_vectors/*.cxx",
		})
		includedirs({
			"../gm_adv_vectors",
		})

		vpaths({
			["Header files/*"] = {
				"../gm_adv_vectors/*.h",
				"../gm_adv_vectors/*.hpp",
				"../gm_adv_vectors/*.hxx",
			},
			["Source files/*"] = {
				"../gm_adv_vectors/*.c",
				"../gm_adv_vectors/*.cpp",
				"../gm_adv_vectors/*.cxx",
			}
		})

		IncludeLuaShared()
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()

	CreateProject({serverside = true})
		language("C++")
		cppdialect("C++11")
		warnings("Extra")
		symbols "On"
		editandcontinue "On"

		luaProjectEx("voxelate_bootstrap","../lua","../source/vox_lua_src.h")
			luaEntryPoint("init.lua")
			includeLua("../lua/**")

		includedirs({"../fastlz","../bullet3-2.87/src","../enet/include","../enetpp/include","../gm_sourcenet","../gm_adv_vectors"})
		links({"fastlz","BulletDynamics","BulletCollision","LinearMath","enet","gm_sourcenet","gm_adv_vectors"})

		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSDKTier2()
		IncludeSDKMathlib()
		-- IncludeSDKRaytrace()
		IncludeSteamAPI()
		IncludeDetouring()
		IncludeScanning()
		IncludeLuaShared()

		if _OPTIONS.luahotloading then
			defines({"VOXELATE_LUA_HOTLOADING"})
		end

	CreateProject({serverside = false})
		language("C++")
		cppdialect("C++11")
		warnings("Extra")
		symbols "On"
		editandcontinue "On"
		
		luaProjectEx("voxelate_bootstrap","../lua","../source/vox_lua_src.h")
			luaEntryPoint("init.lua")
			includeLua("../lua/**")

		links({"shaderlib"})
		libdirs{ "../shaderlib/"..os.target() }
		includedirs({"../fastlz","../bullet3-2.87/src","../enet/include","../enetpp/include","../gm_sourcenet","../gm_adv_vectors"})
		links({"fastlz","BulletDynamics","BulletCollision","LinearMath","enet","gm_sourcenet","gm_adv_vectors"})

		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSDKTier2()
		IncludeSDKMathlib()
		-- IncludeSDKRaytrace()
		IncludeSteamAPI()
		IncludeDetouring()
		IncludeScanning()
		IncludeLuaShared()

		if _OPTIONS.luahotloading then
			defines({"VOXELATE_LUA_HOTLOADING"})
		end

	project("fastlz")
		language("C")
		kind("StaticLib")
		warnings "Off"
		files({
			"../fastlz/fastlz.c",
			"../fastlz/fastlz.h",
		})

	project("enet") -- from https://github.com/danielga/gm_enet
		kind("StaticLib")
		warnings "Off"
		includedirs(ENET_DIRECTORY .. "/include")
		vpaths({
			["Header files"] = ENET_DIRECTORY .. "/**.h",
			["Source files"] = ENET_DIRECTORY .. "/**.c"
		})
		files({
			ENET_DIRECTORY .. "/callbacks.c",
			ENET_DIRECTORY .. "/compress.c",
			ENET_DIRECTORY .. "/host.c",
			ENET_DIRECTORY .. "/list.c",
			ENET_DIRECTORY .. "/packet.c",
			ENET_DIRECTORY .. "/peer.c",
			ENET_DIRECTORY .. "/protocol.c"
		})

		filter("system:windows")
			files(ENET_DIRECTORY .. "/win32.c")
			links({"ws2_32", "winmm"})

		filter("system:linux or macosx")
			defines({
				"HAS_GETADDRINFO",
				"HAS_GETNAMEINFO",
				"HAS_GETHOSTBYADDR_R",
				"HAS_GETHOSTBYNAME_R",
				"HAS_POLL",
				"HAS_FCNTL",
				"HAS_INET_PTON",
				"HAS_INET_NTOP",
				"HAS_MSGHDR_FLAGS",
				"HAS_SOCKLEN_T"
			})
			files(ENET_DIRECTORY .. "/unix.c")

	local oProj = project
	function project(...)
		local p = oProj(...)
		
		optimize "On"
		vectorextensions "SSE2"
		floatingpoint "Fast"
		flags { "StaticRuntime", "NoMinimalRebuild"}

		warnings("Off")

		return p
	end
	
		projectRootDir = os.getcwd().."/../bullet3-2.87/"
		dofile "../bullet3-2.87/build3/findOpenCL.lua"

		include "../bullet3-2.87/src/Bullet3Common"
		include "../bullet3-2.87/src/Bullet3Geometry"
		include "../bullet3-2.87/src/Bullet3Collision"
		include "../bullet3-2.87/src/Bullet3Dynamics"
		include "../bullet3-2.87/src/Bullet3OpenCL"
		include "../bullet3-2.87/src/Bullet3Serialize/Bullet2FileLoader"

		include "../bullet3-2.87/src/BulletInverseDynamics"
		include "../bullet3-2.87/src/BulletSoftBody"
		include "../bullet3-2.87/src/BulletDynamics"
		include "../bullet3-2.87/src/BulletCollision"
		include "../bullet3-2.87/src/LinearMath"

	project = oProj
