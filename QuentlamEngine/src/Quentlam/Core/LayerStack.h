#pragma once
#include "Quentlam/Core/Base.h"
#include "Layer.h"

#include <vector>

namespace Quentlam
{
	class QUENTLAM_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* Layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);


		uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_Layers.size()); }
		const std::string& GetLayerName(uint32_t index) const { return m_Layers[index]->GetName(); }


		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }


	private:
		std::vector<Layer*> m_Layers;
		unsigned int  m_LayerInsertIndex = 0;
	};


}
