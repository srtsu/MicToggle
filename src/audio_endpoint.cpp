#include "audio_endpoint.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <wchar.h>

#pragma comment(lib, "ole32.lib")

static const GUID kMicToggleContext = { 0x6a6840e3, 0x5a86, 0x4e9a, { 0x9d, 0x16, 0x85, 0xb8, 0x7f, 0x1f, 0xa2, 0x53 } };

bool MicEndpoint::ActivateDefault(IMMDeviceEnumerator* e)
{
    IMMDevice* dev = nullptr;
    HRESULT hr = e->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    if (FAILED(hr) || !dev) return false;

    LPWSTR id = nullptr;
    hr = dev->GetId(&id);
    if (FAILED(hr) || !id)
    {
        dev->Release();
        return false;
    }

    IAudioEndpointVolume* v = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (void**)&v);

    dev->Release();

    if (FAILED(hr) || !v)
    {
        CoTaskMemFree(id);
        return false;
    }

    if (m_epVol) { m_epVol->Release(); m_epVol = nullptr; }
    if (m_devId) { CoTaskMemFree(m_devId); m_devId = nullptr; }

    m_epVol = v;
    m_devId = id;
    return true;
}

bool MicEndpoint::EnsureDefaultCurrent()
{
    IMMDeviceEnumerator* e = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                                  __uuidof(IMMDeviceEnumerator), (void**)&e);
    if (FAILED(hr) || !e) return false;

    IMMDevice* dev = nullptr;
    hr = e->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    if (FAILED(hr) || !dev)
    {
        e->Release();
        return false;
    }

    LPWSTR id = nullptr;
    hr = dev->GetId(&id);
    dev->Release();
    if (FAILED(hr) || !id)
    {
        e->Release();
        return false;
    }

    const bool changed = (!m_devId || wcscmp(m_devId, id) != 0);
    CoTaskMemFree(id);

    bool ok = true;
    if (changed || !m_epVol)
        ok = ActivateDefault(e);

    e->Release();
    return ok;
}

bool MicEndpoint::Init()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) m_comInitedByUs = true;
    else if (hr == RPC_E_CHANGED_MODE)
    {
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr)) m_comInitedByUs = true;
        else m_comInitedByUs = false;
    }
    else return false;

    if (!EnsureDefaultCurrent())
    {
        Shutdown();
        return false;
    }
    return true;
}

void MicEndpoint::Shutdown()
{
    if (m_epVol) { m_epVol->Release(); m_epVol = nullptr; }
    if (m_devId) { CoTaskMemFree(m_devId); m_devId = nullptr; }
    if (m_comInitedByUs) CoUninitialize();
    m_comInitedByUs = false;
}

bool MicEndpoint::GetMute(bool& muted) const
{
    muted = false;
    if (!const_cast<MicEndpoint*>(this)->EnsureDefaultCurrent()) return false;
    BOOL m = FALSE;
    if (FAILED(m_epVol->GetMute(&m)))
    {
        if (!const_cast<MicEndpoint*>(this)->EnsureDefaultCurrent()) return false;
        if (FAILED(m_epVol->GetMute(&m))) return false;
    }
    muted = (m != FALSE);
    return true;
}

bool MicEndpoint::ToggleMute(bool& newMuted)
{
    newMuted = false;
    if (!EnsureDefaultCurrent()) return false;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        BOOL muted = FALSE;
        if (FAILED(m_epVol->GetMute(&muted)))
        {
            if (attempt == 0 && EnsureDefaultCurrent()) continue;
            return false;
        }

        const BOOL target = muted ? FALSE : TRUE;
        if (FAILED(m_epVol->SetMute(target, &kMicToggleContext)))
        {
            if (attempt == 0 && EnsureDefaultCurrent()) continue;
            return false;
        }

        BOOL after = target;
        if (FAILED(m_epVol->GetMute(&after))) after = target;
        newMuted = (after != FALSE);
        return true;
    }
    return false;
}
