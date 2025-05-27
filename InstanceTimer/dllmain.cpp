#include <Windows.h>
#include <Shlwapi.h>
#include <thread>
#include <filesystem>

// Make sure the delay load headers come BEFORE including any GWCA headers
#include <delayimp.h>

#pragma comment(lib, "delayimp.lib")
#pragma comment(lib, "./GWCA/lib/gwca.lib")

// IMPORTANT: Only include GWCA headers AFTER the delay load directives
// This ensures that any functions imported from these headers will be delay-loaded
#include <GWCA/GWCA.h>
#include <GWCA/stdafx.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/Managers/MapMgr.h>

#include <GWCA/Utilities/Hook.h>
#include <GWCA/Utilities/Hooker.h>

namespace {
    HMODULE g_hModule = nullptr;

    enum fontStyle {
        smallFont = 0x8,
        mediumFont,
        largeFont
    };

    fontStyle currentFontStyle = fontStyle::mediumFont;

    constexpr uint32_t instance_timer_child_frame_id = 0xffd;

    GW::UI::UIInteractionCallback OnInstanceTimerWindow_UICallback_Func = 0, OnInstanceTimerWindow_UICallback_Ret = 0;
    void OnInstanceTimerWindow_UICallback(GW::UI::InteractionMessage* message, void* wParam, void* lParam) {
        GW::Hook::EnterHook();
        const auto frame = GW::UI::GetFrameById(message->frame_id);
        const auto clock = GW::UI::GetFrameByLabel(L"StClock"); // To be removed if creating new UI component
        //if (frame && frame->child_offset_id == instance_timer_child_frame_id) {
        if (frame && frame->child_offset_id == clock->child_offset_id) {
            switch ((uint32_t)message->message_id) {
            case 0x37:
            case 0x3d: {

                const int instance_time = GW::Map::GetInstanceTime();
                const auto duration = std::chrono::milliseconds(instance_time);

                const auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
                const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
                const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));
                const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration % std::chrono::seconds(1));

                const auto timer = std::format(L"\x108\x107{:01}:{:02}:{:02}.{:01}\x01",
                    hours.count(),
                    minutes.count(),
                    seconds.count(),
                    milliseconds.count() / 100);

                GW::UI::SendFrameUIMessage(GW::UI::GetChildFrame(frame, 0), (GW::UI::UIMessage)0x4c, (void*)timer.c_str(), 0);
                GW::Hook::LeaveHook();
                return;
            } break;
            }
        }
        OnInstanceTimerWindow_UICallback_Ret(message, wParam, lParam);
        GW::Hook::LeaveHook();
    }

    void SetFontStyle(GW::UI::Frame* frame, uint32_t style)
    {
        if (frame && frame->field93_0x18c != style) {
            GW::GameThread::Enqueue([frame, style]() {
                frame->field93_0x18c = style;
                GW::UI::SendFrameUIMessage(frame, (GW::UI::UIMessage)0x35, 0);
                });
        }
    }

    // This hook gets called for all delay load notifications (success and failure)
    FARPROC WINAPI DelayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
    {
        // We only care about the pre-load notification
        if (!(dliNotify == dliNotePreLoadLibrary && strcmp(pdli->szDll, "gwca.dll") == 0))
            return NULL;
        // Get the path to our current DLL
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(g_hModule, path, MAX_PATH);

        // Build the path to GWCA.dll relative to our DLL
        std::filesystem::path fs_path = path;
        std::filesystem::path gwca_path = fs_path.parent_path() / L"gwca.dll";

        return (FARPROC)LoadLibraryW(gwca_path.c_str());
    }

    GW::HookEntry ChatCmdHook;
    GW::HookEntry OnCreateUIComponent_Entry;

    void OnChatCmd(GW::HookStatus*, const wchar_t* cmd, int argc, const LPWSTR* argv) {
        if (wcscmp(argv[0], L"font") == 0) {
            if (argc < 2 || wcscmp(argv[0], L"font") != 0) {
                GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Usage: /font [small|medium|large]");
                return;
            }

            if (wcscmp(argv[1], L"small") == 0) {
                currentFontStyle = fontStyle::smallFont;
            }
            else if (wcscmp(argv[1], L"medium") == 0) {
                currentFontStyle = fontStyle::mediumFont;
            }
            else if (wcscmp(argv[1], L"large") == 0) {
                currentFontStyle = fontStyle::largeFont;
            }
            else {
                GW::Chat::WriteChat(GW::Chat::CHANNEL_MODERATOR, L"Invalid font size. Use: small, medium, or large.");
                return;
            }

            auto frame = GW::UI::GetFrameByLabel(L"StClock");
            auto child = GW::UI::GetChildFrame(frame, 0);
            SetFontStyle(child, currentFontStyle);
        }
    }

    GW::UI::Frame* CreateInstanceTimerFrame() {
        auto frame = GW::UI::GetFrameByLabel(L"StClock");
        if (!(frame && frame->frame_callbacks.size()))
            return nullptr;

        if (!OnInstanceTimerWindow_UICallback_Func) {
            OnInstanceTimerWindow_UICallback_Func = frame->frame_callbacks[0].callback;
            GW::Hook::CreateHook((void**)&OnInstanceTimerWindow_UICallback_Func, OnInstanceTimerWindow_UICallback, (void**)&OnInstanceTimerWindow_UICallback_Ret);
            GW::Hook::EnableHooks(OnInstanceTimerWindow_UICallback_Func);
        }
        GW::UI::SetFrameVisible(frame, true);
        if (!GW::UI::GetPreference(GW::UI::NumberPreference::ClockMode)) {
            GW::UI::SetPreference(GW::UI::NumberPreference::ClockMode, 0x1);
        }
        GW::UI::SetWindowVisible(GW::UI::WindowID::WindowID_InGameClock, 0x1);

        //const auto instance_timer_frame = GW::UI::CreateUIComponent(GW::UI::GetParentFrame(clock)->frame_id, 0x800, 0xffd, clock->frame_callbacks[0].callback, nullptr, L"InstanceTimer");

        SetFontStyle(GW::UI::GetChildFrame(frame, 0), currentFontStyle);

        return frame; // GW::UI::GetFrameById(instance_timer_frame);
    }

    void OnPostUIMessage(GW::HookStatus* status, GW::UI::UIMessage message_id, void* wParam, void* lParam) {
        CreateInstanceTimerFrame();
    }

    void Init(HMODULE hModule) {
        if (!GW::Initialize()) {
            MessageBoxW(nullptr, L"Failed to initialize GWCA.", L"Error", MB_ICONERROR);
            return;
        }
        const GW::UI::UIMessage ui_messages[] = {
            GW::UI::UIMessage::kMapLoaded,
            GW::UI::UIMessage::kPreferenceValueChanged,
            GW::UI::UIMessage::kUIPositionChanged
        };
        for (auto message : ui_messages) {
            GW::UI::RegisterUIMessageCallback(&ChatCmdHook, message, OnPostUIMessage, 0x800);
        }
        GW::Chat::CreateCommand(&ChatCmdHook, L"font", OnChatCmd);
        GW::GameThread::Enqueue(CreateInstanceTimerFrame);
    }
}

const PfnDliHook __pfnDliNotifyHook2 = ::DelayLoadHook;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    g_hModule = hModule;
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread([hModule]() {
            Init(hModule);
            }).detach();
    }
    return TRUE;
}