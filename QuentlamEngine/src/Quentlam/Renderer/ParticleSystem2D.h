#pragma once
#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <vector>
#include <random>

namespace Quentlam
{

struct Particle2D
{
	glm::vec2 Position;
	glm::vec2 Velocity;
	glm::vec2 Size;
	glm::vec4 Color;
	float Rotation = 0.0f;
	float AngularVelocity = 0.0f;
	float Life = 0.0f;
	float MaxLife = 1.0f;
	float CurrentTime = 0.0f;
};

struct ParticleSystemConfig
{
	float EmissionRate = 20.0f;
	int32_t MaxParticles = 200;
	float ParticleLifetime = 1.0f;
	float ParticleSpeed = 2.0f;
	glm::vec2 VelocityMin = { -1.0f, -1.0f };
	glm::vec2 VelocityMax = { 1.0f, 1.0f };
	glm::vec2 SizeMin = { 0.1f, 0.1f };
	glm::vec2 SizeMax = { 0.3f, 0.3f };
	glm::vec4 ColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
	float RotationMin = 0.0f;
	float RotationMax = 360.0f;
	float AngularVelocityMin = -180.0f;
	float AngularVelocityMax = 180.0f;
	float GravityModifier = 0.0f;
	bool Looping = true;
	float StartDelay = 0.0f;
	float Duration = 5.0f;
	Ref<Texture2D> Texture;
};

class QUENTLAM_API ParticleEmitter2D
{
public:
	ParticleEmitter2D() = default;
	ParticleEmitter2D(const ParticleSystemConfig& config);

	void SetConfig(const ParticleSystemConfig& config) { m_Config = config; }
	const ParticleSystemConfig& GetConfig() const { return m_Config; }

	void Play();
	void Stop();
	void Pause();
	void Resume();

	bool IsPlaying() const { return m_IsPlaying; }
	bool IsPaused() const { return m_IsPaused; }

	void Update(float dt);
	void Emit(int32_t count = 1);

	size_t GetParticleCount() const { return m_Particles.size(); }
	const std::vector<Particle2D>& GetParticles() const { return m_Particles; }

	void SetPosition(const glm::vec2& pos) { m_Position = pos; }
	const glm::vec2& GetPosition() const { return m_Position; }

	float GetEmissionAccumulator() const { return m_EmissionAccumulator; }

private:
	Particle2D CreateParticle();

	ParticleSystemConfig m_Config;
	std::vector<Particle2D> m_Particles;
	glm::vec2 m_Position = { 0.0f, 0.0f };

	bool m_IsPlaying = false;
	bool m_IsPaused = false;
	float m_EmissionAccumulator = 0.0f;
	float m_PlayTime = 0.0f;
	float m_DelayTimer = 0.0f;

	std::mt19937 m_RNG{ std::random_device{}() };
	std::uniform_real_distribution<float> m_Dist{ 0.0f, 1.0f };
};

inline ParticleEmitter2D::ParticleEmitter2D(const ParticleSystemConfig& config)
	: m_Config(config) {}

inline void ParticleEmitter2D::Play()
{
	m_IsPlaying = true;
	m_IsPaused = false;
	m_EmissionAccumulator = 0.0f;
	m_PlayTime = 0.0f;
	m_DelayTimer = m_Config.StartDelay;
}

inline void ParticleEmitter2D::Stop()
{
	m_IsPlaying = false;
	m_IsPaused = false;
	m_EmissionAccumulator = 0.0f;
	m_PlayTime = 0.0f;
	m_Particles.clear();
}

inline void ParticleEmitter2D::Pause()
{
	if (m_IsPlaying)
		m_IsPaused = true;
}

inline void ParticleEmitter2D::Resume()
{
	if (m_IsPlaying && m_IsPaused)
		m_IsPaused = false;
}

inline void ParticleEmitter2D::Update(float dt)
{
	if (!m_IsPlaying || m_IsPaused)
		return;

	m_PlayTime += dt;

	if (m_DelayTimer > 0.0f)
	{
		m_DelayTimer -= dt;
		if (m_DelayTimer > 0.0f)
			return;
	}

	if (!m_Config.Looping && m_PlayTime > m_Config.Duration + m_Config.StartDelay)
	{
		m_IsPlaying = false;
	}

	m_EmissionAccumulator += m_Config.EmissionRate * dt;
	int32_t toEmit = static_cast<int32_t>(m_EmissionAccumulator);
	if (toEmit > 0)
	{
		m_EmissionAccumulator -= toEmit;
		Emit(toEmit);
	}

	for (auto it = m_Particles.begin(); it != m_Particles.end(); )
	{
		auto& p = *it;
		p.CurrentTime += dt;
		p.Life -= dt;

		if (p.Life <= 0.0f)
		{
			it = m_Particles.erase(it);
			continue;
		}

		p.Velocity.y -= m_Config.GravityModifier * 9.8f * dt;
		p.Position += p.Velocity * dt;
		p.Rotation += p.AngularVelocity * dt;
		++it;
	}
}

inline void ParticleEmitter2D::Emit(int32_t count)
{
	int32_t space = m_Config.MaxParticles - static_cast<int32_t>(m_Particles.size());
	int32_t actual = std::min(count, space);
	for (int32_t i = 0; i < actual; ++i)
		m_Particles.push_back(CreateParticle());
}

inline Particle2D ParticleEmitter2D::CreateParticle()
{
	Particle2D p;
	p.Position = m_Position;
	float rx = m_Dist(m_RNG);
	float ry = m_Dist(m_RNG);
	float rx2 = m_Dist(m_RNG);
	float ry2 = m_Dist(m_RNG);
	float rx3 = m_Dist(m_RNG);
	float rx4 = m_Dist(m_RNG);

	p.Velocity.x = (rx * (m_Config.VelocityMax.x - m_Config.VelocityMin.x) + m_Config.VelocityMin.x) * m_Config.ParticleSpeed;
	p.Velocity.y = (ry * (m_Config.VelocityMax.y - m_Config.VelocityMin.y) + m_Config.VelocityMin.y) * m_Config.ParticleSpeed;
	p.Size.x = rx2 * (m_Config.SizeMax.x - m_Config.SizeMin.x) + m_Config.SizeMin.x;
	p.Size.y = ry2 * (m_Config.SizeMax.y - m_Config.SizeMin.y) + m_Config.SizeMin.y;
	p.Color = m_Config.ColorBegin;
	p.Life = m_Config.ParticleLifetime;
	p.MaxLife = m_Config.ParticleLifetime;
	p.CurrentTime = 0.0f;
	p.Rotation = rx3 * (m_Config.RotationMax - m_Config.RotationMin) + m_Config.RotationMin;
	p.AngularVelocity = rx4 * (m_Config.AngularVelocityMax - m_Config.AngularVelocityMin) + m_Config.AngularVelocityMin;
	return p;
}

}
