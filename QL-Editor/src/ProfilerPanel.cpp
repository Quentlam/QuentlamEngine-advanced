#include "ProfilerPanel.h"
#include "Quentlam/Renderer/Renderer2D.h"
#include "Quentlam/Renderer/Renderer3D.h"
#include "Quentlam/Resource/ResourceManager.h"
#include "imgui/imgui.h"

namespace Quentlam {

    static ImVec4 MakeColor(float r, float g, float b, float a = 1.0f)
    {
        return ImVec4(r, g, b, a);
    }

    static ImVec4 HealthColor(float ratio)
    {
        if (ratio >= 0.7f) return MakeColor(0.2f, 0.8f, 0.2f);
        if (ratio >= 0.4f) return MakeColor(1.0f, 0.8f, 0.2f);
        return MakeColor(1.0f, 0.2f, 0.2f);
    }

    void EditorProfilerOverlay::CollectMetrics()
    {
        auto r2dStats = Renderer2D::GetStatistics();
        auto r3dStats = Renderer3D::GetStatistics();

        float fps = 0.0f;
        float frameMs = 0.0f;

		float currentFrameTime = static_cast<float>(ImGui::GetTime());
        static float lastFrameTime = currentFrameTime;
        static float fpsAccumulator = 0.0f;
        static int frameCounter = 0;
        static float fpsDisplay = 60.0f;

        float delta = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        frameMs = delta * 1000.0f;

        fpsAccumulator += delta;
        frameCounter++;
        if (fpsAccumulator >= 0.5f)
        {
            fpsDisplay = frameCounter / fpsAccumulator;
            fpsAccumulator = 0.0f;
            frameCounter = 0;
        }
        fps = fpsDisplay;

        if (m_FPSHistory.size() >= static_cast<size_t>(m_GraphDataCount))
            m_FPSHistory.erase(m_FPSHistory.begin());
        m_FPSHistory.push_back(fps);

        if (m_FrameMsHistory.size() >= static_cast<size_t>(m_GraphDataCount))
            m_FrameMsHistory.erase(m_FrameMsHistory.begin());
        m_FrameMsHistory.push_back(frameMs);

        m_Metrics.FPS = fps;
        m_Metrics.FrameMs = frameMs;
        m_Metrics.DrawCalls2D = static_cast<int>(r2dStats.DrawCalls);
        m_Metrics.DrawCalls3D = static_cast<int>(r3dStats.DrawCalls);
        m_Metrics.QuadCount = static_cast<int>(r2dStats.QuadCount);
        m_Metrics.CubeCount = static_cast<int>(r3dStats.CubeCount);
        m_Metrics.VertexCount2D = static_cast<int>(r2dStats.GetTotalVertexCount());
        m_Metrics.VertexCount3D = static_cast<int>(r3dStats.GetTotalVertexCount());
    }

    void EditorProfilerOverlay::DrawBar(const char* label, float value, float max, const ImVec4& color)
    {
        float ratio = max > 0.0f ? (value / max) : 0.0f;
        ratio = ratio > 1.0f ? 1.0f : ratio;

        ImGui::Text("%s", label);
        ImGui::SameLine(100.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(ratio, ImVec2(100.0f, 0.0f));
        ImGui::PopStyleColor();
        ImGui::SameLine(210.0f);
        ImGui::Text("%.1f", value);
    }

    void EditorProfilerOverlay::DrawFPSGraph()
    {
        if (m_FPSHistory.size() < 2) return;

        ImGui::Text("FPS History");
        ImGui::SameLine();
        ImGui::TextColored(HealthColor(60.0f / kDefaultMaxFPS), "%.1f", m_Metrics.FPS);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        std::vector<float> fpsCopy = m_FPSHistory;
        ImGui::PlotLines("##FPS", fpsCopy.data(), static_cast<int>(fpsCopy.size()), 0, nullptr, 0.0f, kDefaultMaxFPS, ImVec2(0, 60.0f));
        ImGui::PopStyleColor(2);
    }

    void EditorProfilerOverlay::DrawMetrics()
    {
        ImGui::Columns(2, "metrics", true);
        ImGui::SetColumnWidth(0, 150.0f);

        ImGui::Text("FPS");
        ImGui::NextColumn();
        ImGui::TextColored(HealthColor(60.0f / kDefaultMaxFPS), "%.1f", m_Metrics.FPS);
        ImGui::NextColumn();

        ImGui::Text("Frame Time");
        ImGui::NextColumn();
        ImGui::TextColored(HealthColor(16.67f / m_Metrics.FrameMs), "%.2f ms", m_Metrics.FrameMs);
        ImGui::NextColumn();

        ImGui::Text("Draw Calls (2D)");
        ImGui::NextColumn();
        ImGui::TextColored(HealthColor(1.0f - m_Metrics.DrawCalls2D / kDefaultMaxDrawCalls),
            "%d", m_Metrics.DrawCalls2D);
        ImGui::NextColumn();

        ImGui::Text("Draw Calls (3D)");
        ImGui::NextColumn();
        ImGui::TextColored(HealthColor(1.0f - m_Metrics.DrawCalls3D / kDefaultMaxDrawCalls),
            "%d", m_Metrics.DrawCalls3D);
        ImGui::NextColumn();

        ImGui::Text("Quads");
        ImGui::NextColumn();
        ImGui::Text("%d", m_Metrics.QuadCount);
        ImGui::NextColumn();

        ImGui::Text("Cubes");
        ImGui::NextColumn();
        ImGui::Text("%d", m_Metrics.CubeCount);
        ImGui::NextColumn();

        ImGui::Text("Vertices (2D)");
        ImGui::NextColumn();
        ImGui::Text("%d", m_Metrics.VertexCount2D);
        ImGui::NextColumn();

        ImGui::Text("Vertices (3D)");
        ImGui::NextColumn();
        ImGui::Text("%d", m_Metrics.VertexCount3D);
        ImGui::NextColumn();

        ImGui::Columns(1);
    }

    void EditorProfilerOverlay::OnImGuiRender()
    {
        if (!m_Visible) return;

        CollectMetrics();

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::SetNextWindowPos(ImVec2(
            ImGui::GetIO().DisplaySize.x - 10.0f,
            ImGui::GetIO().DisplaySize.y - 10.0f),
            ImGuiCond_Always, ImVec2(1.0f, 1.0f));

        if (!ImGui::Begin("##ProfilerOverlay", &m_Visible, flags))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Profiler");
        ImGui::Separator();

        DrawMetrics();
        ImGui::Spacing();
        DrawFPSGraph();

        ImGui::End();
    }

} // namespace Quentlam
