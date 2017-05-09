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
    trigger = "autoinstall",
    description = "Copies resulting binaries to garrysmod folder. Windows only."
})

newoption({
    trigger = "luahotloading",
    description = "Exposes a seriously dangerous read-any-file-on-OS function to Lua."
})

if _OPTIONS.autoinstall and os.target() ~= "windows" then
    error("Autoinstall is windows only right now thanks.")
end

local ENET_DIRECTORY = "../enet"

CreateWorkspace({name = "voxelate"})
    defines({
        "IS_DOUBLE_PRECISION_ENABLED",
        "RAD_TELEMETRY_DISABLED"
    })

    targetdir("bin")

    project("gm_sourcenet")
        kind("StaticLib")
        language("C++11")

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

    CreateProject({serverside = true})
        language("C++11")

        luaProjectEx("voxelate_bootstrap","../lua","../source/vox_lua_src.h")
            luaEntryPoint("init.lua")
            includeLua("../lua/**")

        includedirs({"../fastlz","../reactphysics3d/src","../enet/include","../gm_sourcenet"})
        links({"fastlz","reactphysics3d","enet","gm_sourcenet"})

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

        if _OPTIONS.autoinstall then
            postbuildcommands { [[copy "..\..\bin\gmsv_voxelate_win32.dll" "C:\Program Files (x86)\Steam\steamapps\common\GarrysMod\garrysmod\lua\bin\gmsv_voxelate_win32.dll"]] }
        end

        if _OPTIONS.luahotloading then
            defines({"VOXELATE_LUA_HOTLOADING"})
        end

    CreateProject({serverside = false})
        language("C++11")

        luaProjectEx("voxelate_bootstrap","../lua","../source/vox_lua_src.h")
            luaEntryPoint("init.lua")
            includeLua("../lua/**")

        includedirs({"../fastlz","../reactphysics3d/src","../enet/include","../gm_sourcenet"})
        links({"fastlz","reactphysics3d","enet","gm_sourcenet"})

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

        if _OPTIONS.autoinstall then
            postbuildcommands { [[copy "..\..\bin\gmcl_voxelate_win32.dll" "C:\Program Files (x86)\Steam\steamapps\common\GarrysMod\garrysmod\lua\bin\gmcl_voxelate_win32.dll"]] }
        end

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

    project("reactphysics3d")
        language "C++11"
        kind "StaticLib"
        symbols "On"
        warnings "Off"

        includedirs {
            "../reactphysics3d/src/",
        }

        files {
            "../reactphysics3d/src/configuration.h",
            "../reactphysics3d/src/decimal.h",
            "../reactphysics3d/src/reactphysics3d.h",
            "../reactphysics3d/src/body/Body.h",
            "../reactphysics3d/src/body/Body.cpp",
            "../reactphysics3d/src/body/CollisionBody.h",
            "../reactphysics3d/src/body/CollisionBody.cpp",
            "../reactphysics3d/src/body/RigidBody.h",
            "../reactphysics3d/src/body/RigidBody.cpp",
            "../reactphysics3d/src/collision/broadphase/BroadPhaseAlgorithm.h",
            "../reactphysics3d/src/collision/broadphase/BroadPhaseAlgorithm.cpp",
            "../reactphysics3d/src/collision/broadphase/DynamicAABBTree.h",
            "../reactphysics3d/src/collision/broadphase/DynamicAABBTree.cpp",
            "../reactphysics3d/src/collision/narrowphase/CollisionDispatch.h",
            "../reactphysics3d/src/collision/narrowphase/DefaultCollisionDispatch.h",
            "../reactphysics3d/src/collision/narrowphase/DefaultCollisionDispatch.cpp",
            "../reactphysics3d/src/collision/narrowphase/EPA/EdgeEPA.h",
            "../reactphysics3d/src/collision/narrowphase/EPA/EdgeEPA.cpp",
            "../reactphysics3d/src/collision/narrowphase/EPA/EPAAlgorithm.h",
            "../reactphysics3d/src/collision/narrowphase/EPA/EPAAlgorithm.cpp",
            "../reactphysics3d/src/collision/narrowphase/EPA/TriangleEPA.h",
            "../reactphysics3d/src/collision/narrowphase/EPA/TriangleEPA.cpp",
            "../reactphysics3d/src/collision/narrowphase/EPA/TrianglesStore.h",
            "../reactphysics3d/src/collision/narrowphase/EPA/TrianglesStore.cpp",
            "../reactphysics3d/src/collision/narrowphase/GJK/Simplex.h",
            "../reactphysics3d/src/collision/narrowphase/GJK/Simplex.cpp",
            "../reactphysics3d/src/collision/narrowphase/GJK/GJKAlgorithm.h",
            "../reactphysics3d/src/collision/narrowphase/GJK/GJKAlgorithm.cpp",
            "../reactphysics3d/src/collision/narrowphase/NarrowPhaseAlgorithm.h",
            "../reactphysics3d/src/collision/narrowphase/NarrowPhaseAlgorithm.cpp",
            "../reactphysics3d/src/collision/narrowphase/SphereVsSphereAlgorithm.h",
            "../reactphysics3d/src/collision/narrowphase/SphereVsSphereAlgorithm.cpp",
            "../reactphysics3d/src/collision/narrowphase/ConcaveVsConvexAlgorithm.h",
            "../reactphysics3d/src/collision/narrowphase/ConcaveVsConvexAlgorithm.cpp",
            "../reactphysics3d/src/collision/shapes/AABB.h",
            "../reactphysics3d/src/collision/shapes/AABB.cpp",
            "../reactphysics3d/src/collision/shapes/ConvexShape.h",
            "../reactphysics3d/src/collision/shapes/ConvexShape.cpp",
            "../reactphysics3d/src/collision/shapes/ConcaveShape.h",
            "../reactphysics3d/src/collision/shapes/ConcaveShape.cpp",
            "../reactphysics3d/src/collision/shapes/BoxShape.h",
            "../reactphysics3d/src/collision/shapes/BoxShape.cpp",
            "../reactphysics3d/src/collision/shapes/CapsuleShape.h",
            "../reactphysics3d/src/collision/shapes/CapsuleShape.cpp",
            "../reactphysics3d/src/collision/shapes/CollisionShape.h",
            "../reactphysics3d/src/collision/shapes/CollisionShape.cpp",
            "../reactphysics3d/src/collision/shapes/ConeShape.h",
            "../reactphysics3d/src/collision/shapes/ConeShape.cpp",
            "../reactphysics3d/src/collision/shapes/ConvexMeshShape.h",
            "../reactphysics3d/src/collision/shapes/ConvexMeshShape.cpp",
            "../reactphysics3d/src/collision/shapes/CylinderShape.h",
            "../reactphysics3d/src/collision/shapes/CylinderShape.cpp",
            "../reactphysics3d/src/collision/shapes/SphereShape.h",
            "../reactphysics3d/src/collision/shapes/SphereShape.cpp",
            "../reactphysics3d/src/collision/shapes/TriangleShape.h",
            "../reactphysics3d/src/collision/shapes/TriangleShape.cpp",
            "../reactphysics3d/src/collision/shapes/ConcaveMeshShape.h",
            "../reactphysics3d/src/collision/shapes/ConcaveMeshShape.cpp",
            "../reactphysics3d/src/collision/shapes/HeightFieldShape.h",
            "../reactphysics3d/src/collision/shapes/HeightFieldShape.cpp",
            "../reactphysics3d/src/collision/RaycastInfo.h",
            "../reactphysics3d/src/collision/RaycastInfo.cpp",
            "../reactphysics3d/src/collision/ProxyShape.h",
            "../reactphysics3d/src/collision/ProxyShape.cpp",
            "../reactphysics3d/src/collision/TriangleVertexArray.h",
            "../reactphysics3d/src/collision/TriangleVertexArray.cpp",
            "../reactphysics3d/src/collision/TriangleMesh.h",
            "../reactphysics3d/src/collision/TriangleMesh.cpp",
            "../reactphysics3d/src/collision/CollisionDetection.h",
            "../reactphysics3d/src/collision/CollisionDetection.cpp",
            "../reactphysics3d/src/collision/CollisionShapeInfo.h",
            "../reactphysics3d/src/collision/ContactManifold.h",
            "../reactphysics3d/src/collision/ContactManifold.cpp",
            "../reactphysics3d/src/collision/ContactManifoldSet.h",
            "../reactphysics3d/src/collision/ContactManifoldSet.cpp",
            "../reactphysics3d/src/constraint/BallAndSocketJoint.h",
            "../reactphysics3d/src/constraint/BallAndSocketJoint.cpp",
            "../reactphysics3d/src/constraint/ContactPoint.h",
            "../reactphysics3d/src/constraint/ContactPoint.cpp",
            "../reactphysics3d/src/constraint/FixedJoint.h",
            "../reactphysics3d/src/constraint/FixedJoint.cpp",
            "../reactphysics3d/src/constraint/HingeJoint.h",
            "../reactphysics3d/src/constraint/HingeJoint.cpp",
            "../reactphysics3d/src/constraint/Joint.h",
            "../reactphysics3d/src/constraint/Joint.cpp",
            "../reactphysics3d/src/constraint/SliderJoint.h",
            "../reactphysics3d/src/constraint/SliderJoint.cpp",
            "../reactphysics3d/src/engine/CollisionWorld.h",
            "../reactphysics3d/src/engine/CollisionWorld.cpp",
            "../reactphysics3d/src/engine/ConstraintSolver.h",
            "../reactphysics3d/src/engine/ConstraintSolver.cpp",
            "../reactphysics3d/src/engine/ContactSolver.h",
            "../reactphysics3d/src/engine/ContactSolver.cpp",
            "../reactphysics3d/src/engine/DynamicsWorld.h",
            "../reactphysics3d/src/engine/DynamicsWorld.cpp",
            "../reactphysics3d/src/engine/EventListener.h",
            "../reactphysics3d/src/engine/Impulse.h",
            "../reactphysics3d/src/engine/Island.h",
            "../reactphysics3d/src/engine/Island.cpp",
            "../reactphysics3d/src/engine/Material.h",
            "../reactphysics3d/src/engine/Material.cpp",
            "../reactphysics3d/src/engine/OverlappingPair.h",
            "../reactphysics3d/src/engine/OverlappingPair.cpp",
            "../reactphysics3d/src/engine/Profiler.h",
            "../reactphysics3d/src/engine/Profiler.cpp",
            "../reactphysics3d/src/engine/Timer.h",
            "../reactphysics3d/src/engine/Timer.cpp",
            "../reactphysics3d/src/mathematics/mathematics.h",
            "../reactphysics3d/src/mathematics/mathematics_functions.h",
            "../reactphysics3d/src/mathematics/mathematics_functions.cpp",
            "../reactphysics3d/src/mathematics/Matrix2x2.h",
            "../reactphysics3d/src/mathematics/Matrix2x2.cpp",
            "../reactphysics3d/src/mathematics/Matrix3x3.h",
            "../reactphysics3d/src/mathematics/Matrix3x3.cpp",
            "../reactphysics3d/src/mathematics/Quaternion.h",
            "../reactphysics3d/src/mathematics/Quaternion.cpp",
            "../reactphysics3d/src/mathematics/Transform.h",
            "../reactphysics3d/src/mathematics/Transform.cpp",
            "../reactphysics3d/src/mathematics/Vector2.h",
            "../reactphysics3d/src/mathematics/Vector2.cpp",
            "../reactphysics3d/src/mathematics/Vector3.h",
            "../reactphysics3d/src/mathematics/Ray.h",
            "../reactphysics3d/src/mathematics/Vector3.cpp",
            "../reactphysics3d/src/memory/MemoryAllocator.h",
            "../reactphysics3d/src/memory/MemoryAllocator.cpp",
            "../reactphysics3d/src/memory/Stack.h"
        }

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
