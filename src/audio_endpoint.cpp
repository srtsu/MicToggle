#include "audio_endpoint.h"
#include <endpointvolume.h>
#include <wchar.h>

#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib")
#endif

static const GUID kMicToggleContext = { 0x6a6840e3, 0x5a86, 0x4e9a, { 0x9d, 0x16, 0x85, 0xb8, 0x7f, 0x1f, 0xa2, 0x53 } };

MicEndpoint::NotificationClient::NotificationClient(volatile LONG* dirty)
    : m_dirty(dirty)
{
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::QueryInterface(
    REFIID riid, void** object)
{
    if (!object) return E_POINTER;
    *object = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, __uuidof(IMMNotificationClient)))
    {
        *object = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE MicEndpoint::NotificationClient::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refs));
}

ULONG STDMETHODCALLTYPE MicEndpoint::NotificationClient::Release()
{
    return static_cast<ULONG>(InterlockedDecrement(&m_refs));
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::OnDefaultDeviceChanged(
    EDataFlow flow, ERole role, LPCWSTR)
{
    if (flow == eCapture && role == eConsole)
        InterlockedExchange(m_dirty, 1);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::OnDeviceAdded(LPCWSTR)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::OnDeviceRemoved(LPCWSTR)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::OnDeviceStateChanged(
    LPCWSTR, DWORD)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MicEndpoint::NotificationClient::OnPropertyValueChanged(
    LPCWSTR, const PROPERTYKEY)
{
    return S_OK;
}

MicEndpoint::MicEndpoint() : m_notifications(&m_defaultDirty)
{
}

MicEndpoint::~MicEndpoint()
{
    Shutdown();
}

bool MicEndpoint::ActivateDefault()
{
    if (!m_enumerator) return false;

    IMMDevice* dev = nullptr;
    HRESULT hr = m_enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    if (FAILED(hr) || !dev) return false;

    LPWSTR id = nullptr;
    hr = dev->GetId(&id);
    if (FAILED(hr) || !id)
    {
        dev->Release();
        return false;
    }

    IAudioEndpointVolume* v = nullptr;
    hr = dev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr,
                       reinterpret_cast<void**>(&v));

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
    if (!m_enumerator) return false;

    if (m_notificationsRegistered)
    {
        const bool refresh =
            !m_epVol || InterlockedExchange(&m_defaultDirty, 0) != 0;
        if (!refresh) return true;

        if (ActivateDefault()) return true;
        InterlockedExchange(&m_defaultDirty, 1);
        return false;
    }

    IMMDevice* dev = nullptr;
    HRESULT hr = m_enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &dev);
    if (FAILED(hr) || !dev)
        return false;

    LPWSTR id = nullptr;
    hr = dev->GetId(&id);
    dev->Release();
    if (FAILED(hr) || !id)
        return false;

    const bool changed = (!m_devId || wcscmp(m_devId, id) != 0);
    CoTaskMemFree(id);

    if (changed || !m_epVol)
        return ActivateDefault();
    return true;
}

bool MicEndpoint::RebindDefault()
{
    InterlockedExchange(&m_defaultDirty, 0);
    if (ActivateDefault()) return true;
    InterlockedExchange(&m_defaultDirty, 1);
    return false;
}

bool MicEndpoint::Init()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) m_comInitedByUs = true;
    else if (hr != RPC_E_CHANGED_MODE)
        return false;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void**>(&m_enumerator));
    if (FAILED(hr) || !m_enumerator)
    {
        Shutdown();
        return false;
    }

    hr = m_enumerator->RegisterEndpointNotificationCallback(&m_notifications);
    if (SUCCEEDED(hr))
    {
        m_notificationsRegistered = true;
    }

    if (!RebindDefault())
    {
        Shutdown();
        return false;
    }
    return true;
}

void MicEndpoint::Shutdown()
{
    if (m_notificationsRegistered && m_enumerator)
        m_enumerator->UnregisterEndpointNotificationCallback(&m_notifications);
    m_notificationsRegistered = false;

    if (m_epVol) { m_epVol->Release(); m_epVol = nullptr; }
    if (m_devId) { CoTaskMemFree(m_devId); m_devId = nullptr; }
    if (m_enumerator) { m_enumerator->Release(); m_enumerator = nullptr; }
    if (m_comInitedByUs) CoUninitialize();
    m_comInitedByUs = false;
    InterlockedExchange(&m_defaultDirty, 1);
}

bool MicEndpoint::GetMute(bool& muted)
{
    muted = false;
    if (!EnsureDefaultCurrent()) return false;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        BOOL current = FALSE;
        if (SUCCEEDED(m_epVol->GetMute(&current)))
        {
            muted = (current != FALSE);
            return true;
        }
        if (attempt == 0 && RebindDefault()) continue;
        return false;
    }
    return false;
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
            if (attempt == 0 && RebindDefault()) continue;
            return false;
        }

        const BOOL target = muted ? FALSE : TRUE;
        if (FAILED(m_epVol->SetMute(target, &kMicToggleContext)))
        {
            if (attempt == 0 && RebindDefault()) continue;
            return false;
        }

        newMuted = (target != FALSE);
        return true;
    }
    return false;
}
