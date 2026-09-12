
<p align="center"><img alt="The age logo, an wireframe of St. Peters dome in Rome, with the text: age, file encryption" width="600" src="https://user-images.githubusercontent.com/1225294/132245842-fda4da6a-1cea-4738-a3da-2dc860861c98.png"></p>

This is a Windows GUI for the file encryption tool [age](https://age-encryption.org), built on top of [rage](https://str4d.xyz/rage). This adapter wraps the `age` crate, compiles as a static library, and is used by a C++ MFC front end.

The format specification is at age-encryption.org/v1. To discuss the spec or other age related topics, please email the mailing list at age-dev@googlegroups.com. age was designed by [@Benjojo12](https://twitter.com/Benjojo12) and [@FiloSottile](https://twitter.com/FiloSottile). rage was designed by [@str4d](https://twitter.com/str4d).

https://user-images.githubusercontent.com/22626146/139507406-08803f91-f7d7-4c15-b69c-0f4494884786.mp4

# Installation

Install the MSI on the [releases](https://github.com/spieglt/winage/releases) page.

The installer isn't code signed, so Windows shows "Windows protected your PC" the first time you run it. Select `More info`, then `Run anyway`. The UAC prompt will name an unknown publisher.

Each release carries a build provenance attestation, so you can confirm an installer came from this repository before running it:

    gh attestation verify ageSetup.msi --repo spieglt/winage

# Use

To generate a new identity, right-click the background of an Explorer window and select `Generate new age identity`.

To encrypt a file, right-click it and select `Encrypt with age`.

To decrypt an `.age` file, double-click it and specify the passphrase or identity file.

To encrypt to multiple recipients, specify a text file with one recipient on each line. To encrypt to a single recipient, you can paste it directly.

If your identity file is itself encrypted to a passphrase, winage asks for that passphrase when it opens the file.

# Plugins

age plugins such as `age-plugin-yubikey` and `age-plugin-pq` work with winage. Put the plugin's `.exe` either in a directory on your `PATH` or in the winage install folder next to `age.exe`, then use its recipients and identities as you would native ones.

Windows hands a program the `PATH` its parent had, so a `PATH` you changed after signing in won't reach winage until you sign out and back in. Dropping the plugin next to `age.exe` avoids that.

# Compilation instructions

1. Install [Rust](https://www.rust-lang.org/tools/install), open `winage`, and run `cargo build --release`. Run `cargo test` to exercise the encryption and decryption paths.

2. Install and open Visual Studio 2019. Go to `Extensions` > `Manage Extensions` and install `Microsoft Visual Studio Installer Projects`. Open the `winage\winage\age.sln` Solution, select `Release`, `x64`, and build.

# Restrictions

- File names and typed passphrases are limited to your system's ANSI code page. Characters outside it, such as a Japanese file name on a Western install, are replaced with `?` by Windows before winage sees them.


