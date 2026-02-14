#pragma once
#include <windows.h>

struct MicEndpoint
{
    bool Init();
    void Shutdown();

    bool GetMute(bool& muted) const;
    bool ToggleMute(bool& newMuted);

private:
    bool ReacquireEndpoint();

    struct IAudioEndpointVolume* m_epVol = nullptr;
    bool m_comInitedByUs = false;
};
