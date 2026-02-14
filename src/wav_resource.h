#pragma once
#include <windows.h>

struct ResourceSounds
{
    bool Init(HINSTANCE hInst);
    void PlayMuted() const;
    void PlayUnmuted() const;
    void PlayError() const;

private:
    const BYTE* m_wavMute = nullptr;
    const BYTE* m_wavUnmute = nullptr;
};
