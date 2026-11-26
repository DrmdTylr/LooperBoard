# The Looper Board Archetype v0.1.0

**Tag:** `v0.1.0`  
**Release type:** Pre-release  
**Asset:** `The-Looper-Board-Archetype-Setup-0.1.0-x64.exe`

## Release notes

First Windows x64 pre-release of The Looper Board Archetype (Vintage Deck), a standalone looper and effects board.

- Record and control an eight-layer looper with a tuner and adjustable audio controls.
- Arrange pedalboard effects and host up to 16 compatible 64-bit mono or stereo VST3 effects.
- Save and reload loop projects and pedalboard presets.
- Choose from multiple board layouts and configure audio devices, including ASIO.

### Installation

Run the installer and follow the prompts. If Microsoft Edge WebView2 Runtime is missing, the installer downloads it from Microsoft; an internet connection is required. Install audio-interface drivers and any desired VST3 plugins separately. The installer is unsigned and Windows SmartScreen may show a warning.

This is a pre-release and has not been validated with every audio interface or MIDI device. See the README for setup details and known limitations.

## Build and publish

### 1. Check release prerequisites

Before creating a GitHub release, inspect the tracked files under `build/Looper_artefacts/Release/Looper.exe.WebView2/`. This repository currently tracks 187 WebView2 profile/cache files there. GitHub-generated source archives include tracked files, so do not publish until these files have been reviewed and the profile data has been safely removed from the release commit. If any data was already pushed, removing it from the latest commit alone does not remove it from repository history.

Review the licensing and corresponding-source requirements before distributing the binary. The README documents the ASIO SDK's GPLv3 option; the repository currently has no root `LICENSE` file.

### 2. Build and test a clean x64 release

Use a Windows machine with Visual Studio C++ desktop tools, the Windows SDK, CMake, Git, and NSIS installed. In PowerShell, from the repository root:

```powershell
cmake -S . -B release-build -A x64 '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>'
cmake --build release-build --config Release
ctest --test-dir release-build -C Release --output-on-failure
```

### 3. Package and verify the installer

Download Microsoft's WebView2 bootstrapper, verify its Authenticode signature, and compile the installer:

```powershell
Invoke-WebRequest 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' -OutFile MicrosoftEdgeWebview2Setup.exe
$signature = Get-AuthenticodeSignature .\MicrosoftEdgeWebview2Setup.exe
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') { throw 'Invalid WebView2 bootstrapper signature' }
makensis "/DAPP_EXE=$PWD\release-build\Looper_artefacts\Release\The Looper Board Archetype.exe" "/DWEBVIEW_BOOTSTRAPPER=$PWD\MicrosoftEdgeWebview2Setup.exe" "/DASIO_LICENSE=$PWD\release-build\_deps\asio_sdk-src\LICENSE.txt" "/DJUCE_LICENSE=$PWD\release-build\_deps\juce-src\LICENSE.md" "/DOUTPUT_FILE=$PWD\The-Looper-Board-Archetype-Setup-0.1.0-x64.exe" installer\Looper.nsi
```

Test the generated installer on a separate Windows account or machine before publishing.

### 4. Create the GitHub release

After the prerequisites above are resolved:

1. Open [New release](https://github.com/DrmdTylr/LooperBoard/releases/new).
2. Select **Choose a tag**, enter `v0.1.0`, and create the tag on the reviewed `main` commit.
3. Set the title to **The Looper Board Archetype v0.1.0**.
4. Paste the **Release notes** section above into the release description.
5. Upload `The-Looper-Board-Archetype-Setup-0.1.0-x64.exe`.
6. Select **Set as a pre-release**. Leave **Set as the latest release** unchecked if GitHub shows that option.
7. Review the tag, description, and uploaded asset, then publish the release.
