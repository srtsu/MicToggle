#pragma once
#include <windows.h>

struct IMMDeviceEnumerator;

struct MicEndpoint
{
    bool Init();
    void Shutdown();

    bool GetMute(bool& muted) const;
    bool ToggleMute(bool& newMuted);

private:
    bool EnsureDefaultCurrent();
    bool ActivateDefault(IMMDeviceEnumerator* e);

    struct IAudioEndpointVolume* m_epVol = nullptr;
    wchar_t* m_devId = nullptr;
    bool m_comInitedByUs = false;
};
