#pragma once
#include <windows.h>
#include <mmdeviceapi.h>

struct IAudioEndpointVolume;

struct MicEndpoint
{
    MicEndpoint();
    ~MicEndpoint();

    MicEndpoint(const MicEndpoint&) = delete;
    MicEndpoint& operator=(const MicEndpoint&) = delete;

    bool Init();
    void Shutdown();

    bool GetMute(bool& muted);
    bool ToggleMute(bool& newMuted);

private:
    bool EnsureDefaultCurrent();
    bool ActivateDefault();
    bool RebindDefault();

    class NotificationClient final : public IMMNotificationClient
    {
    public:
        explicit NotificationClient(volatile LONG* dirty);

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
            EDataFlow flow, ERole role, LPCWSTR deviceId) override;
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR deviceId) override;
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR deviceId) override;
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
            LPCWSTR deviceId, DWORD newState) override;
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
            LPCWSTR deviceId, const PROPERTYKEY key) override;

    private:
        volatile LONG m_refs = 1;
        volatile LONG* m_dirty;
    };

    volatile LONG m_defaultDirty = 1;
    NotificationClient m_notifications;
    IMMDeviceEnumerator* m_enumerator = nullptr;
    IAudioEndpointVolume* m_epVol = nullptr;
    wchar_t* m_devId = nullptr;
    bool m_notificationsRegistered = false;
    bool m_comInitedByUs = false;
};
