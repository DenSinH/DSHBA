#ifndef GC__MEMORY_VIEWER_H
#define GC__MEMORY_VIEWER_H

#include "imgui/imgui.h"
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <cstdio>      // sprintf, scanf
#include <cstdint>     // uint8_t, etc.

#ifdef _MSC_VER
#define _PRISizeT   "I"
#define ImSnprintf  _snprintf
#else
#define _PRISizeT   "z"
#define ImSnprintf  snprintf
#endif

#include "default.h"

struct MemoryViewer
{
    enum DataFormat
    {
        DataFormat_Bin = 0,
        DataFormat_Dec = 1,
        DataFormat_Hex = 2,
        DataFormat_COUNT
    };

    // Settings
    bool            Open;                                       // = true   // set to false when DrawWindow() was closed. ignore if not using DrawWindow().
    bool            ReadOnly;                                   // = false  // disable any editing.
    int             Cols;                                       // = 16     // number of columns to display.
    bool            OptShowOptions;                             // = true   // display options button/context menu. when disabled, options will be locked unless you provide your own UI for them.
    bool            OptShowDataPreview;                         // = false  // display a footer previewing the decimal/binary/hex/float representation of the currently selected bytes.
    bool            OptShowHexII;                               // = false  // display values in HexII representation instead of regular hexadecimal: hide null/zero bytes, ascii values as ".X".
    bool            OptShowAscii;                               // = true   // display ASCII representation on the right side.
    bool            OptGreyOutZeroes;                           // = true   // display null/zero bytes using the TextDisabled color.
    bool            OptUpperCaseHex;                            // = true   // display hexadecimal values as "FF" instead of "ff".
    int             OptMidColsCount;                            // = 8      // set to 0 to disable extra spacing between every mid-cols.
    int             OptAddrDigitsCount;                         // = 0      // number of addr digits to display (default calculated based on maximum displayed addr).
    ImU32           HighlightColor;                             //          // background color of highlighted bytes.
    ImU8            (*ReadFn)(uint64_t off);    // = 0      // optional handler to read bytes.

    uint8_t* mem_data;
    uint64_t mem_size;

    // [Internal State]
    bool            ContentsWidthChanged;
    uint64_t        DataPreviewAddr;
    uint64_t        DataEditingAddr;
    bool            DataEditingTakeFocus;
    char            DataInputBuf[32];
    char            AddrInputBuf[32];
    uint64_t        GotoAddr;
    uint32_t        CenterAddr;
    uint64_t        LineTotalCount;
    uint64_t        HighlightMin, HighlightMax;
    int             PreviewEndianess;
    ImGuiDataType   PreviewDataType;

    MemoryViewer()
    {
        // Settings
        Open = true;
        ReadOnly = true;
        Cols = 16;
        OptShowOptions = true;
        OptShowDataPreview = false;
        OptShowHexII = false;
        OptShowAscii = true;
        OptGreyOutZeroes = true;
        OptUpperCaseHex = false;
        OptMidColsCount = 8;
        OptAddrDigitsCount = 0;
        HighlightColor = IM_COL32(255, 255, 255, 50);
        ReadFn = nullptr;

        // State/Internals
        ContentsWidthChanged = false;
        DataPreviewAddr = DataEditingAddr = (uint64_t)-1;
        DataEditingTakeFocus = false;
        memset(DataInputBuf, 0, sizeof(DataInputBuf));
        memset(AddrInputBuf, 0, sizeof(AddrInputBuf));
        GotoAddr = (uint64_t)-1;
        HighlightMin = HighlightMax = (uint64_t)-1;
        PreviewEndianess = 1;
        PreviewDataType = ImGuiDataType_S32;
    }

    void GotoAddrAndHighlight(uint64_t addr_min, uint64_t addr_max)
    {
        GotoAddr = addr_min;
        HighlightMin = addr_min;
        HighlightMax = addr_max;
    }

    struct Sizes
    {
        int     AddrDigitsCount;
        float   LineHeight;
        float   GlyphWidth;
        float   HexCellWidth;
        float   SpacingBetweenMidCols;
        float   PosHexStart;
        float   PosHexEnd;
        float   PosAsciiStart;
        float   PosAsciiEnd;
        float   WindowWidth;

        Sizes() { memset(this, 0, sizeof(*this)); }
    };

    void CalcSizes(Sizes& s, uint64_t base_display_addr) const
    {
        ImGuiStyle& style = ImGui::GetStyle();
        s.AddrDigitsCount = OptAddrDigitsCount;
        if (s.AddrDigitsCount == 0)
            for (uint64_t n = base_display_addr + mem_size - 1; n > 0; n >>= 4)
                s.AddrDigitsCount++;
        s.LineHeight = ImGui::GetTextLineHeight();
        s.GlyphWidth = ImGui::CalcTextSize("F").x + 1;                  // We assume the font is mono-space
        s.HexCellWidth = (float)(int)(s.GlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
        s.SpacingBetweenMidCols = (float)(int)(s.HexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing
        s.PosHexStart = (s.AddrDigitsCount + 2) * s.GlyphWidth;
        s.PosHexEnd = s.PosHexStart + (s.HexCellWidth * Cols);
        s.PosAsciiStart = s.PosAsciiEnd = s.PosHexEnd;
        if (OptShowAscii)
        {
            s.PosAsciiStart = s.PosHexEnd + s.GlyphWidth * 1;
            if (OptMidColsCount > 0)
                s.PosAsciiStart += (float)((float)(Cols + OptMidColsCount - 1) / OptMidColsCount) * s.SpacingBetweenMidCols;
            s.PosAsciiEnd = s.PosAsciiStart + Cols * s.GlyphWidth;
        }
        s.WindowWidth = s.PosAsciiEnd + style.ScrollbarSize + style.WindowPadding.x * 2 + s.GlyphWidth;
    }

    // Standalone Mem Editor window
    void Draw(bool* p_open)
    {
        uint64_t base_display_addr = 0;
        Sizes s;
        CalcSizes(s, base_display_addr);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

        if (ImGui::Begin("Mem Viewer", p_open, ImGuiWindowFlags_NoScrollbar))
        {
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup("context");
            DrawContents(base_display_addr);
            if (ContentsWidthChanged)
            {
                CalcSizes(s, base_display_addr);
                ImGui::SetWindowSize(ImVec2(s.WindowWidth, ImGui::GetWindowSize().y));
            }
        }
        ImGui::End();
    }

    // Mem Editor contents only
    void DrawContents(uint64_t base_display_addr = 0)
    {
        if (Cols < 1)
            Cols = 1;

        if (!ReadFn && !mem_data) {
            // nullptr passed
            return;
        }

        Sizes s;
        CalcSizes(s, base_display_addr);
        ImGuiStyle& style = ImGui::GetStyle();

        // We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove' in order to prevent click from moving the window.
        // This is used as a facility since our main click detection code doesn't assign an ActiveId so the click would normally be caught as a window-move.
        const float height_separator = style.ItemSpacing.y;
        float footer_height = 0;
        if (OptShowOptions)
            footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1;
        if (OptShowDataPreview)
            footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1 + ImGui::GetTextLineHeightWithSpacing() * 3;

        ImGui::BeginChild("##scrolling", ImVec2(0, -footer_height), false, ImGuiWindowFlags_NoMove);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        const auto line_total_count = (uint64_t)((mem_size + Cols - 1) / Cols);
        ImGuiListClipper clipper(line_total_count, s.LineHeight);
        const int lines_to_show = clipper.DisplayEnd - clipper.DisplayStart;

        if (ImGui::IsWindowFocused()) {
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))         { CenterAddr -= Cols; }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))  { CenterAddr += Cols; }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))  { CenterAddr -= Cols * lines_to_show; }
            else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) { CenterAddr += Cols * lines_to_show; }
        }

        // Draw vertical separator
        ImVec2 window_pos = ImGui::GetWindowPos();
        if (OptShowAscii)
            draw_list->AddLine(ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y), ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));

        const ImU32 color_text = ImGui::GetColorU32(ImGuiCol_Text);
        const ImU32 color_disabled = OptGreyOutZeroes ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : color_text;

        const char* format_address = "%0*" _PRISizeT "x: ";
        const char* format_byte_space = OptUpperCaseHex ? "%02X " : "%02x ";

        for (uint64_t line_i = 0; line_i < lines_to_show; line_i++) // display only visible lines
        {
            uint32_t addr = (CenterAddr / Cols) * Cols + (uint64_t)((line_i - (lines_to_show >> 1))* Cols);

            ImGui::Text(format_address, s.AddrDigitsCount, base_display_addr + addr);

            // Draw Hexadecimal
            for (int n = 0; n < Cols && addr < mem_size; n++, addr++)
            {
                float byte_pos_x = s.PosHexStart + s.HexCellWidth * n;
                if (OptMidColsCount > 0)
                    byte_pos_x += (float)((float)n / OptMidColsCount) * s.SpacingBetweenMidCols;
                ImGui::SameLine(byte_pos_x);

                {
                    // NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
                    ImU8 b = ReadFn ? ReadFn(addr) : mem_data[addr];

                    if (OptShowHexII)
                    {
                        if ((b >= 32 && b < 128))
                            ImGui::Text(".%c ", b);
                        else if (b == 0xFF && OptGreyOutZeroes)
                            ImGui::TextDisabled("## ");
                        else if (b == 0x00)
                            ImGui::Text("   ");
                        else
                            ImGui::Text(format_byte_space, b);
                    }
                    else
                    {
                        if (b == 0 && OptGreyOutZeroes)
                            ImGui::TextDisabled("00 ");
                        else
                            ImGui::Text(format_byte_space, b);
                    }
                    if (!ReadOnly && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                    {
                        DataEditingTakeFocus = true;
                    }
                }
            }

            if (OptShowAscii)
            {
                // Draw ASCII values
                ImGui::SameLine(s.PosAsciiStart);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                addr = (CenterAddr / Cols) * Cols + (uint64_t)((line_i - (lines_to_show >> 1))* Cols);
                ImGui::PushID(line_i);
                if (ImGui::InvisibleButton("ascii", ImVec2(s.PosAsciiEnd - s.PosAsciiStart, s.LineHeight)))
                {
                    DataEditingAddr = DataPreviewAddr = addr + (uint64_t)((ImGui::GetIO().MousePos.x - pos.x) / s.GlyphWidth);
                    DataEditingTakeFocus = true;
                }
                ImGui::PopID();
                for (int n = 0; n < Cols && addr < mem_size; n++, addr++)
                {
                    if (addr == DataEditingAddr)
                    {
                        draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
                        draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
                    }
                    unsigned char c = ReadFn ? ReadFn(addr) : mem_data[addr];
                    char display_c = (c < 32 || c >= 128) ? '.' : c;
                    draw_list->AddText(pos, (display_c == '.') ? color_disabled : color_text, &display_c, &display_c + 1);
                    pos.x += s.GlyphWidth;
                }
            }
        }
        clipper.End();
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        const bool lock_show_data_preview = OptShowDataPreview;
        if (OptShowOptions)
        {
            ImGui::Separator();
            DrawOptionsLine(s, base_display_addr);
        }

        if (lock_show_data_preview)
        {
            ImGui::Separator();
            DrawPreviewLine(s);
        }

        // Notify the main window of our ideal child content size (FIXME: we are missing an API to get the contents size from the child)
        ImGui::SetCursorPosX(s.WindowWidth);
    }

    void DrawOptionsLine(const Sizes& s, uint64_t base_display_addr)
    {
        IM_UNUSED(mem_data);
        ImGuiStyle& style = ImGui::GetStyle();
        const char* format_range = OptUpperCaseHex ? "Range %08" PRIX64 "..%08" PRIX64 : "Range %08" PRIx64 "..%08" PRIx64;

        // Options menu
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("context");
        if (ImGui::BeginPopup("context"))
        {
            ImGui::PushItemWidth(56);
            if (ImGui::DragInt("##cols", &Cols, 0.2f, 4, 32, "%d cols")) { ContentsWidthChanged = true; if (Cols < 1) Cols = 1; }
            ImGui::PopItemWidth();
            ImGui::Checkbox("Show Data Preview", &OptShowDataPreview);
            ImGui::Checkbox("Show HexII", &OptShowHexII);
            if (ImGui::Checkbox("Show Ascii", &OptShowAscii)) { ContentsWidthChanged = true; }
            ImGui::Checkbox("Grey out zeroes", &OptGreyOutZeroes);
            ImGui::Checkbox("Uppercase Hex", &OptUpperCaseHex);

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text(format_range, base_display_addr, base_display_addr + mem_size - 1);
        ImGui::SameLine();
        ImGui::PushItemWidth((s.AddrDigitsCount + 1) * s.GlyphWidth + style.FramePadding.x * 2.0f);
        if (ImGui::InputText("##addr", AddrInputBuf, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            uint64_t goto_addr;
            if (SSCANF(AddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1)
            {
                GotoAddr = (uint32_t)(goto_addr - base_display_addr);
                HighlightMin = HighlightMax = (uint64_t)-1;
            }
        }
        ImGui::PopItemWidth();

        if (GotoAddr != (uint64_t)-1)
        {
            if (GotoAddr < mem_size)
            {
                ImGui::BeginChild("##scrolling");
                CenterAddr = (uint32_t)GotoAddr;
                ImGui::EndChild();
                DataEditingAddr = DataPreviewAddr = GotoAddr;
                DataEditingTakeFocus = true;
            }
            GotoAddr = (uint64_t)-1;
        }
    }

    void DrawPreviewLine(const Sizes& s)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Preview as:");
        ImGui::SameLine();
        ImGui::PushItemWidth((s.GlyphWidth * 10.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
        if (ImGui::BeginCombo("##combo_type", DataTypeGetDesc(PreviewDataType), ImGuiComboFlags_HeightLargest))
        {
            for (int n = 0; n < ImGuiDataType_COUNT; n++)
                if (ImGui::Selectable(DataTypeGetDesc((ImGuiDataType)n), PreviewDataType == n))
                    PreviewDataType = (ImGuiDataType)n;
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth((s.GlyphWidth * 6.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
        ImGui::Combo("##combo_endianess", &PreviewEndianess, "LE\0BE\0\0");
        ImGui::PopItemWidth();

        char buf[128] = "";
        float x = s.GlyphWidth * 6.0f;
        bool has_value = DataPreviewAddr != (uint64_t)-1;
        if (has_value)
            DrawPreviewData(DataPreviewAddr, PreviewDataType, DataFormat_Dec, buf, (uint64_t)IM_ARRAYSIZE(buf));
        ImGui::Text("Dec"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
        if (has_value)
            DrawPreviewData(DataPreviewAddr, PreviewDataType, DataFormat_Hex, buf, (uint64_t)IM_ARRAYSIZE(buf));
        ImGui::Text("Hex"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
        if (has_value)
            DrawPreviewData(DataPreviewAddr, PreviewDataType, DataFormat_Bin, buf, (uint64_t)IM_ARRAYSIZE(buf));
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        ImGui::Text("Bin"); ImGui::SameLine(x); ImGui::TextUnformatted(has_value ? buf : "N/A");
    }

    // Utilities for Data Preview
    [[nodiscard]] static const char* DataTypeGetDesc(ImGuiDataType data_type)
    {
        const char* descs[] = { "Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double" };
        IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
        return descs[data_type];
    }

    [[nodiscard]] static uint64_t DataTypeGetSize(ImGuiDataType data_type)
    {
        const uint64_t sizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, sizeof(float), sizeof(double) };
        IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
        return sizes[data_type];
    }

    [[nodiscard]] static const char* DataFormatGetDesc(DataFormat data_format)
    {
        const char* descs[] = { "Bin", "Dec", "Hex" };
        IM_ASSERT(data_format >= 0 && data_format < DataFormat_COUNT);
        return descs[data_format];
    }

    static bool IsBigEndian()
    {
        uint16_t x = 1;
        char c[2];
        memcpy(c, &x, 2);
        return c[0] != 0;
    }

    static void* EndianessCopyBigEndian(void* _dst, void* _src, uint64_t s, int is_little_endian)
    {
        if (is_little_endian)
        {
            auto* dst = (uint8_t*)_dst;
            uint8_t* src = (uint8_t*)_src + s - 1;
            for (int i = 0, n = (int)s; i < n; ++i)
                memcpy(dst++, src--, 1);
            return _dst;
        }
        else
        {
            return memcpy(_dst, _src, s);
        }
    }

    static void* EndianessCopyLittleEndian(void* _dst, void* _src, uint64_t s, int is_little_endian)
    {
        if (is_little_endian)
        {
            return memcpy(_dst, _src, s);
        }
        else
        {
            auto* dst = (uint8_t*)_dst;
            uint8_t* src = (uint8_t*)_src + s - 1;
            for (int i = 0, n = (int)s; i < n; ++i)
                memcpy(dst++, src--, 1);
            return _dst;
        }
    }

    void* EndianessCopy(void* dst, void* src, uint64_t size) const
    {
        static void* (*fp)(void*, void*, uint64_t, int) = nullptr;
        if (fp == nullptr)
            fp = IsBigEndian() ? EndianessCopyBigEndian : EndianessCopyLittleEndian;
        return fp(dst, src, size, PreviewEndianess);
    }

    static const char* FormatBinary(const uint8_t* buf, int width)
    {
        IM_ASSERT(width <= 64);
        uint64_t out_n = 0;
        static char out_buf[64 + 8 + 1];
        int n = width / 8;
        for (int j = n - 1; j >= 0; --j)
        {
            for (int i = 0; i < 8; ++i)
                out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
            out_buf[out_n++] = ' ';
        }
        IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
        out_buf[out_n] = 0;
        return out_buf;
    }

    // [Internal]
    void DrawPreviewData(uint64_t addr, ImGuiDataType data_type, DataFormat data_format, char* out_buf, uint64_t out_buf_size) const
    {
        uint8_t buf[8];
        uint64_t elem_size = DataTypeGetSize(data_type);
        uint64_t size = addr + elem_size > mem_size ? mem_size - addr : elem_size;
        if (ReadFn)
            for (int i = 0, n = (int)size; i < n; ++i)
                buf[i] = ReadFn(addr + i);
        else
            memcpy(buf, mem_data + addr, size);

        if (data_format == DataFormat_Bin)
        {
            uint8_t binbuf[8];
            EndianessCopy(binbuf, buf, size);
            ImSnprintf(out_buf, out_buf_size, "%s", FormatBinary(binbuf, (int)size * 8));
            return;
        }

        out_buf[0] = 0;
        switch (data_type)
        {
            case ImGuiDataType_S8:
            {
                int8_t int8 = 0;
                EndianessCopy(&int8, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hhd", int8); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF); return; }
                break;
            }
            case ImGuiDataType_U8:
            {
                uint8_t uint8 = 0;
                EndianessCopy(&uint8, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hhu", uint8); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF); return; }
                break;
            }
            case ImGuiDataType_S16:
            {
                int16_t int16 = 0;
                EndianessCopy(&int16, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hd", int16); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF); return; }
                break;
            }
            case ImGuiDataType_U16:
            {
                uint16_t uint16 = 0;
                EndianessCopy(&uint16, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%hu", uint16); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF); return; }
                break;
            }
            case ImGuiDataType_S32:
            {
                int32_t int32 = 0;
                EndianessCopy(&int32, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%d", int32); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%08x", int32); return; }
                break;
            }
            case ImGuiDataType_U32:
            {
                uint32_t uint32 = 0;
                EndianessCopy(&uint32, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%u", uint32); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32); return; }
                break;
            }
            case ImGuiDataType_S64:
            {
                int64_t int64 = 0;
                EndianessCopy(&int64, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64); return; }
                break;
            }
            case ImGuiDataType_U64:
            {
                uint64_t uint64 = 0;
                EndianessCopy(&uint64, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64); return; }
                break;
            }
            case ImGuiDataType_Float:
            {
                float float32 = 0.0f;
                EndianessCopy(&float32, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%f", float32); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "%a", float32); return; }
                break;
            }
            case ImGuiDataType_Double:
            {
                double float64 = 0.0;
                EndianessCopy(&float64, buf, size);
                if (data_format == DataFormat_Dec) { ImSnprintf(out_buf, out_buf_size, "%f", float64); return; }
                if (data_format == DataFormat_Hex) { ImSnprintf(out_buf, out_buf_size, "%a", float64); return; }
                break;
            }
            case ImGuiDataType_COUNT:
            default:
                break;
        } // Switch
        IM_ASSERT(0); // Shouldn't reach
    }
};

#undef _PRISizeT
#undef ImSnprintf


#endif //GC__MEMORY_VIEWER_H
