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

        includedirs {
            "../reactphysics3d/src",
        }

        files {
            "../reactphysics3d/srcconfiguration.h",
            "../reactphysics3d/srcdecimal.h",
            "../reactphysics3d/srcreactphysics3d.h",
            "../reactphysics3d/srcbody/Body.h",
            "../reactphysics3d/srcbody/Body.cpp",
            "../reactphysics3d/srcbody/CollisionBody.h",
            "../reactphysics3d/srcbody/CollisionBody.cpp",
            "../reactphysics3d/srcbody/RigidBody.h",
            "../reactphysics3d/srcbody/RigidBody.cpp",
            "../reactphysics3d/srccollision/broadphase/BroadPhaseAlgorithm.h",
            "../reactphysics3d/srccollision/broadphase/BroadPhaseAlgorithm.cpp",
            "../reactphysics3d/srccollision/broadphase/DynamicAABBTree.h",
            "../reactphysics3d/srccollision/broadphase/DynamicAABBTree.cpp",
            "../reactphysics3d/srccollision/narrowphase/CollisionDispatch.h",
            "../reactphysics3d/srccollision/narrowphase/DefaultCollisionDispatch.h",
            "../reactphysics3d/srccollision/narrowphase/DefaultCollisionDispatch.cpp",
            "../reactphysics3d/srccollision/narrowphase/EPA/EdgeEPA.h",
            "../reactphysics3d/srccollision/narrowphase/EPA/EdgeEPA.cpp",
            "../reactphysics3d/srccollision/narrowphase/EPA/EPAAlgorithm.h",
            "../reactphysics3d/srccollision/narrowphase/EPA/EPAAlgorithm.cpp",
            "../reactphysics3d/srccollision/narrowphase/EPA/TriangleEPA.h",
            "../reactphysics3d/srccollision/narrowphase/EPA/TriangleEPA.cpp",
            "../reactphysics3d/srccollision/narrowphase/EPA/TrianglesStore.h",
            "../reactphysics3d/srccollision/narrowphase/EPA/TrianglesStore.cpp",
            "../reactphysics3d/srccollision/narrowphase/GJK/Simplex.h",
            "../reactphysics3d/srccollision/narrowphase/GJK/Simplex.cpp",
            "../reactphysics3d/srccollision/narrowphase/GJK/GJKAlgorithm.h",
            "../reactphysics3d/srccollision/narrowphase/GJK/GJKAlgorithm.cpp",
            "../reactphysics3d/srccollision/narrowphase/NarrowPhaseAlgorithm.h",
            "../reactphysics3d/srccollision/narrowphase/NarrowPhaseAlgorithm.cpp",
            "../reactphysics3d/srccollision/narrowphase/SphereVsSphereAlgorithm.h",
            "../reactphysics3d/srccollision/narrowphase/SphereVsSphereAlgorithm.cpp",
            "../reactphysics3d/srccollision/narrowphase/ConcaveVsConvexAlgorithm.h",
            "../reactphysics3d/srccollision/narrowphase/ConcaveVsConvexAlgorithm.cpp",
            "../reactphysics3d/srccollision/shapes/AABB.h",
            "../reactphysics3d/srccollision/shapes/AABB.cpp",
            "../reactphysics3d/srccollision/shapes/ConvexShape.h",
            "../reactphysics3d/srccollision/shapes/ConvexShape.cpp",
            "../reactphysics3d/srccollision/shapes/ConcaveShape.h",
            "../reactphysics3d/srccollision/shapes/ConcaveShape.cpp",
            "../reactphysics3d/srccollision/shapes/BoxShape.h",
            "../reactphysics3d/srccollision/shapes/BoxShape.cpp",
            "../reactphysics3d/srccollision/shapes/CapsuleShape.h",
            "../reactphysics3d/srccollision/shapes/CapsuleShape.cpp",
            "../reactphysics3d/srccollision/shapes/CollisionShape.h",
            "../reactphysics3d/srccollision/shapes/CollisionShape.cpp",
            "../reactphysics3d/srccollision/shapes/ConeShape.h",
            "../reactphysics3d/srccollision/shapes/ConeShape.cpp",
            "../reactphysics3d/srccollision/shapes/ConvexMeshShape.h",
            "../reactphysics3d/srccollision/shapes/ConvexMeshShape.cpp",
            "../reactphysics3d/srccollision/shapes/CylinderShape.h",
            "../reactphysics3d/srccollision/shapes/CylinderShape.cpp",
            "../reactphysics3d/srccollision/shapes/SphereShape.h",
            "../reactphysics3d/srccollision/shapes/SphereShape.cpp",
            "../reactphysics3d/srccollision/shapes/TriangleShape.h",
            "../reactphysics3d/srccollision/shapes/TriangleShape.cpp",
            "../reactphysics3d/srccollision/shapes/ConcaveMeshShape.h",
            "../reactphysics3d/srccollision/shapes/ConcaveMeshShape.cpp",
            "../reactphysics3d/srccollision/shapes/HeightFieldShape.h",
            "../reactphysics3d/srccollision/shapes/HeightFieldShape.cpp",
            "../reactphysics3d/srccollision/RaycastInfo.h",
            "../reactphysics3d/srccollision/RaycastInfo.cpp",
            "../reactphysics3d/srccollision/ProxyShape.h",
            "../reactphysics3d/srccollision/ProxyShape.cpp",
            "../reactphysics3d/srccollision/TriangleVertexArray.h",
            "../reactphysics3d/srccollision/TriangleVertexArray.cpp",
            "../reactphysics3d/srccollision/TriangleMesh.h",
            "../reactphysics3d/srccollision/TriangleMesh.cpp",
            "../reactphysics3d/srccollision/CollisionDetection.h",
            "../reactphysics3d/srccollision/CollisionDetection.cpp",
            "../reactphysics3d/srccollision/CollisionShapeInfo.h",
            "../reactphysics3d/srccollision/ContactManifold.h",
            "../reactphysics3d/srccollision/ContactManifold.cpp",
            "../reactphysics3d/srccollision/ContactManifoldSet.h",
            "../reactphysics3d/srccollision/ContactManifoldSet.cpp",
            "../reactphysics3d/srcconstraint/BallAndSocketJoint.h",
            "../reactphysics3d/srcconstraint/BallAndSocketJoint.cpp",
            "../reactphysics3d/srcconstraint/ContactPoint.h",
            "../reactphysics3d/srcconstraint/ContactPoint.cpp",
            "../reactphysics3d/srcconstraint/FixedJoint.h",
            "../reactphysics3d/srcconstraint/FixedJoint.cpp",
            "../reactphysics3d/srcconstraint/HingeJoint.h",
            "../reactphysics3d/srcconstraint/HingeJoint.cpp",
            "../reactphysics3d/srcconstraint/Joint.h",
            "../reactphysics3d/srcconstraint/Joint.cpp",
            "../reactphysics3d/srcconstraint/SliderJoint.h",
            "../reactphysics3d/srcconstraint/SliderJoint.cpp",
            "../reactphysics3d/srcengine/CollisionWorld.h",
            "../reactphysics3d/srcengine/CollisionWorld.cpp",
            "../reactphysics3d/srcengine/ConstraintSolver.h",
            "../reactphysics3d/srcengine/ConstraintSolver.cpp",
            "../reactphysics3d/srcengine/ContactSolver.h",
            "../reactphysics3d/srcengine/ContactSolver.cpp",
            "../reactphysics3d/srcengine/DynamicsWorld.h",
            "../reactphysics3d/srcengine/DynamicsWorld.cpp",
            "../reactphysics3d/srcengine/EventListener.h",
            "../reactphysics3d/srcengine/Impulse.h",
            "../reactphysics3d/srcengine/Island.h",
            "../reactphysics3d/srcengine/Island.cpp",
            "../reactphysics3d/srcengine/Material.h",
            "../reactphysics3d/srcengine/Material.cpp",
            "../reactphysics3d/srcengine/OverlappingPair.h",
            "../reactphysics3d/srcengine/OverlappingPair.cpp",
            "../reactphysics3d/srcengine/Profiler.h",
            "../reactphysics3d/srcengine/Profiler.cpp",
            "../reactphysics3d/srcengine/Timer.h",
            "../reactphysics3d/srcengine/Timer.cpp",
            "../reactphysics3d/srcmathematics/mathematics.h",
            "../reactphysics3d/srcmathematics/mathematics_functions.h",
            "../reactphysics3d/srcmathematics/mathematics_functions.cpp",
            "../reactphysics3d/srcmathematics/Matrix2x2.h",
            "../reactphysics3d/srcmathematics/Matrix2x2.cpp",
            "../reactphysics3d/srcmathematics/Matrix3x3.h",
            "../reactphysics3d/srcmathematics/Matrix3x3.cpp",
            "../reactphysics3d/srcmathematics/Quaternion.h",
            "../reactphysics3d/srcmathematics/Quaternion.cpp",
            "../reactphysics3d/srcmathematics/Transform.h",
            "../reactphysics3d/srcmathematics/Transform.cpp",
            "../reactphysics3d/srcmathematics/Vector2.h",
            "../reactphysics3d/srcmathematics/Vector2.cpp",
            "../reactphysics3d/srcmathematics/Vector3.h",
            "../reactphysics3d/srcmathematics/Ray.h",
            "../reactphysics3d/srcmathematics/Vector3.cpp",
            "../reactphysics3d/srcmemory/MemoryAllocator.h",
            "../reactphysics3d/srcmemory/MemoryAllocator.cpp",
            "../reactphysics3d/srcmemory/Stack.h"
        }
