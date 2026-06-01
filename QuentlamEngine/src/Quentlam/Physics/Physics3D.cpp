#include "qlpch.h"
#include "Physics3D.h"
#include "Physics3DValidation.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/Scene/Entity.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

JPH_SUPPRESS_WARNINGS

namespace Quentlam {

	using namespace JPH;

	struct PhysicsData;
	static PhysicsData* s_Data = nullptr;
	static Scene* s_RuntimeScene = nullptr;

	namespace
	{
		void ResetRuntimeBodyIds(Scene* scene)
		{
			auto view = scene->GetRegistry().view<Rigidbody3DComponent>();
			for (auto e : view)
				view.get<Rigidbody3DComponent>(e).RuntimeBodyID = 0xFFFFFFFF;
		}
	}

	namespace Layers {
		static constexpr ObjectLayer NON_MOVING = 0;
		static constexpr ObjectLayer MOVING = 1;
		static constexpr ObjectLayer NUM_LAYERS = 2;
	};

	class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl()
		{
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayer(0);
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayer(1);
		}

		virtual uint GetNumBroadPhaseLayers() const override { return 2; }
		virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
		{
			switch ((BroadPhaseLayer::Type)inLayer)
			{
			case (BroadPhaseLayer::Type)0: return "NON_MOVING";
			case (BroadPhaseLayer::Type)1: return "MOVING";
			default: JPH_ASSERT(false); return "INVALID";
			}
		}
#endif

	private:
		BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayer(1);
			case Layers::MOVING: return true;
			default: JPH_ASSERT(false); return false;
			}
		}
	};

	class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
			case Layers::MOVING: return true;
			default: JPH_ASSERT(false); return false;
			}
		}
	};

	static void TraceImpl(const char* inFMT, ...)
	{
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);
		// Log
	}

#ifdef JPH_ENABLE_ASSERTS
	static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
	{
		QL_CORE_ERROR("Jolt Physics Assert Failed: {0} : {1} at {2}:{3}", inExpression, inMessage ? inMessage : "", inFile, inLine);
		return false;
	};
#endif

	struct PhysicsData
	{
		PhysicsSystem* physicsSystem;
		JobSystemThreadPool* jobSystem;
		TempAllocatorImpl* tempAllocator;
		BPLayerInterfaceImpl broadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImpl objectVsObjectLayerFilter;
	};

	namespace
	{
		void DestroyPhysicsData()
		{
			if (!s_Data)
				return;

			delete s_Data->physicsSystem;
			delete s_Data->jobSystem;
			delete s_Data->tempAllocator;
			delete s_Data;
			s_Data = nullptr;
		}
	}

	void Physics3D::Init()
	{
		RegisterDefaultAllocator();

		Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

		Factory::sInstance = new Factory();
		RegisterTypes();
	}

	void Physics3D::Shutdown()
	{
		if (s_Data)
			OnRuntimeStop(s_RuntimeScene);

		if (!Factory::sInstance)
			return;

		UnregisterTypes();
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}

	bool Physics3D::OnRuntimeStart(Scene* scene)
	{
		if (!scene)
		{
			QL_CORE_ERROR("Physics3D runtime start failed: scene is null.");
			return false;
		}

		if (!Factory::sInstance)
			Init();

		if (s_Data && s_RuntimeScene == scene)
		{
			return true;
		}

		if (s_Data)
			OnRuntimeStop(s_RuntimeScene ? s_RuntimeScene : scene);

		ResetRuntimeBodyIds(scene);

		s_Data = new PhysicsData();
		s_RuntimeScene = scene;
		s_Data->tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
		uint32_t hwConcurrency = thread::hardware_concurrency();
		uint32_t workerThreads = hwConcurrency > 1 ? hwConcurrency - 1 : 1;
		QL_CORE_WARN("Physics3D: hardware_concurrency={0}, using {1} worker threads", hwConcurrency, workerThreads);
		s_Data->jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, workerThreads);
		s_Data->physicsSystem = new PhysicsSystem();
		QL_CORE_WARN("Physics3D: physics system created, initializing broad phase...");

		const uint cMaxBodies = 1024;
		const uint cNumBodyMutexes = 0;
		const uint cMaxBodyPairs = 1024;
		const uint cMaxContactConstraints = 1024;

		s_Data->physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
			s_Data->broadPhaseLayerInterface, s_Data->objectVsBroadPhaseLayerFilter, s_Data->objectVsObjectLayerFilter);
		QL_CORE_WARN("Physics3D: physics system initialized, creating bodies...");

		BodyInterface& bodyInterface = s_Data->physicsSystem->GetBodyInterface();

		auto view = scene->GetRegistry().view<Rigidbody3DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, scene };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb3d = entity.GetComponent<Rigidbody3DComponent>();

			if (entity.HasComponent<BoxCollider3DComponent>())
			{
				auto& bc3d = entity.GetComponent<BoxCollider3DComponent>();

				glm::vec3 scale(
					glm::length(glm::vec3(transform.Transform[0])),
					glm::length(glm::vec3(transform.Transform[1])),
					glm::length(glm::vec3(transform.Transform[2]))
				);

				glm::vec3 extents = Physics3DValidation::SanitizeHalfExtent(bc3d.HalfExtent, scale);
				float convexRadius = Physics3DValidation::CalculateConvexRadius(extents);

				BoxShapeSettings boxShapeSettings(Vec3(extents.x, extents.y, extents.z), convexRadius);
				ShapeSettings::ShapeResult boxShapeResult = boxShapeSettings.Create();
				if (boxShapeResult.HasError())
				{
					QL_CORE_ERROR("Physics3D runtime start failed for entity '{0}': invalid box collider ({1}).",
						entity.GetComponent<TagComponent>().Tag, boxShapeResult.GetError().c_str());
					OnRuntimeStop(scene);
					return false;
				}
				ShapeRefC boxShape = boxShapeResult.Get();
				if (boxShape == nullptr)
				{
					QL_CORE_ERROR("Physics3D runtime start failed for entity '{0}': shape creation returned null.",
						entity.GetComponent<TagComponent>().Tag);
					OnRuntimeStop(scene);
					return false;
				}

				glm::vec3 position = transform.Transform[3];
				if (!Physics3DValidation::IsFiniteVec3(position))
				{
					QL_CORE_ERROR("Physics3D runtime start failed for entity '{0}': transform position is invalid.",
						entity.GetComponent<TagComponent>().Tag);
					OnRuntimeStop(scene);
					return false;
				}
				
				// Basic rotation extraction
				// ... (Ideally extract quaternion from transform.Transform)
				// Simplified to Identity for now
				Quat rotation = Quat::sIdentity();

				EMotionType motionType = rb3d.Type == Rigidbody3DComponent::BodyType::Static ? EMotionType::Static :
					(rb3d.Type == Rigidbody3DComponent::BodyType::Dynamic ? EMotionType::Dynamic : EMotionType::Kinematic);

				ObjectLayer layer = motionType == EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING;

				BodyCreationSettings bodySettings(boxShape, RVec3(position.x, position.y, position.z), rotation, motionType, layer);
				
				if (rb3d.Type == Rigidbody3DComponent::BodyType::Dynamic)
				{
					float safeMass = Physics3DValidation::SanitizeMass(rb3d.Mass);
					if (safeMass != rb3d.Mass)
					{
						QL_CORE_WARN("Physics3D mass adjusted for entity '{0}': {1} -> {2}.",
							entity.GetComponent<TagComponent>().Tag, rb3d.Mass, safeMass);
						rb3d.Mass = safeMass;
					}
					bodySettings.mMassPropertiesOverride.mMass = safeMass;
					bodySettings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
				}

				Body* body = bodyInterface.CreateBody(bodySettings);
				if (!body)
				{
					QL_CORE_ERROR("Physics3D runtime start failed for entity '{0}': body creation returned null.",
						entity.GetComponent<TagComponent>().Tag);
					OnRuntimeStop(scene);
					return false;
				}
				bodyInterface.AddBody(body->GetID(), motionType == EMotionType::Static ? EActivation::DontActivate : EActivation::Activate);
				
				rb3d.RuntimeBodyID = body->GetID().GetIndexAndSequenceNumber();
			}
		}

		s_Data->physicsSystem->OptimizeBroadPhase();
		return true;
	}

	void Physics3D::OnRuntimeStop(Scene* scene)
	{
		Scene* sceneToReset = scene ? scene : s_RuntimeScene;
		if (!s_Data)
		{
			ResetRuntimeBodyIds(sceneToReset);
			s_RuntimeScene = nullptr;
			return;
		}

		if (!sceneToReset)
		{
			DestroyPhysicsData();
			s_RuntimeScene = nullptr;
			return;
		}

		BodyInterface& bodyInterface = s_Data->physicsSystem->GetBodyInterface();
		auto view = sceneToReset->GetRegistry().view<Rigidbody3DComponent>();
		for (auto e : view)
		{
			auto& rb3d = view.get<Rigidbody3DComponent>(e);
			if (rb3d.RuntimeBodyID != 0xFFFFFFFF)
			{
				BodyID bodyID(rb3d.RuntimeBodyID);
				bodyInterface.RemoveBody(bodyID);
				bodyInterface.DestroyBody(bodyID);
				rb3d.RuntimeBodyID = 0xFFFFFFFF;
			}
		}

		DestroyPhysicsData();
		ResetRuntimeBodyIds(sceneToReset);
		s_RuntimeScene = nullptr;
	}

	void Physics3D::OnUpdate(Scene* scene, Timestep ts)
	{
		if (!s_Data)
			return;

		s_Data->physicsSystem->Update(ts, 1, s_Data->tempAllocator, s_Data->jobSystem);

		BodyInterface& bodyInterface = s_Data->physicsSystem->GetBodyInterface();

		auto view = scene->GetRegistry().view<Rigidbody3DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, scene };
			auto& rb3d = entity.GetComponent<Rigidbody3DComponent>();

			if (rb3d.Type != Rigidbody3DComponent::BodyType::Static && rb3d.RuntimeBodyID != 0xFFFFFFFF)
			{
				BodyID bodyID(rb3d.RuntimeBodyID);
				RVec3 position = bodyInterface.GetPosition(bodyID);
				Quat rotation = bodyInterface.GetRotation(bodyID);

				auto& transform = entity.GetComponent<TransformComponent>();
				
				// Keep scale
				glm::vec3 scale(
					glm::length(glm::vec3(transform.Transform[0])),
					glm::length(glm::vec3(transform.Transform[1])),
					glm::length(glm::vec3(transform.Transform[2]))
				);

				// Update transform (Rotation & Translation)
				// Reconstruct matrix from position and rotation (simplified)
				
				glm::quat q(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
				glm::mat4 rotMat = glm::mat4_cast(q);

				transform.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(position.GetX(), position.GetY(), position.GetZ()))
					* rotMat
					* glm::scale(glm::mat4(1.0f), scale);
			}
		}
	}
}
