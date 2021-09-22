use std::fmt;
use std::io;

pub(crate) enum EncryptError {
    Age(age::EncryptError),
    InvalidRecipient(String),
    Io(io::Error),
    PassphraseMissing,
    UnsupportedKey(String, age::ssh::UnsupportedKey),
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
            EncryptError::UnsupportedKey(filename, k) => k.display(f, Some(filename.as_str())),
        }
    }
}

pub(crate) enum DecryptError {
    Age(age::DecryptError),
    IdentityNotFound(String),
    Io(io::Error),
    MissingIdentities,
    UnsupportedKey(String, age::ssh::UnsupportedKey),
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

impl fmt::Display for DecryptError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DecryptError::Age(e) => write!(f, "{}", e),
            DecryptError::IdentityNotFound(filename) => write!(
                f, "Identity file not found: {}", filename.as_str()
            ),
            DecryptError::Io(e) => write!(f, "{}", e),
            DecryptError::MissingIdentities => write!(f, "Missing identities."),
            DecryptError::UnsupportedKey(filename, k) => k.display(f, Some(filename.as_str())),
        }
    }
}

pub(crate) enum Error {
    Decryption(DecryptError),
    Encryption(EncryptError),
}

impl From<DecryptError> for Error {
    fn from(e: DecryptError) -> Self {
        Error::Decryption(e)
    }
}

impl From<EncryptError> for Error {
    fn from(e: EncryptError) -> Self {
        Error::Encryption(e)
    }
}
