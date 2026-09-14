# winage 2.0 release checklist

Branch: `age-0.12-upgrade` (3 commits, not yet merged to `main`).

## Already verified

- `cargo test --release` — 24/24 pass, covering passphrase, armored, identity-file, recipients-list and encrypted-identity round trips through the real FFI entry points, plus four mixed-script cases the old build could not have expressed.
- `msbuild age.sln /t:age /p:Configuration=Release /p:Platform=x64` and the same for `Debug` — both clean under Visual Studio 2026 and the v145 toolset, `age.exe` reports FileVersion 2.0.0.0.
- `age.exe` imports no ANSI Win32 path entry point. Checked with `dumpbin /imports`: `CreateFileA`, `GetWindowTextA`, `SetWindowTextA`, `PathFileExistsA`, `GetOpenFileNameA`, `GetSaveFileNameA`, `FindFirstFileA` and `GetEnvironmentVariableA` are all absent, and the W forms are present.
- Installer builds locally and lands at `winage\ageSetup\Release\ageSetup.msi`, the path the workflow expects. ProductVersion 2.0.0, new ProductCode, UpgradeCode unchanged.
- Upgrade from an installed 1.0 completes with a single Add/Remove Programs entry.

## Manual smoke tests

Everything here is C++ or shell integration, which the Rust tests cannot reach.

The Unicode port rewrote the string handling under every one of these, so the boxes are cleared: a pass on the MBCS build says nothing about this one. Build a fresh MSI and install it before starting, or the binary you are testing is the old one.

Start with the two that would catch a bad port fastest:

- [x] Double-click any `.age` file. This is the shell-integration path and the one that would break outright if `__wargv` were not populated. If the main window comes up with the file already filled in, argument handling survived.
- [x] Encrypt a file whose name is pure ASCII, using a passphrase, and decrypt it again. The plain path has to keep working before anything exotic matters.

Then the cases the port exists for. None of these could work before:

- [x] A file whose name mixes scripts, say `日本語-Ελληνικά-Русский.txt`, on your CP1252 system. Encrypt and decrypt it.
- [x] A file whose name contains an emoji, which is a surrogate pair in UTF-16 and four bytes in UTF-8 — a different code path in the conversion than the rest.
- [x] A file in a folder whose name is non-ASCII, so the directory component is exercised and not just the leaf.
- [x] Type a passphrase containing non-ASCII characters, encrypt, then decrypt with the same passphrase. This is the case that used to corrupt silently. If you have the age CLI handy, decrypting the result with it is the real proof.
- [x] Paste a recipient into the identity/recipient box and confirm a pasted key still parses. The box now returns UTF-16, and the pasted-versus-file branch keys off `PathFileExists`.

Then the rest of the UI, all of which moved:

- [x] Rename a text or image file to `.age` and double-click it. Explorer only sends winage what the `.age` association points at, so this is the only way to reach the "exists but is not an age file" branch. It should report age's own reason, then exit cleanly.
- [x] Chop the first few bytes off a real `.age` file and double-click it. Same branch, different reason.
- [x] Run `age.exe decrypt C:\nope.age` from a terminal. That is the missing-file branch, which nothing in Explorer can reach.
- [x] Run "Generate new age identity" from the folder background, and separately cancel the save dialog. Both should exit without leaving a window behind. Save it under a non-ASCII name too.
- [x] Confirm the main window closes after a successful encrypt.
- [x] Encrypt with the passphrase box left empty, to exercise the generated-passphrase dialog. That dialog now receives a converted UTF-16 string rather than the raw bytes from Rust, and the text must be selectable and copyable as before.
- [x] Encrypt through the UI using an identity file.
- [x] Encrypt an identity file with a passphrase, then use it as the identity for both an encrypt and a decrypt. The prompt should name the file it is asking about, in full, even when that name is non-ASCII. Cancelling should report an error rather than hang.
- [x] Check the About box renders its text intact, since that string literal moved to `_T()`.
- [ ] Drop an `age-plugin-*.exe` next to `age.exe` and use it. This is the actual fix for what Achim16 reported, and is the one item the port did not touch.
- [x] After reinstalling, confirm the install folder holds only `age.exe`.
- [x] Uninstall and confirm it completes without a registry error, that `Directory\Background\shell` keeps its `cmd` and `Powershell` entries, and that winage's two menu items are gone.

## Release mechanics

- [ ] Open the PR to `main`. The workflow's push trigger is `main` only, so this branch has never run CI even once.
- [ ] Watch the installer step on the runner. First CI run got through checkout, the Rust tests and the MFC build on `windows-2025-vs2026` — the image has Visual Studio 2026 Enterprise 18.9.2, MSVC 14.51.36231 and ATLMFC, so the label and toolset are right — then failed in devenv with "The parameter is incorrect." That was argument parsing: `/Project` was given a path relative to the working directory, and the solution records the project as the name `ageSetup` under `winage\`. Fixed.

  Still unknown after that fix: whether the runner has the Visual Studio Installer Projects extension at all. devenv rejected the arguments before it ever tried to load the `.vdproj`, so that question has not been answered yet. If the next run fails inside the project load, WiX v3.14 is already on the runner's PATH and is the fallback.
- [ ] Publish the GitHub release and let the workflow build and upload the MSI. Attaching the local one by hand breaks the attestation, which is generated against the artifact the workflow produced.
- [ ] Run `gh attestation verify ageSetup.msi --repo spieglt/winage` against the published asset. The README tells users to, and the command has never been run against a real release.
- [ ] Delete the `SIGN_CERT_BASE64` secret and its password from repo settings if they were ever added. Nothing reads them since signing was dropped.

## Communication

- [ ] Post the drafted reply to Achim16 on issue #4.
- [x] Write release notes. Drafted in `RELEASE_NOTES.md`.

## Needs doing in Visual Studio

Neither of these should be hand-edited in the project files; both are a minute in the UI.

- [ ] Add the Windows 10 launch condition. Without it a Windows 7, 8 or 8.1 machine installs cleanly and then fails at launch with "The procedure entry point ProcessPrng could not be located in the dynamic link library bcryptprimitives.dll", which points at nothing useful.

  Do not use `VersionNT`. Windows Installer still reports `VersionNT = 603` and `WindowsBuild = 9600` on Windows 11 — I confirmed that on this machine with an administrative install — so any comparison against 1000 rejects every machine. Detect the registry value instead, which exists only on Windows 10 and later:

  Right-click the `ageSetup` project, View > Launch Conditions. Right-click "Search Target Machine", Add Registry Search, and set `Property` to `WIN10ORLATER`, `Root` to `vsdrrHKLM`, `RegKey` to `SOFTWARE\Microsoft\Windows NT\CurrentVersion`, and `Value` to `CurrentMajorVersionNumber`. Then right-click "Requirements on Target Machine", Add Launch Condition, and set `Condition` to `WIN10ORLATER` and `Message` to "winage requires Windows 10 or later." The condition is just the property being set, since the value is absent before Windows 10.
- [ ] Open `IDD_IDENTITY_PASS_DIALOG` in the resource editor once so Visual Studio writes its own `DESIGNINFO` entry for it. The dialog was added to `Age.rc` by hand and works, but has no designer metadata yet.

## Deferred

- Plugin text input. `WinageCallbacks::request_public_string` returns None, so a plugin that asks for something other than a passphrase carries on without it. Needs a dialog decision rather than more plumbing.
- Localization, issue #2.

## Notes on the Unicode port

The x64 configurations are `CharacterSet=Unicode` now, matching Win32, which had been set that way all along and consequently had not compiled in years.

`AnsiToUtf8` became `WideToUtf8` and lost a step: it was UTF-16 to the ANSI code page to UTF-16 to UTF-8, and is now a single `WideCharToMultiByte`. The FFI itself did not change — Rust still receives UTF-8 `char*` — so `src/` needed no edits beyond tests.

Every buffer size in `OnBnClickedButton` counts characters; only the `malloc` calls scale by `sizeof(TCHAR)`. That is the arithmetic that produced the `.age` chop underflow, so it is the part most worth a second pair of eyes: `AgeDlg.cpp` lines 401-438.

`__wargv` is reached through `CommandLineArg()` in `Age.h`, which bounds-checks and returns `L""` rather than null. MFC enters through `wWinMain` in a Unicode build so the CRT populates it, but a null there would break every shell-integration launch rather than fail visibly.

One deliberate lossy edge remains: `WideToUtf8` does not pass `WC_ERR_INVALID_CHARS`, so a file name containing an unpaired surrogate — legal on NTFS, though nothing creates them — converts to U+FFFD instead of failing the operation. Rust cannot represent such a name in a `String` either way.
