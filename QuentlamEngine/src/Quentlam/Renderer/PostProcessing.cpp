#include "qlpch.h"
#include "PostProcessing.h"
#include "Quentlam/Renderer/Renderer.h"
#include "Quentlam/Renderer/Shader.h"
#include "Quentlam/Renderer/FrameBuffer.h"

namespace Quentlam
{

BloomEffect::BloomEffect() {}
void BloomEffect::Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize)
{
}

ColorGradingEffect::ColorGradingEffect() {}
void ColorGradingEffect::Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize)
{
}

VignetteEffect::VignetteEffect() {}
void VignetteEffect::Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize)
{
}

ChromaticAberrationEffect::ChromaticAberrationEffect() {}
void ChromaticAberrationEffect::Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize)
{
}

PostProcessingPipeline::PostProcessingPipeline()
{
}

PostProcessingPipeline::~PostProcessingPipeline()
{
	Shutdown();
}

void PostProcessingPipeline::Initialize(int width, int height)
{
	m_Width = width;
	m_Height = height;

	QL_CORE_INFO("PostProcessingPipeline: Initialized at {0}x{1}", width, height);
}

void PostProcessingPipeline::Resize(int width, int height)
{
	if (m_Width == width && m_Height == height)
		return;

	m_Width = width;
	m_Height = height;

	QL_CORE_INFO("PostProcessingPipeline: Resized to {0}x{1}", width, height);
}

void PostProcessingPipeline::Shutdown()
{
	m_Effects.clear();
	m_OutputTexture.reset();
	m_PingTexture.reset();
	m_PongTexture.reset();
	m_Width = 0;
	m_Height = 0;
}

void PostProcessingPipeline::AddEffect(Ref<PostProcessEffect> effect)
{
	m_Effects.push_back(effect);
	QL_CORE_INFO("PostProcessingPipeline: Added effect '{0}'", effect->GetName());
}

void PostProcessingPipeline::RemoveEffect(Ref<PostProcessEffect> effect)
{
	m_Effects.erase(
		std::remove_if(m_Effects.begin(), m_Effects.end(),
			[&effect](const Ref<PostProcessEffect>& e) { return e.get() == effect.get(); }),
		m_Effects.end());
}

void PostProcessingPipeline::RemoveEffectByType(EPostProcessEffectType type)
{
	m_Effects.erase(
		std::remove_if(m_Effects.begin(), m_Effects.end(),
			[type](const Ref<PostProcessEffect>& e) { return e->GetType() == type; }),
		m_Effects.end());
}

Ref<PostProcessEffect> PostProcessingPipeline::GetEffect(EPostProcessEffectType type)
{
	for (auto& effect : m_Effects)
	{
		if (effect->GetType() == type)
			return effect;
	}
	return nullptr;
}

void PostProcessingPipeline::Render(Ref<Texture2D> sceneTexture, const glm::vec2& viewportSize)
{
	if (m_Effects.empty() || !sceneTexture)
		return;

	Ref<Texture2D> currentTexture = sceneTexture;

	for (size_t i = 0; i < m_Effects.size(); ++i)
	{
		auto& effect = m_Effects[i];
		if (!effect->GetSettings()->Enabled)
			continue;

		if (i == m_Effects.size() - 1)
		{
			effect->Apply(currentTexture, m_OutputTexture, viewportSize);
		}
		else
		{
			Ref<Texture2D> target = (i % 2 == 0) ? m_PingTexture : m_PongTexture;
			effect->Apply(currentTexture, target, viewportSize);
			currentTexture = target;
		}
	}
}

}
