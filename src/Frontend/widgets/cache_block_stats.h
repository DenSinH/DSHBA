#ifndef DSHBA_CACHE_BLOCK_STATS_H
#define DSHBA_CACHE_BLOCK_STATS_H

#include <array>
#include <imgui/imgui.h>
#include <cstdint>
#include <algorithm>
#include "../imgui/implot.h"

#define METRICS_HISTORY_SECONDS 5

#define METRICS_HISTORY_ITEMS ((METRICS_HISTORY_SECONDS) * 60)

template<typename T>
struct RingBuffer {
    int offset;
    std::array<T, METRICS_HISTORY_ITEMS> data;

    RingBuffer() {
        offset = 0;
        memset(data.data(), 0, sizeof(data));
    }

    T max() {
        return *std::max_element(std::begin(data), std::end(data));
    }

    void add_point(T point) {
        data[offset++] = point;
        offset %= METRICS_HISTORY_ITEMS;
    }
};

struct CacheBlockStats {

    RingBuffer<uint32_t> cached_steps;
    RingBuffer<uint32_t> make_cached_steps;
    RingBuffer<uint32_t> non_cached_steps;

    uint32_t* cached_steps_ptr;
    uint32_t* make_cached_steps_ptr;
    uint32_t* non_cached_steps_ptr;

    CacheBlockStats(uint32_t* cached, uint32_t* make_cached, uint32_t* non_cached) :
        cached_steps_ptr(cached),
        make_cached_steps_ptr(make_cached),
        non_cached_steps_ptr(non_cached) {

    }

    CacheBlockStats() :
        cached_steps_ptr(nullptr),
        make_cached_steps_ptr(nullptr),
        non_cached_steps_ptr(nullptr) {

    }

    void Update() {
        if (!cached_steps_ptr || !make_cached_steps_ptr || !non_cached_steps_ptr) {
            return;
        }

        uint32_t cached = *cached_steps_ptr;
        uint32_t make_cached = *make_cached_steps_ptr;
        uint32_t non_cached = *non_cached_steps_ptr;

        *cached_steps_ptr = 0;
        *make_cached_steps_ptr = 0;
        *non_cached_steps_ptr = 0;

        cached_steps.add_point(non_cached + make_cached + cached);
        make_cached_steps.add_point(non_cached + make_cached);
        non_cached_steps.add_point(non_cached);
    }

    void Draw(bool* p_open) {
        if (!cached_steps_ptr || !make_cached_steps_ptr || !non_cached_steps_ptr) {
            return;
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(-1, -1),    ImVec2(-1, -1));
        ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_Once);

        if (!ImGui::Begin("Cache Block Stats", p_open))
        {
            ImGui::End();
            return;
        }

        ImPlot::SetNextPlotLimitsY(0, cached_steps.max(), ImGuiCond_Always, 0);
        ImPlot::SetNextPlotLimitsX(0, METRICS_HISTORY_ITEMS, ImGuiCond_Always);
        if (ImPlot::BeginPlot("Cache Block Stats")) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1, 0, 0, 1));
            ImPlot::PlotLine("Non-Cached Steps", non_cached_steps.data.data(), METRICS_HISTORY_ITEMS, 1, 0, non_cached_steps.offset);
            ImPlot::PopStyleColor();
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, 0, 1, 1));
            ImPlot::PlotLine("Make Cached Steps", make_cached_steps.data.data(), METRICS_HISTORY_ITEMS, 1, 0, make_cached_steps.offset);
            ImPlot::PopStyleColor();
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, 1, 0, 1));
            ImPlot::PlotLine("Cached Steps", cached_steps.data.data(), METRICS_HISTORY_ITEMS, 1, 0, cached_steps.offset);
            ImPlot::PopStyleColor();
            ImPlot::EndPlot();
        }

        ImGui::End();
    }
};

#endif //DSHBA_CACHE_BLOCK_STATS_H
