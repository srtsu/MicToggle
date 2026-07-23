# MicToggle

**MicToggle is an ultra-lightweight Windows utility** that toggles the **default microphone mute** and plays **different sounds** for mute/unmute.

Designed to be:
- **Tiny** (no frameworks, no tray UI, no background services)
- **Fast** (event-driven, idle = almost no work)
- **Reliable** (toggles the real system mic mute state via Core Audio)

✅ Uses Windows Core Audio (`IAudioEndpointVolume`) to toggle the actual system mute state  
✅ Plays **embedded WAV** sounds from resources (no disk I/O at runtime)  
✅ Hotkey: `VK_LAUNCH_MEDIA_SELECT` (Media key). If `RegisterHotKey` fails, falls back to a low-level keyboard hook.

---

## Why it’s lightweight

- No .NET / Electron / Python
- No background polling loop
- No external dependencies
- Uses WinAPI + Core Audio only

---

## Build

### Windows (MSVC)

~~~powershell
cmake -S . -B build -A x64
cmake --build build --config Release
~~~

To embed a specific version into the EXE properties:

~~~powershell
cmake -S . -B build -A x64 -DMIC_TOGGLE_VERSION=1.2.3
cmake --build build --config Release
~~~

GitHub Actions sets this value automatically from release tags such as `v1.2.3`.

### macOS → Windows cross-build (mingw-w64)

Install toolchain:

~~~bash
brew install mingw-w64 cmake
~~~

Build:

~~~bash
cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake
cmake --build build-win -j
~~~

Output:

~~~
build-win/mic_toggle.exe
~~~

---

## Autostart (recommended)

MicToggle is meant to run as a normal user process (not a Windows Service).

- **Task Scheduler** (recommended): create a task “At log on” → run `mic_toggle.exe`
- or Win+R → `shell:startup` → place a shortcut to the exe

---

## Replace sounds

Replace:
- `res/mute.wav`
- `res/unmute.wav`

Keep them as standard **WAV (RIFF/WAVE)** files. Short sounds are recommended.
