# winage 2.0

## Security fix

winage 1.0 was built on rage 0.6.0, which is affected by RUSTSEC-2024-0433: a recipient naming a plugin with a path separator in it causes rage to run an arbitrary binary. winage parses plugin recipients from pasted text and from recipient files, so this is worth updating for if you handle recipients that came from someone else. Version 2 is built on rage 0.12.

## File names in any language

File names, folder names and typed passphrases can use any characters Windows can store, in any mix of scripts, whatever your system locale is set to. Previously anything outside your system's code page was replaced with `?` before winage saw it, so a Japanese file name on a Western install could not be opened (#4). A passphrase typed outside the code page was altered the same way, silently, and produced a file that other age tools would not open.

## Plugins, and identity files with a passphrase

age plugins such as `age-plugin-yubikey` and `age-plugin-pq` work for both recipients and identities. Put the plugin's `.exe` on your `PATH` or in the winage install folder next to `age.exe`, which is the simpler option because Windows hands a program the `PATH` its parent had (#4).

An identity file that is itself encrypted to a passphrase also works. winage asks for that passphrase when it opens the file.

## Installing and uninstalling

Version 2 requires Windows 10, and the installer says so rather than letting you install and fail at launch.

Uninstalling 1.0 ended with a registry error and left winage's entries in the right-click menu behind. Uninstall is clean now.

The installer is no longer code signed. age stopped signing its own Windows builds, and the certificate winage 1.0 used expired in 2024, so Windows shows a SmartScreen prompt the first time you run it: select `More info`, then `Run anyway`. Each release carries a build provenance attestation instead, so you can confirm an installer came from this repository before running it:

    gh attestation verify ageSetup.msi --repo spieglt/winage
