#include "audio_endpoint.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#pragma comment(lib, "ole32.lib")

static const GUID kMicToggleContext = { 0x6a6840e3, 0x5a86, 0x4e9a, { 0x9d, 0x16, 0x85, 0xb8, 0x7f, 0x1f, 0xa2, 0x53 } };

bool MicEndpoint::ReacquireEndpoint()
{
    if (m_epVol) { m_epVol->Release(); m_epVol = nullptr; }

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

    IAudioEndpointVolume* v = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (void**)&v);

    dev->Release();
    e->Release();

    if (FAILED(hr) || !v) return false;
    m_epVol = v;
    return true;
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

    if (!ReacquireEndpoint())
    {
        Shutdown();
        return false;
    }
    return true;
}

void MicEndpoint::Shutdown()
{
    if (m_epVol) { m_epVol->Release(); m_epVol = nullptr; }
    if (m_comInitedByUs) CoUninitialize();
    m_comInitedByUs = false;
}

bool MicEndpoint::GetMute(bool& muted) const
{
    muted = false;
    if (!m_epVol) return const_cast<MicEndpoint*>(this)->ReacquireEndpoint() ? const_cast<MicEndpoint*>(this)->GetMute(muted) : false;
    BOOL m = FALSE;
    if (FAILED(m_epVol->GetMute(&m)))
    {
        if (!const_cast<MicEndpoint*>(this)->ReacquireEndpoint()) return false;
        if (FAILED(m_epVol->GetMute(&m))) return false;
    }
    muted = (m != FALSE);
    return true;
}

bool MicEndpoint::ToggleMute(bool& newMuted)
{
    newMuted = false;
    if (!m_epVol && !ReacquireEndpoint()) return false;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        BOOL muted = FALSE;
        if (FAILED(m_epVol->GetMute(&muted)))
        {
            if (attempt == 0 && ReacquireEndpoint()) continue;
            return false;
        }

        const BOOL target = muted ? FALSE : TRUE;
        if (FAILED(m_epVol->SetMute(target, &kMicToggleContext)))
        {
            if (attempt == 0 && ReacquireEndpoint()) continue;
            return false;
        }

        BOOL after = target;
        if (FAILED(m_epVol->GetMute(&after))) after = target;
        newMuted = (after != FALSE);
        return true;
    }
    return false;
}
