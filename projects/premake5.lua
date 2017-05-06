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

CreateWorkspace({name = "voxelate"})
	defines({
		"RAD_TELEMETRY_DISABLED"
	})

	CreateProject({serverside = true})
		language("C++11")

		includedirs("../fastlz")
		links({"fastlz"})

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

	CreateProject({serverside = false})
		language("C++11")

		includedirs("../fastlz")
		links({"fastlz"})

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

	project("fastlz")
		language("C")
		kind("StaticLib")
		files({
			"../fastlz/fastlz.c",
			"../fastlz/fastlz.h",
		})
