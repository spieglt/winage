
<p align="center"><img alt="The age logo, an wireframe of St. Peters dome in Rome, with the text: age, file encryption" width="600" src="https://user-images.githubusercontent.com/1225294/132245842-fda4da6a-1cea-4738-a3da-2dc860861c98.png"></p>

This is a Windows GUI for the file encryption tool [age](https://age-encryption.org), built on top of [rage](https://str4d.xyz/rage). This adapter wraps the `age` crate, compiles as a static library, and is used by an MFC GUI.

The format specification is at age-encryption.org/v1. To discuss the spec or other age related topics, please email the mailing list at age-dev@googlegroups.com. age was designed by [@Benjojo12](https://twitter.com/Benjojo12) and [@FiloSottile](https://twitter.com/FiloSottile). rage was designed by [@str4d](https://twitter.com/str4d).

# Installation

Install the MSI on the [releases](https://github.com/spieglt/winage/releases) page.

# Use

To generate a new identity, right-click the background of an Explorer window and select `Generate new age identity`.

To encrypt a file, right-click it and select `Encrypt with age`.

To decrypt an `.age` file, double-click it and specify the passphrase or identity file.

To encrypt to multiple recipients, specify a text file with one recipient on each line. To encrypt to a single recipient, you can paste it directly.

# Compilation instructions

1. Install [Rust](https://www.rust-lang.org/tools/install), open `winage`, and run `cargo build --release`.

2. Install and open Visual Studio 2019. Go to `Extensions` > `Manage Extensions` and install `Microsoft Visual Studio Installer Projects`. Open the `winage\winage\age.sln` Solution, select `Release`, `x64`, and build.

# Restrictions

- Does not handle passphrase-protected identity files.


