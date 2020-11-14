#pragma once

#include "../interface.h"

#include "imgui/imgui.h"
#include "default.h"

#include <memory>
#include <list>
#include <cctype>

#define SANITIZE(string) ((string != NULL) ? string : "")
#define MAX_ARGS 5

typedef struct s_console_command {
    const char* command;
    const char* description;
    CONSOLE_COMMAND((*callback));
} s_console_command;


struct ConsoleWidget
{
    char                         InputBuf[256]{};
    char                         OutputBuf[MAX_OUTPUT_LENGTH];
    ImVector<char*>              Items;
    std::list<s_console_command> Commands;
    ImVector<char*>              History;
    int                          HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter              Filter;
    bool                         AutoScroll;
    bool                         ScrollToBottom;

    ConsoleWidget()
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        this->AddCommand(
                s_console_command {
                        .command = "HELP",
                        .description = "Get a list of commands",
                        .callback = NULL
                }
        );

        this->AddCommand(
                s_console_command {
                        .command = "CLEAR",
                        .description = "Clear the console",
                        .callback = NULL
                }
        );

        this->AddCommand(
                s_console_command {
                        .command = "HISTORY",
                        .description = "Get a history of commands sent",
                        .callback = NULL
                }
        );
        AutoScroll = true;
        ScrollToBottom = false;
    }
    ~ConsoleWidget()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    // Portable helpers
    static void  ToLower(char* string)                           { for ( ; *string; ++string) *string = tolower(*string); }
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void AddCommand(s_console_command command) {
        Commands.push_back(command);
    }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void    Draw(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Debugger Console", p_open))
        {
            ImGui::End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console"))
                *p_open = false;
            ImGui::EndPopup();
        }

        ImGui::TextWrapped(
                "Debugger console.");
        ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

        if (ImGui::SmallButton("Clear"))           { ClearLog(); } ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");

        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!Filter.PassFilter(item))
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if (strncmp(item, "[error]", 7) == 0)          { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
            else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();

        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            char* s = InputBuf;
            Strtrim(s);
            ToLower(s);
            if (s[0]) {
                ExecCommand(s);
            }
            else if (History.Size) {
                STRCPY(InputBuf, sizeof(InputBuf), History[History.Size - 1]);
                ExecCommand(s);
            }
            STRCPY(s, sizeof(InputBuf), "");
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        ImGui::End();
    }

    void Help() {
        AddLog("Commands:");
        std::list<s_console_command> :: iterator command_iter;
        for (command_iter = this->Commands.begin(); command_iter != this->Commands.end(); ++command_iter) {
            AddLog("- %s: %s", SANITIZE(command_iter->command), SANITIZE(command_iter->description));
        }
    }

    void Clear() {
        ClearLog();
    }

    void GetHistory() {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
            AddLog("%3d: %s\n", i, History[i]);
    }

    static int SplitCommand(char* command, char** dest) {
        /* assume target is a char[>MAX_ARGS + 1] */
        char* token = strtok(command, " ");
        int i;
        for (i = 0; i < MAX_ARGS && token != nullptr; i++) {
            dest[i] = token;

            token = strtok(nullptr, " ");
        }
        dest[i + 1] = nullptr;

        return i;
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        if (Stricmp(command_line, "CLEAR") == 0)
        {
            this->Clear();
            ScrollToBottom = true;
            return;
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            this->Help();
            ScrollToBottom = true;
            return;
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            this->GetHistory();
            ScrollToBottom = true;
            return;
        }
        else if (Stricmp(command_line, "ENCOURAGE") == 0)
        {
            AddLog("You can do it!");
            ScrollToBottom = true;
            return;
        }

        char command[strlen(command_line) + 1];
        STRCPY(command, sizeof(command), command_line);

        char* args[MAX_ARGS + 1];
        int argc = ConsoleWidget::SplitCommand(command, args);

        for (auto& cmd : this->Commands) {
            if (Stricmp(args[0], cmd.command) == 0) {

                cmd.callback(args, argc, this->OutputBuf);
                if (this->OutputBuf[0]) {
                    AddLog("%s", this->OutputBuf);
                }
                this->OutputBuf[0] = 0;

                // On command input, we scroll to bottom even if AutoScroll==false
                ScrollToBottom = true;
                return;
            }
        }

        AddLog("Unknown command: '%s'\n", command_line);
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        auto console = (ConsoleWidget*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
            case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                std::list<s_console_command> :: iterator command_iter;
                for (command_iter = this->Commands.begin(); command_iter != this->Commands.end(); ++command_iter) {
                    if (Strnicmp(command_iter->command, word_start, (int)(word_end - word_start)) == 0)
                        candidates.push_back(command_iter->command);
                }

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
            case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};
