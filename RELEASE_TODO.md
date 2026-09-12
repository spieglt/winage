# winage 2.0 release checklist

Branch: `age-0.12-upgrade` (3 commits, not yet merged to `main`).

## Already verified

- `cargo test --release` — 20/20 pass, covering passphrase, armored, identity-file, recipients-list and encrypted-identity round trips through the real FFI entry points.
- `msbuild age.sln /t:age /p:Configuration=Release /p:Platform=x64` — clean under Visual Studio 2026 and the v145 toolset, `age.exe` reports FileVersion 2.0.0.0.
- Installer builds locally and lands at `winage\ageSetup\Release\ageSetup.msi`, the path the workflow expects. ProductVersion 2.0.0, new ProductCode, UpgradeCode unchanged.
- Upgrade from an installed 1.0 completes with a single Add/Remove Programs entry.

## Manual smoke tests

None of this is reachable from the Rust tests — everything below is C++ or shell integration.

- [x] Encrypt and decrypt a file whose name contains an umlaut. This is issue #4, and `AnsiToUtf8` has never run outside a compiler.
- [x] Rename a text or image file to `.age` and double-click it. Explorer only sends winage what the `.age` association points at, so this is the only way to reach the "exists but is not an age file" branch. It should now report age's own reason rather than a flat "not a valid age file", then exit cleanly.
- [x] Chop the first few bytes off a real `.age` file and double-click it. Same branch, different reason.
- [x] Run `age.exe decrypt C:\nope.age` from a terminal. That is the missing-file branch, which nothing in Explorer can reach.
- [x] Give a file a name with a character outside your code page, a CJK one on CP1252 say, rename it to `.age`, and double-click. It should say Windows replaced a character rather than blaming the file.
- [x] Run "Generate new age identity" from the folder background, and separately cancel the save dialog. Both should exit without leaving a window behind.
- [x] Confirm the main window closes after a successful encrypt.
- [x] Encrypt with the passphrase box left empty, to exercise the generated-passphrase dialog. That block was restructured around a leak and the zeroing.
- [x] Encrypt through the UI using an identity file, which now goes through `IdentityFile::to_recipients` rather than the old hand-rolled path.
- [ ] Drop an `age-plugin-*.exe` next to `age.exe` and use it. This is the actual fix for what Achim16 reported.
- [x] After reinstalling, confirm the install folder holds only `age.exe`. A build before the exclusions shipped copies of `ntdll.dll`, `bcryptprimitives.dll` and `api-ms-win-core-synch-l1-2-0.dll` taken from the build machine, and an existing install still has them on disk.
- [x] Encrypt an identity file with a passphrase, then use it as the identity for both an encrypt and a decrypt. The new dialog should name the file it is asking about. Cancelling it should report an error rather than hang.
- [x] Uninstall and confirm it completes without a registry error, that `Directory\Background\shell` keeps its `cmd` and `Powershell` entries, and that winage's two menu items are gone. The currently installed build predates the vdproj fix, so Windows still has the old package cached for uninstall; install a freshly built MSI over it first.

## Release mechanics

- [ ] Open the PR to `main`. The workflow's push trigger is `main` only, so this branch has never run CI even once.
- [ ] Watch the installer step on the runner. It needs the Visual Studio Installer Projects extension, which GitHub runners do not preinstall, and devenv hung locally on what looked like a first-run dialog. WiX is the fallback if it fails. The build job runs on `windows-2025-vs2026`, a preview label, since the project needs v145 and `windows-latest` is still on Visual Studio 2022.
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

- Unicode port of the C++ front end. The x64 configs are `CharacterSet=MultiByte`, so paths and passphrases outside the machine's ANSI codepage are `?`-substituted by Windows before winage sees them. Cross-script filenames break; single-locale users are fine.
- Plugin text input. `WinageCallbacks::request_public_string` returns None, so a plugin that asks for something other than a passphrase carries on without it. Needs a dialog decision rather than more plumbing.
- Localization, issue #2.
