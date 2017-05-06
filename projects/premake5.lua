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

	targetdir("bin")

	CreateProject({serverside = true})
		language("C++11")

		includedirs("../fastlz")
		links({"fastlz","reactphysics3d"})

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
		links({"fastlz","reactphysics3d"})

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
		warnings "Off"
		files({
			"../fastlz/fastlz.c",
			"../fastlz/fastlz.h",
		})

    project("reactphysics3d")
        language "C++"
        kind "StaticLib"
		warnings "Off"

		defines({
			"IS_DOUBLE_PRECISION_ENABLED",
		})

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
