# winage 2.0 release checklist

Branch: `age-0.12-upgrade` (3 commits, not yet merged to `main`).

## Already verified

- `cargo test --release` — 20/20 pass, covering passphrase, armored, identity-file, recipients-list and encrypted-identity round trips through the real FFI entry points.
- `msbuild age.sln /t:age /p:Configuration=Release /p:Platform=x64` — clean under Visual Studio 2026 and the v145 toolset, `age.exe` reports FileVersion 2.0.0.0.
- Installer builds locally and lands at `winage\ageSetup\Release\ageSetup.msi`, the path the workflow expects. ProductVersion 2.0.0, new ProductCode, UpgradeCode unchanged.
- Upgrade from an installed 1.0 completes with a single Add/Remove Programs entry.

## Manual smoke tests

None of this is reachable from the Rust tests — everything below is C++ or shell integration.

- [ ] Encrypt and decrypt a file whose name contains an umlaut. This is issue #4, and `AnsiToUtf8` has never run outside a compiler.
- [ ] Double-click a file that is not an age file. Expect the error dialog, then a clean exit.
- [ ] Double-click a corrupt `.age` file. Same.
- [ ] Run "Generate new age identity" from the folder background, and separately cancel the save dialog. Both should exit without leaving a window behind.
- [ ] Confirm the main window closes after a successful encrypt.
- [ ] Encrypt with the passphrase box left empty, to exercise the generated-passphrase dialog. That block was restructured around a leak and the zeroing.
- [ ] Encrypt through the UI using an identity file, which now goes through `IdentityFile::to_recipients` rather than the old hand-rolled path.
- [ ] Drop an `age-plugin-*.exe` next to `age.exe` and use it. This is the actual fix for what Achim16 reported.
- [ ] Encrypt an identity file with a passphrase, then use it as the identity for both an encrypt and a decrypt. The new dialog should name the file it is asking about. Cancelling it should report an error rather than hang.

## Release mechanics

- [ ] Open the PR to `main`. The workflow's push trigger is `main` only, so this branch has never run CI even once.
- [ ] Watch the installer step on the runner. It needs the Visual Studio Installer Projects extension, which GitHub runners do not preinstall, and devenv hung locally on what looked like a first-run dialog. WiX is the fallback if it fails. The build job runs on `windows-2025-vs2026`, a preview label, since the project needs v145 and `windows-latest` is still on Visual Studio 2022.
- [ ] Publish the GitHub release and let the workflow build and upload the MSI. Attaching the local one by hand breaks the attestation, which is generated against the artifact the workflow produced.
- [ ] Run `gh attestation verify ageSetup.msi --repo spieglt/winage` against the published asset. The README tells users to, and the command has never been run against a real release.
- [ ] Delete the `SIGN_CERT_BASE64` secret and its password from repo settings if they were ever added. Nothing reads them since signing was dropped.

## Communication

- [ ] Post the drafted reply to Achim16 on issue #4.
- [ ] Write release notes led by RUSTSEC-2024-0433: the shipped 1.0 can execute an arbitrary binary via a malicious plugin name. Then plugins being found next to `age.exe`, non-ASCII paths working, and the installer no longer being signed.

## Deferred

- Unicode port of the C++ front end. The x64 configs are `CharacterSet=MultiByte`, so paths and passphrases outside the machine's ANSI codepage are `?`-substituted by Windows before winage sees them. Cross-script filenames break; single-locale users are fine.
- Plugin text input. `WinageCallbacks::request_public_string` returns None, so a plugin that asks for something other than a passphrase carries on without it. Needs a dialog decision rather than more plumbing.
- Localization, issue #2.
