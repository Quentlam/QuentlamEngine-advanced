#pragma once

#include "Quentlam/Core/Base.h"
#include "Quentlam/Renderer/Texture.h"
#include <vector>
#include <string>
#include <functional>

namespace Quentlam
{

class RenderPass;

enum class EPostProcessEffectType
{
	Bloom,
	ColorGrading,
	Vignette,
	ChromaticAberration,
	MotionBlur,
	Blur,
	Sharpen
};

struct PostProcessSettings
{
	bool Enabled = true;
	float Intensity = 1.0f;
};

struct BloomSettings : public PostProcessSettings
{
	float Threshold = 0.8f;
	float SoftThreshold = 0.5f;
	float Radius = 1.0f;
	float Intensity = 1.0f;
};

struct ColorGradingSettings : public PostProcessSettings
{
	float Exposure = 1.0f;
	float Contrast = 1.0f;
	float Saturation = 1.0f;
	float Brightness = 0.0f;

	float Temperature = 0.0f;
	float Tint = 0.0f;

	float Highlights = 0.0f;
	float Shadows = 0.0f;
	float Midtones = 0.0f;

	float RedOffset = 0.0f;
	float GreenOffset = 0.0f;
	float BlueOffset = 0.0f;
	float Gamma = 1.0f;
};

struct VignetteSettings : public PostProcessSettings
{
	float Darkness = 0.5f;
	float Offset = 0.5f;
	float Roundness = 1.0f;
	float Smoothness = 0.2f;
};

struct ChromaticAberrationSettings : public PostProcessSettings
{
	float Intensity = 0.5f;
	float CenterX = 0.5f;
	float CenterY = 0.5f;
};

struct MotionBlurSettings : public PostProcessSettings
{
	int SampleCount = 8;
	float Strength = 1.0f;
};

struct BlurSettings : public PostProcessSettings
{
	float Radius = 1.0f;
	int Iterations = 1;
};

struct SharpenSettings : public PostProcessSettings
{
	float Intensity = 0.5f;
	float EdgeThreshold = 0.0f;
};

class PostProcessEffect
{
public:
	PostProcessEffect() = default;
	virtual ~PostProcessEffect() = default;

	virtual EPostProcessEffectType GetType() const = 0;
	virtual const char* GetName() const = 0;
	virtual void Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize) = 0;

	PostProcessSettings* GetSettings() { return &m_Settings; }
	const PostProcessSettings* GetSettings() const { return &m_Settings; }

protected:
	PostProcessSettings m_Settings;
};

class BloomEffect : public PostProcessEffect
{
public:
	BloomEffect();
	EPostProcessEffectType GetType() const override { return EPostProcessEffectType::Bloom; }
	const char* GetName() const override { return "Bloom"; }
	void Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize) override;

	BloomSettings* GetBloomSettings() { return &m_BloomSettings; }
	const BloomSettings* GetBloomSettings() const { return &m_BloomSettings; }

private:
	BloomSettings m_BloomSettings;
	Ref<Texture2D> m_BrightTexture;
	Ref<Texture2D> m_HorizontalTexture;
	Ref<Texture2D> m_VerticalTexture;
};

class ColorGradingEffect : public PostProcessEffect
{
public:
	ColorGradingEffect();
	EPostProcessEffectType GetType() const override { return EPostProcessEffectType::ColorGrading; }
	const char* GetName() const override { return "Color Grading"; }
	void Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize) override;

	ColorGradingSettings* GetColorGradingSettings() { return &m_ColorGradingSettings; }

private:
	ColorGradingSettings m_ColorGradingSettings;
};

class VignetteEffect : public PostProcessEffect
{
public:
	VignetteEffect();
	EPostProcessEffectType GetType() const override { return EPostProcessEffectType::Vignette; }
	const char* GetName() const override { return "Vignette"; }
	void Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize) override;

	VignetteSettings* GetVignetteSettings() { return &m_VignetteSettings; }

private:
	VignetteSettings m_VignetteSettings;
};

class ChromaticAberrationEffect : public PostProcessEffect
{
public:
	ChromaticAberrationEffect();
	EPostProcessEffectType GetType() const override { return EPostProcessEffectType::ChromaticAberration; }
	const char* GetName() const override { return "Chromatic Aberration"; }
	void Apply(Ref<Texture2D> input, Ref<Texture2D> output, const glm::vec2& viewportSize) override;

	ChromaticAberrationSettings* GetChromaticAberrationSettings() { return &m_ChromaticSettings; }

private:
	ChromaticAberrationSettings m_ChromaticSettings;
};

class PostProcessingPipeline
{
public:
	PostProcessingPipeline();
	~PostProcessingPipeline();

	void Initialize(int width, int height);
	void Resize(int width, int height);
	void Shutdown();

	void AddEffect(Ref<PostProcessEffect> effect);
	void RemoveEffect(Ref<PostProcessEffect> effect);
	void RemoveEffectByType(EPostProcessEffectType type);

	Ref<PostProcessEffect> GetEffect(EPostProcessEffectType type);
	const std::vector<Ref<PostProcessEffect>>& GetEffects() const { return m_Effects; }

	void Render(Ref<Texture2D> sceneTexture, const glm::vec2& viewportSize);

	Ref<Texture2D> GetOutputTexture() const { return m_OutputTexture; }

private:
	std::vector<Ref<PostProcessEffect>> m_Effects;
	Ref<Texture2D> m_OutputTexture;
	Ref<Texture2D> m_PingTexture;
	Ref<Texture2D> m_PongTexture;
	int m_Width = 0;
	int m_Height = 0;
};

}
