#include "wav_resource.h"
#include "resource.h"
#include <mmsystem.h>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

static const BYTE* LoadWavResource(HINSTANCE hInst, int id)
{
    HRSRC r = FindResourceW(hInst, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!r) return nullptr;

    HGLOBAL hg = LoadResource(hInst, r);
    if (!hg) return nullptr;

    const DWORD size = SizeofResource(hInst, r);
    if (!size) return nullptr;

    return static_cast<const BYTE*>(LockResource(hg));
}

static void PlayWavMemAsync(const BYTE* mem)
{
    if (!mem) return;
    PlaySoundW(reinterpret_cast<LPCWSTR>(mem), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

bool ResourceSounds::Init(HINSTANCE hInst)
{
    m_wavMute   = LoadWavResource(hInst, IDR_WAV_MUTE);
    m_wavUnmute = LoadWavResource(hInst, IDR_WAV_UNMUTE);
    return m_wavMute && m_wavUnmute;
}

void ResourceSounds::PlayMuted() const   { PlayWavMemAsync(m_wavMute); }
void ResourceSounds::PlayUnmuted() const { PlayWavMemAsync(m_wavUnmute); }
void ResourceSounds::PlayError() const { MessageBeep(MB_ICONHAND); }
