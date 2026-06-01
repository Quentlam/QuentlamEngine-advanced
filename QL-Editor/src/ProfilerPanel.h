#pragma once

#include "imgui/imgui.h"
#include <string>
#include <vector>

namespace Quentlam {

    class EditorProfilerOverlay {
    public:
        struct ModuleStats {
            std::string Name;
            float TimeMs = 0.0f;
        };

        struct Metrics {
            float FPS = 60.0f;
            float FrameMs = 0.0f;
            int DrawCalls2D = 0;
            int DrawCalls3D = 0;
            int QuadCount = 0;
            int CubeCount = 0;
            int VertexCount2D = 0;
            int VertexCount3D = 0;
            float TextureMemoryMB = 0.0f;
            int TextureCount = 0;
            std::vector<ModuleStats> Modules;
        };

        void OnImGuiRender();
        void CollectMetrics();

        bool IsVisible() const { return m_Visible; }
        void Toggle() { m_Visible = !m_Visible; }
        void SetVisible(bool v) { m_Visible = v; }

    private:
        void DrawMetrics();
        void DrawFPSGraph();
        void DrawBar(const char* label, float value, float max, const ImVec4& color);

        bool m_Visible = true;
        int m_GraphDataCount = 60;
        std::vector<float> m_FPSHistory;
        std::vector<float> m_FrameMsHistory;
        Metrics m_Metrics;

        static constexpr float kDefaultMaxFPS = 144.0f;
        static constexpr float kDefaultMaxFrameMs = 16.67f;
        static constexpr float kDefaultMaxDrawCalls = 200.0f;
    };
}
