#pragma once

#include "Quentlam/Core/Layer.h"
#include "Quentlam/Persistence/GameStateSerializer.h"
#include "Quentlam/Audio/AudioModule.h"
#include <string>

class SaveLoadTestLayer : public Quentlam::Layer
{
public:
	SaveLoadTestLayer();
	~SaveLoadTestLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Quentlam::Timestep ts) override;
	void OnEvent(Quentlam::Event& event) override;
	void OnImGuiLayer() override;

private:
	Quentlam::GameStateSerializer m_Serializer;
	std::string m_SlotId = "test_slot_001";
	std::string m_LastMessage;
	float m_MessageTimer = 0.0f;
};
