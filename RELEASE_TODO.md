# winage 2.0 release checklist

Branch: `age-0.12-upgrade` (3 commits, not yet merged to `main`).

## Already verified

- `cargo test --release` — 14/14 pass, covering passphrase, armored, identity-file and recipients-list round trips through the real FFI entry points.
- `msbuild age.sln /t:age /p:Configuration=Release /p:Platform=x64` — clean, `age.exe` reports FileVersion 2.0.0.0.
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
- [ ] Point the identity field at a passphrase-protected identity file and note the exact error. age 0.12 detects these now, so it should fail clearly rather than misparse; worth confirming before the README claims anything.

## Release mechanics

- [ ] Open the PR to `main`. The workflow's push trigger is `main` only, so this branch has never run CI even once.
- [ ] Watch the installer step on the runner. It needs the Visual Studio Installer Projects extension, which GitHub runners do not preinstall, and devenv hung locally on what looked like a first-run dialog. WiX is the fallback if it fails.
- [ ] Publish the GitHub release and let the workflow build and upload the MSI. Attaching the local one by hand breaks the attestation, which is generated against the artifact the workflow produced.
- [ ] Run `gh attestation verify ageSetup.msi --repo spieglt/winage` against the published asset. The README tells users to, and the command has never been run against a real release.
- [ ] Delete the `SIGN_CERT_BASE64` secret and its password from repo settings if they were ever added. Nothing reads them since signing was dropped.

## Communication

- [ ] Post the drafted reply to Achim16 on issue #4.
- [ ] Write release notes led by RUSTSEC-2024-0433: the shipped 1.0 can execute an arbitrary binary via a malicious plugin name. Then plugins being found next to `age.exe`, non-ASCII paths working, and the installer no longer being signed.
- [ ] Decide what to do with `CODE_REVIEW.md`. It is untracked and mostly stale now that 20 of its 21 items are closed.

## Known issues, not blocking

- `src/lib.rs` `convert()` builds `AgeOptions.passphrase` as a plain `String` that is never zeroed. The C++ buffers and the strings returned through `free_rust_string` are, and `SecretString` covers the clone `encrypt`/`decrypt` make, so this is the last plaintext copy left in a freed allocation.
- `src/lib.rs:75` calls `convert(c_opts)` before `catch_ffi`, leaving the function that does the most pointer work outside the unwind guard.
- `winage/age/AgeDlg.cpp:450` reports `AnsiToUtf8` failures as "Memory allocation error", but that function also returns NULL when `MultiByteToWideChar` fails.
- `winage/age/AgeDlg.cpp:184` collapses every `get_decryption_mode` result into "Not a valid age file", discarding detail the Rust side now produces.
- `src/lib.rs:325` `read_recipients_list` pushes recipients as it parses and only then returns Err, so the identity-file fallback extends a vector that may already hold entries from the same file. The GUI cannot trigger it, since the recipient and identity-file fields are mutually exclusive there.
- `.github/workflows/build.yml:14` grants `contents: write` job-wide, including on pull_request builds.
- `.github/workflows/build.yml:45` says `ageSetup.vdproj`; the file is `AgeSetup.vdproj`. Harmless on Windows.

## Deferred

- Unicode port of the C++ front end. The x64 configs are `CharacterSet=MultiByte`, so paths and passphrases outside the machine's ANSI codepage are `?`-substituted by Windows before winage sees them. Cross-script filenames break; single-locale users are fine.
- Passphrase-protected identity files. age 0.12 already detects them and the `Callbacks` trait is public, so what is missing is a GUI passphrase prompt to replace `UiCallbacks`, which only talks to a terminal.
- Localization, issue #2.
