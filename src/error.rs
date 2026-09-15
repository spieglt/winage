use std::fmt;
use std::io;

use age::cli_common::ReadError;

pub(crate) enum EncryptError {
    Age(age::EncryptError),
    InvalidRecipient(String),
    Io(io::Error),
    PassphraseMissing,
    PluginResolve(age::plugin::ResolveError),
    UnsupportedKey(String, age::ssh::UnsupportedKey),
}

impl From<age::plugin::ResolveError> for EncryptError {
    fn from(e: age::plugin::ResolveError) -> Self {
        EncryptError::PluginResolve(e)
    }
}

impl From<age::EncryptError> for EncryptError {
    fn from(e: age::EncryptError) -> Self {
        match e {
            age::EncryptError::Io(e) => EncryptError::Io(e),
            _ => EncryptError::Age(e),
        }
    }
}

impl From<io::Error> for EncryptError {
    fn from(e: io::Error) -> Self {
        EncryptError::Io(e)
    }
}

impl fmt::Display for EncryptError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EncryptError::Age(e) => write!(f, "{}", e),
            EncryptError::InvalidRecipient(recipient) => write!(
                f, "Invalid recipient: {}", recipient.as_str()
            ),
            EncryptError::Io(e) => write!(f, "{}", e),
            EncryptError::PassphraseMissing => write!(f, "Passphrase not provided"),
            EncryptError::PluginResolve(e) => write!(f, "{}", e),
            EncryptError::UnsupportedKey(filename, k) => k.display(f, Some(filename.as_str())),
        }
    }
}

pub(crate) enum DecryptError {
    Age(age::DecryptError),
    Identities(ReadError),
    Io(io::Error),
    MissingIdentities,
    MissingPassphrase,
}

impl From<age::DecryptError> for DecryptError {
    fn from(e: age::DecryptError) -> Self {
        DecryptError::Age(e)
    }
}

impl From<io::Error> for DecryptError {
    fn from(e: io::Error) -> Self {
        DecryptError::Io(e)
    }
}

impl From<ReadError> for DecryptError {
    fn from(e: ReadError) -> Self {
        DecryptError::Identities(e)
    }
}

impl fmt::Display for DecryptError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DecryptError::Age(e) => write!(f, "{}", e),
            DecryptError::Identities(e) => write!(f, "{}", e),
            DecryptError::Io(e) => write!(f, "{}", e),
            DecryptError::MissingIdentities => write!(f, "Missing identities."),
            DecryptError::MissingPassphrase => write!(f, "Passphrase not provided"),
        }
    }
}
