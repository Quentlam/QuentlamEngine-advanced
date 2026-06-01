#include "qlpch.h"
#include "Physics2D.h"
#include "Quentlam/Scene/Components.h"
#include "Quentlam/Scene/Entity.h"
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_collision.h>
#include <box2d/b2_shape.h>

namespace Quentlam {

static b2World* s_PhysicsWorld = nullptr;
static Scene* s_RuntimeScene = nullptr;

namespace
{
	void ResetRuntimeHandles(Scene* scene)
	{
		if (!scene)
			return;

		auto rigidbodyView = scene->GetRegistry().view<Rigidbody2DComponent>();
		for (auto e : rigidbodyView)
			rigidbodyView.get<Rigidbody2DComponent>(e).RuntimeBody = nullptr;

		auto colliderView = scene->GetRegistry().view<BoxCollider2DComponent>();
		for (auto e : colliderView)
			colliderView.get<BoxCollider2DComponent>(e).RuntimeFixture = nullptr;
	}

	glm::vec3 ExtractScale(const glm::mat4& transform)
	{
		return {
			glm::length(glm::vec3(transform[0])),
			glm::length(glm::vec3(transform[1])),
			glm::length(glm::vec3(transform[2]))
		};
	}
}

bool Physics2D::OnRuntimeStart(Scene* scene)
{
	if (!scene)
	{
		QL_CORE_ERROR("Physics2D runtime start failed: scene is null.");
		return false;
	}

	if (s_PhysicsWorld && s_RuntimeScene == scene)
		return true;

	if (s_PhysicsWorld)
		OnRuntimeStop(s_RuntimeScene ? s_RuntimeScene : scene);

	ResetRuntimeHandles(scene);

	s_PhysicsWorld = new b2World({ 0.0f, -9.8f });
	s_RuntimeScene = scene;

	auto view = scene->GetRegistry().view<Rigidbody2DComponent>();
	for (auto e : view)
	{
		Entity entity = { e, scene };
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = rb2d.Type == Rigidbody2DComponent::BodyType::Static ? b2_staticBody :
			(rb2d.Type == Rigidbody2DComponent::BodyType::Dynamic ? b2_dynamicBody : b2_kinematicBody);

		glm::vec3 position = transform.Transform[3];
		bodyDef.position.Set(position.x, position.y);
		bodyDef.angle = atan2(transform.Transform[0][1], transform.Transform[0][0]);
		bodyDef.fixedRotation = rb2d.FixedRotation;
		bodyDef.gravityScale = rb2d.GravityScale;
		bodyDef.userData.pointer = (uint64_t)e;

		b2Body* body = s_PhysicsWorld->CreateBody(&bodyDef);
		rb2d.RuntimeBody = body;

		glm::vec3 scale = ExtractScale(transform.Transform);

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
			b2PolygonShape boxShape;
			boxShape.SetAsBox(
				bc2d.Size.x * scale.x * 0.5f,
				bc2d.Size.y * scale.y * 0.5f,
				b2Vec2(bc2d.Offset.x, bc2d.Offset.y),
				0.0f);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &boxShape;
			fixtureDef.density = bc2d.Density;
			fixtureDef.friction = bc2d.Friction;
			fixtureDef.restitution = bc2d.Restitution;
			fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
			fixtureDef.filter.categoryBits = rb2d.CollisionLayer;
			fixtureDef.filter.maskBits = rb2d.CollisionMask;
			bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
			b2CircleShape circleShape;
			float avgScale = (scale.x + scale.y) * 0.5f;
			circleShape.m_radius = cc2d.Radius * avgScale;
			circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &circleShape;
			fixtureDef.density = cc2d.Density;
			fixtureDef.friction = cc2d.Friction;
			fixtureDef.restitution = cc2d.Restitution;
			fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
			fixtureDef.filter.categoryBits = rb2d.CollisionLayer;
			fixtureDef.filter.maskBits = rb2d.CollisionMask;
			cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}

		if (entity.HasComponent<TriangleCollider2DComponent>())
		{
			auto& tc2d = entity.GetComponent<TriangleCollider2DComponent>();
			b2PolygonShape triangleShape;
			b2Vec2 vertices[3];
			vertices[0].Set(tc2d.Offset.x - tc2d.Size.x * scale.x * 0.5f, tc2d.Offset.y - tc2d.Size.y * scale.y * 0.5f);
			vertices[1].Set(tc2d.Offset.x + tc2d.Size.x * scale.x * 0.5f, tc2d.Offset.y - tc2d.Size.y * scale.y * 0.5f);
			vertices[2].Set(tc2d.Offset.x, tc2d.Offset.y + tc2d.Size.y * scale.y * 0.5f);
			triangleShape.Set(vertices, 3);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &triangleShape;
			fixtureDef.density = tc2d.Density;
			fixtureDef.friction = tc2d.Friction;
			fixtureDef.restitution = tc2d.Restitution;
			fixtureDef.restitutionThreshold = tc2d.RestitutionThreshold;
			fixtureDef.filter.categoryBits = rb2d.CollisionLayer;
			fixtureDef.filter.maskBits = rb2d.CollisionMask;
			tc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}
	}

	return true;
}

void Physics2D::OnRuntimeStop(Scene* scene)
{
	Scene* sceneToReset = scene ? scene : s_RuntimeScene;
	ResetRuntimeHandles(sceneToReset);
	delete s_PhysicsWorld;
	s_PhysicsWorld = nullptr;
	s_RuntimeScene = nullptr;
}

void Physics2D::OnUpdate(Scene* scene, Timestep ts)
{
	if (!s_PhysicsWorld || ts == 0.0f)
		return;

	const int32_t velocityIterations = 6;
	const int32_t positionIterations = 2;
	s_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

	auto view = scene->GetRegistry().view<Rigidbody2DComponent>();
	for (auto e : view)
	{
		Entity entity = { e, scene };
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		if (!body) continue;

		const auto& position = body->GetPosition();
		glm::vec3 scale = ExtractScale(transform.Transform);
		float angle = body->GetAngle();

		transform.Transform[3][0] = position.x;
		transform.Transform[3][1] = position.y;
		transform.Transform[0] = glm::vec4(cos(angle) * scale.x, sin(angle) * scale.x, 0.0f, 0.0f);
		transform.Transform[1] = glm::vec4(-sin(angle) * scale.y, cos(angle) * scale.y, 0.0f, 0.0f);
		transform.Transform[2] = glm::vec4(0.0f, 0.0f, scale.z, 0.0f);
	}
}

bool Physics2D::Raycast(const glm::vec2& origin, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	class LocalRaycastCallback : public b2RayCastCallback
	{
	public:
		LocalRaycastCallback(Scene* scene) : m_Scene(scene) {}

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			if (!fixture || !fixture->GetBody() || !m_Scene)
				return 1.0f;

			b2Body* body = fixture->GetBody();
			uint64_t userData = body->GetUserData().pointer;

			auto view = m_Scene->GetRegistry().view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				if ((uint64_t)userData == (uint64_t)e)
				{
					m_Hit.EntityId = (uint32_t)e;
					m_Hit.Point = { point.x, point.y };
					m_Hit.Normal = { normal.x, normal.y };
					m_Hit.Distance = fraction;
					return fraction;
				}
			}
			return 1.0f;
		}

		RaycastHit m_Hit;
		Scene* m_Scene;
	};

	LocalRaycastCallback callback(s_RuntimeScene);
	b2Vec2 p1(origin.x, origin.y);
	b2Vec2 p2(origin.x + direction.x * distance, origin.y + direction.y * distance);
	s_PhysicsWorld->RayCast(&callback, p1, p2);

	if (callback.m_Hit.EntityId != 0)
	{
		outHit = callback.m_Hit;
		return true;
	}
	return false;
}

bool Physics2D::RaycastAll(const glm::vec2& origin, const glm::vec2& direction, float distance, uint32_t layerMask, std::vector<RaycastHit>& outHits)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	class LocalRaycastCallback : public b2RayCastCallback
	{
	public:
		LocalRaycastCallback(Scene* scene) : m_Scene(scene) {}

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			if (!fixture || !fixture->GetBody() || !m_Scene)
				return 1.0f;

			b2Body* body = fixture->GetBody();
			uint64_t userData = body->GetUserData().pointer;

			auto view = m_Scene->GetRegistry().view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				if ((uint64_t)userData == (uint64_t)e)
				{
					RaycastHit hit;
					hit.EntityId = (uint32_t)e;
					hit.Point = { point.x, point.y };
					hit.Normal = { normal.x, normal.y };
					hit.Distance = fraction;
					m_Hits.push_back(hit);
					return 1.0f;
				}
			}
			return 1.0f;
		}

		std::vector<RaycastHit> m_Hits;
		Scene* m_Scene;
	};

	outHits.clear();
	LocalRaycastCallback callback(s_RuntimeScene);
	b2Vec2 p1(origin.x, origin.y);
	b2Vec2 p2(origin.x + direction.x * distance, origin.y + direction.y * distance);
	s_PhysicsWorld->RayCast(&callback, p1, p2);
	outHits = callback.m_Hits;
	return !outHits.empty();
}

bool Physics2D::BoxCast(const glm::vec2& center, const glm::vec2& halfExtents, float angle, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	class LocalRaycastCallback : public b2RayCastCallback
	{
	public:
		LocalRaycastCallback(Scene* scene) : m_Scene(scene) {}

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			if (!fixture || !fixture->GetBody() || !m_Scene)
				return 1.0f;

			b2Body* body = fixture->GetBody();
			uint64_t userData = body->GetUserData().pointer;

			auto view = m_Scene->GetRegistry().view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				if ((uint64_t)userData == (uint64_t)e)
				{
					m_Hit.EntityId = (uint32_t)e;
					m_Hit.Point = { point.x, point.y };
					m_Hit.Normal = { normal.x, normal.y };
					m_Hit.Distance = fraction;
					return fraction;
				}
			}
			return 1.0f;
		}

		RaycastHit m_Hit;
		Scene* m_Scene;
	};

	b2PolygonShape box;
	box.SetAsBox(halfExtents.x, halfExtents.y);

	b2AABB aabb;
	box.ComputeAABB(&aabb, b2Transform(b2Vec2(center.x, center.y), b2Rot(angle)), 0);

	b2Vec2 p1(center.x, center.y);
	b2Vec2 p2(center.x + direction.x * distance, center.y + direction.y * distance);

	LocalRaycastCallback callback(s_RuntimeScene);
	s_PhysicsWorld->RayCast(&callback, p1, p2);

	if (callback.m_Hit.EntityId != 0)
	{
		outHit = callback.m_Hit;
		return true;
	}
	return false;
}

bool Physics2D::CircleCast(const glm::vec2& center, float radius, const glm::vec2& direction, float distance, uint32_t layerMask, RaycastHit& outHit)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	class LocalRaycastCallback : public b2RayCastCallback
	{
	public:
		LocalRaycastCallback(Scene* scene) : m_Scene(scene) {}

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			if (!fixture || !fixture->GetBody() || !m_Scene)
				return 1.0f;

			b2Body* body = fixture->GetBody();
			uint64_t userData = body->GetUserData().pointer;

			auto view = m_Scene->GetRegistry().view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				if ((uint64_t)userData == (uint64_t)e)
				{
					m_Hit.EntityId = (uint32_t)e;
					m_Hit.Point = { point.x, point.y };
					m_Hit.Normal = { normal.x, normal.y };
					m_Hit.Distance = fraction;
					return fraction;
				}
			}
			return 1.0f;
		}

		RaycastHit m_Hit;
		Scene* m_Scene;
	};

	b2CircleShape circle;
	circle.m_radius = radius;

	b2AABB aabb;
	circle.ComputeAABB(&aabb, b2Transform(b2Vec2(center.x, center.y), b2Rot(0.0f)), 0);

	b2Vec2 p1(center.x, center.y);
	b2Vec2 p2(center.x + direction.x * distance, center.y + direction.y * distance);

	LocalRaycastCallback callback(s_RuntimeScene);
	s_PhysicsWorld->RayCast(&callback, p1, p2);

	if (callback.m_Hit.EntityId != 0)
	{
		outHit = callback.m_Hit;
		return true;
	}
	return false;
}

bool Physics2D::OverlapPoint(const glm::vec2& point, uint32_t layerMask)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	b2AABB aabb;
	aabb.lowerBound.Set(point.x - 0.001f, point.y - 0.001f);
	aabb.upperBound.Set(point.x + 0.001f, point.y + 0.001f);

	class PointQueryCallback : public b2QueryCallback
	{
	public:
		PointQueryCallback(const b2Vec2& pt, Scene* scene)
			: m_Point(pt), m_Scene(scene), m_Hit(false), m_EntityId(0) {}

		bool ReportFixture(b2Fixture* fixture) override
		{
			b2Shape* shape = fixture->GetShape();
			if (shape->TestPoint(fixture->GetBody()->GetTransform(), m_Point))
			{
				m_Hit = true;
				m_EntityId = (uint32_t)(uint64_t)fixture->GetBody()->GetUserData().pointer;
				return false;
			}
			return true;
		}

		bool m_Hit;
		uint32_t m_EntityId;
		b2Vec2 m_Point;
		Scene* m_Scene;
	};

	PointQueryCallback callback(b2Vec2(point.x, point.y), s_RuntimeScene);
	s_PhysicsWorld->QueryAABB(&callback, aabb);
	return callback.m_Hit;
}

bool Physics2D::OverlapBox(const glm::vec2& center, const glm::vec2& halfExtents, float angle, uint32_t layerMask)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	b2PolygonShape box;
	box.SetAsBox(halfExtents.x, halfExtents.y);

	b2AABB aabb;
	box.ComputeAABB(&aabb, b2Transform(b2Vec2(center.x, center.y), b2Rot(angle)), 0);

	class BoxQueryCallback : public b2QueryCallback
	{
	public:
		BoxQueryCallback(const b2Shape* shape, const b2Transform& tx, Scene* scene)
			: m_Shape(shape), m_Transform(tx), m_Scene(scene), m_Hit(false) {}

		bool ReportFixture(b2Fixture* fixture) override
		{
			if (b2TestOverlap(m_Shape, 0, fixture->GetShape(), 0, m_Transform, fixture->GetBody()->GetTransform()))
			{
				m_Hit = true;
				return false;
			}
			return true;
		}

		bool m_Hit;
		const b2Shape* m_Shape;
		b2Transform m_Transform;
		Scene* m_Scene;
	};

	BoxQueryCallback callback(&box, b2Transform(b2Vec2(center.x, center.y), b2Rot(angle)), s_RuntimeScene);
	s_PhysicsWorld->QueryAABB(&callback, aabb);
	return callback.m_Hit;
}

bool Physics2D::OverlapCircle(const glm::vec2& center, float radius, uint32_t layerMask)
{
	if (!s_PhysicsWorld || !s_RuntimeScene)
		return false;

	b2CircleShape circle;
	circle.m_radius = radius;
	circle.m_p.Set(0, 0);

	b2AABB aabb;
	circle.ComputeAABB(&aabb, b2Transform(b2Vec2(center.x, center.y), b2Rot(0.0f)), 0);

	class CircleQueryCallback : public b2QueryCallback
	{
	public:
		CircleQueryCallback(const b2Shape* shape, Scene* scene)
			: m_Shape(shape), m_Scene(scene), m_Hit(false) {}

		bool ReportFixture(b2Fixture* fixture) override
		{
			if (b2TestOverlap(m_Shape, 0, fixture->GetShape(), 0, b2Transform(b2Vec2(0, 0), b2Rot(0.0f)), fixture->GetBody()->GetTransform()))
			{
				m_Hit = true;
				return false;
			}
			return true;
		}

		bool m_Hit;
		const b2Shape* m_Shape;
		Scene* m_Scene;
	};

	CircleQueryCallback callback(&circle, s_RuntimeScene);
	s_PhysicsWorld->QueryAABB(&callback, aabb);
	return callback.m_Hit;
}

}
