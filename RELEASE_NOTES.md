# winage 2.0

## Update as soon as you can

winage 1.0 shipped age 0.6.0, which is affected by RUSTSEC-2024-0433. A recipient that names a plugin with a path separator in it makes age execute an arbitrary binary, and winage parses plugin recipients from pasted text and from recipient files. Anyone who handed you a recipient string could run code as you. Version 2 is built on age 0.12.

## Plugins work

age plugins such as `age-plugin-yubikey` and `age-plugin-pq` can be used for both recipients and identities. Put the plugin's `.exe` on your `PATH` or in the winage install folder next to `age.exe`, which is the simpler option because Windows hands a program the `PATH` its parent had (#4).

## Passphrase-protected identity files

An identity file that is itself encrypted to a passphrase now works. winage asks for the passphrase when it opens the file.

## Windows 10 or later, and an unsigned installer

Version 2 requires Windows 10. The installer is no longer code signed, because age stopped signing its own Windows builds and the certificate winage 1.0 used expired in 2024. Windows will show a SmartScreen prompt the first time you run it. Each release instead carries a build provenance attestation, so you can confirm an installer came from this repository:

    gh attestation verify ageSetup.msi --repo spieglt/winage
