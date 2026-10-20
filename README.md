# Atlas Tune Pro  (VST3, Windows, FL Studio)

## Build (once)
1. Install **Visual Studio 2022 Community** (workload: "Desktop development with C++") and **CMake**.
2. Double-click `build.bat` (first run downloads JUCE 7.0.12 automatically, needs internet).
3. Copy `build\AtlasTunePro_artefacts\Release\VST3\Atlas Tune Pro.vst3` to `C:\Program Files\Common Files\VST3\`.
4. FL Studio: Options > Manage plugins > Find plugins (or "Verify plugins"), then add *Atlas Tune Pro* on a mixer insert.

## Quick use
* Pick **Key** (chips) + **Scale**; click piano keys to switch single notes on/off (dots = allowed notes).
* **Retune Speed**: 0 ms = hard "T-Pain" effect, 15-40 ms = tight modern, 80+ ms = natural.
* **Low Latency** (Mode panel): reports ~20 ms at 48 kHz instead of ~38 ms, lowest tracked note becomes ~150 Hz instead of ~80 Hz.
* **Harmony Engine**: switch ON, set interval (3rd/4th/5th/6th/7th/octave, up or down - in the chosen scale),
  voice type (Soprano/Alto/Tenor/Bass = formant character) and level for up to 4 voices.
* MIDI: if you route MIDI notes to the plugin, the notes you hold become the only allowed target notes.
* FL tip: for recording use the Low Latency mode and enable plugin delay compensation (the plugin reports its latency).

## Build on GitHub (no local setup)
1. Create a new GitHub repo and upload this whole folder (including the `.github` folder).
2. Open the **Actions** tab -> "Build Atlas Tune Pro" runs automatically on every push (or press **Run workflow**).
3. When it is green, open the run -> **Artifacts** -> download `AtlasTunePro-Windows-VST3`.
4. Unzip and copy `Atlas Tune Pro.vst3` to `C:\Program Files\Common Files\VST3\`.
Tip: push a tag like `v1.0.0` and the zip also appears under **Releases**.
