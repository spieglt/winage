mod error;
mod identity;
#[cfg(test)]
mod tests;

use age::{
    armor::{ArmoredReader, ArmoredWriter, Format},
    cli_common::{file_io, read_identities, StdinGuard, UiCallbacks},
    plugin,
    secrecy::SecretString,
    Identity, IdentityFile, Recipient,
};
use rand::{
    distributions::{Distribution, Uniform},
    rngs::OsRng,
};

use std::convert::TryFrom;
use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::{self, BufRead, BufReader};
use std::iter;
use std::os::raw::{c_char, c_int, c_uchar};
use std::panic::{self, AssertUnwindSafe};
use std::ptr::null_mut;

const BIP39_WORDLIST: &str = include_str!("..\\assets\\bip39-english.txt");

#[repr(C)]
#[derive(Debug)]
pub struct COptions {
    input: *const c_char,
    encrypt: c_int,          // bool: 0 == decrypting, 1 == encrypting
    using_passphrase: c_int, // bool: 0 == identity/recipients, 1 == passphrase
    passphrase: *const c_char,
    max_work_factor: c_uchar,
    armor: c_int, // bool: 0 == no armor, 1 == armor
    recipient: *const c_char,
    recipient_or_identity_file: *const c_char,
    output: *const c_char,
}

#[derive(Debug)]
struct AgeOptions {
    input: String,
    encrypt: bool,
    using_passphrase: bool,
    passphrase: Option<String>,
    max_work_factor: Option<u8>,
    armor: bool,
    recipient: Vec<String>,
    recipient_or_identity_file: Vec<String>,
    output: String,
}

// Panics must not unwind into the C++ caller; convert them to an error string.
fn catch_ffi(f: impl FnOnce() -> *const c_char) -> *const c_char {
    panic::catch_unwind(AssertUnwindSafe(f))
        .unwrap_or_else(|_| to_c_string("Error: internal error (panic) in age library".to_string()))
}

// age's localized errors wrap interpolated values in Unicode bidi isolation
// marks (U+2068/U+2069), which the MFC dialogs render as mojibake.
fn to_c_string(s: String) -> *const c_char {
    let cleaned: String = s
        .chars()
        .filter(|c| !matches!(c, '\u{2068}' | '\u{2069}'))
        .collect();
    CString::new(cleaned)
        .unwrap_or_else(|_| {
            CString::new("Error: message contained a null byte").expect("static string")
        })
        .into_raw()
}

#[no_mangle]
pub extern "C" fn wrapper(c_opts: *mut COptions) -> *const c_char {
    if c_opts.is_null() {
        return to_c_string("Error: null options pointer".to_string());
    }
    let opts: AgeOptions;
    unsafe {
        opts = convert(c_opts);
    }

    catch_ffi(move || {
        if opts.encrypt {
            match encrypt(&opts) {
                Ok(()) => to_c_string(format!("Successfully encrypted {}", opts.output)),
                Err(e) => to_c_string(format!("Error: {}", e)),
            }
        } else {
            match decrypt(&opts) {
                Ok(()) => to_c_string(format!("Successfully decrypted {}", opts.output)),
                Err(e) => to_c_string(format!("Error: {}", e)),
            }
        }
    })
}

#[no_mangle]
pub extern "C" fn get_passphrase() -> *const c_char {
    catch_ffi(|| {
        let words: Vec<&str> = BIP39_WORDLIST.lines().collect();
        let between = Uniform::from(0..words.len());
        let mut rng = OsRng;
        let passphrase = (0..10)
            .map(|_| words[between.sample(&mut rng)])
            .collect::<Vec<_>>()
            .join("-");
        CString::new(passphrase).expect("wordlist is ASCII").into_raw()
    })
}

#[no_mangle]
pub unsafe extern "C" fn free_rust_string(ptr: *mut c_char) {
    if ptr != null_mut() {
        let s = CString::from_raw(ptr);
        // Some of these strings are generated passphrases; don't leave them
        // in the freed allocation.
        let len = s.as_bytes().len();
        std::ptr::write_bytes(ptr as *mut u8, 0, len);
        drop(s);
    }
}

#[no_mangle]
pub unsafe extern "C" fn get_decryption_mode(input: *const c_char) -> *const c_char {
    if input.is_null() {
        return to_c_string("get_decryption_mode: null input".to_string());
    }
    let rust_input = CStr::from_ptr(input).to_string_lossy().into_owned();
    catch_ffi(move || {
        let input_file = match file_io::InputReader::new(Some(rust_input)) {
            Ok(i) => i,
            Err(e) => return to_c_string(format!("InputReader::new() failed: {}", e)),
        };
        let decryptor = match age::Decryptor::new(ArmoredReader::new(input_file)) {
            Ok(d) => d,
            Err(e) => return to_c_string(format!("Decryptor::new() failed: {}", e)),
        };
        if decryptor.is_scrypt() {
            CString::new("passphrase").expect("static string").into_raw()
        } else {
            CString::new("recipients").expect("static string").into_raw()
        }
    })
}

#[no_mangle]
pub unsafe extern "C" fn generate_identity(output_path: *const c_char) -> *const c_char {
    let p = if !output_path.is_null() {
        CStr::from_ptr(output_path).to_string_lossy().into_owned()
    } else {
        "".to_string()
    };
    catch_ffi(move || match identity::generate(&p) {
        Ok(()) => CString::new("ok").expect("static string").into_raw(),
        Err(e) => to_c_string(format!("{}", e)),
    })
}

unsafe fn convert(c_opts: *mut COptions) -> AgeOptions {
    let input = if !(*c_opts).input.is_null() {
        CStr::from_ptr((*c_opts).input)
            .to_string_lossy()
            .into_owned()
    } else {
        "".to_string()
    };

    let passphrase = if !(*c_opts).passphrase.is_null() {
        let p = CStr::from_ptr((*c_opts).passphrase)
            .to_string_lossy()
            .into_owned();
        Some(p)
    } else {
        None
    };

    let max_work_factor = match (*c_opts).max_work_factor {
        i if i < 1 => None,
        _ => Some((*c_opts).max_work_factor),
    };

    // for recipient, recipients_file, identity, if null, put empty vec in rust options. if not, make vec with len 1.
    let (mut recipient, mut recipient_or_identity_file) = (vec![], vec![]);
    if !(*c_opts).recipient.is_null() {
        recipient = vec![CStr::from_ptr((*c_opts).recipient)
            .to_string_lossy()
            .into_owned()];
    }
    if !(*c_opts).recipient_or_identity_file.is_null() {
        recipient_or_identity_file = vec![CStr::from_ptr((*c_opts).recipient_or_identity_file)
            .to_string_lossy()
            .into_owned()];
    }

    let output = if !(*c_opts).output.is_null() {
        CStr::from_ptr((*c_opts).output)
            .to_string_lossy()
            .into_owned()
    } else {
        "".to_string()
    };

    AgeOptions {
        input: input,
        encrypt: (*c_opts).encrypt != 0,
        using_passphrase: (*c_opts).using_passphrase != 0,
        passphrase: passphrase,
        max_work_factor: max_work_factor,
        armor: (*c_opts).armor != 0,
        recipient: recipient,
        recipient_or_identity_file: recipient_or_identity_file,
        output: output,
    }
}

fn encrypt(opts: &AgeOptions) -> Result<(), error::EncryptError> {
    let encryptor = if opts.using_passphrase {
        let passphrase = match &opts.passphrase {
            Some(p) => SecretString::from(p.clone()),
            None => return Err(error::EncryptError::PassphraseMissing),
        };
        age::Encryptor::with_user_passphrase(passphrase)
    } else {
        // if not using passphrase, use recipients/identity
        let recipients = read_recipients(
            opts.recipient.clone(),
            opts.recipient_or_identity_file.clone(),
        )?;
        age::Encryptor::with_recipients(recipients.iter().map(|b| b.as_ref() as &dyn Recipient))?
    };

    // then io::copy, output, and armor
    let mut input = file_io::InputReader::new(Some(opts.input.clone()))?;

    let (format, output_format) = if opts.armor {
        (Format::AsciiArmor, file_io::OutputFormat::Text)
    } else {
        (Format::Binary, file_io::OutputFormat::Binary)
    };

    // Create an output to the user-requested location. The file dialog has
    // already prompted about overwrites, so allow them here.
    let output = file_io::OutputWriter::new(
        Some(opts.output.clone()),
        true,
        output_format,
        0o666,
        false,
    )?;
    let mut output = encryptor.wrap_output(ArmoredWriter::wrap_output(output, format)?)?;

    io::copy(&mut input, &mut output)?;
    output.finish().and_then(|armor| armor.finish())?;

    Ok(())
}

fn decrypt(opts: &AgeOptions) -> Result<(), error::DecryptError> {
    let input_file = file_io::InputReader::new(Some(opts.input.clone()))?;
    let decryptor = age::Decryptor::new(ArmoredReader::new(input_file))?;
    let output = opts.output.clone();

    if decryptor.is_scrypt() {
        let passphrase = match &opts.passphrase {
            Some(p) => SecretString::from(p.clone()),
            None => return Err(error::DecryptError::MissingPassphrase),
        };
        let mut identity = age::scrypt::Identity::new(passphrase);
        if let Some(f) = opts.max_work_factor {
            identity.set_max_work_factor(f);
        }
        decryptor
            .decrypt(iter::once(&identity as &dyn Identity))
            .map_err(error::DecryptError::from)
            .and_then(|input| write_output(input, Some(output)))
    } else {
        let identities = read_identities(
            opts.recipient_or_identity_file.clone(),
            opts.max_work_factor,
            &mut StdinGuard::new(false),
        )?;

        if identities.is_empty() {
            return Err(error::DecryptError::MissingIdentities);
        }

        decryptor
            .decrypt(identities.iter().map(|i| i.as_ref() as &dyn Identity))
            .map_err(error::DecryptError::from)
            .and_then(|input| write_output(input, Some(output)))
    }
}

fn write_output<R: io::Read>(
    mut input: R,
    output: Option<String>,
) -> Result<(), error::DecryptError> {
    let mut output =
        file_io::OutputWriter::new(output, true, file_io::OutputFormat::Unknown, 0o666, false)?;

    io::copy(&mut input, &mut output)?;

    Ok(())
}

/// Reads recipients from the provided arguments.
fn read_recipients(
    recipient_strings: Vec<String>,
    file_strings: Vec<String>,
) -> Result<Vec<Box<dyn Recipient + Send>>, error::EncryptError> {
    let mut recipients: Vec<Box<dyn Recipient + Send>> = vec![];
    let mut plugin_recipients: Vec<plugin::Recipient> = vec![];

    for arg in recipient_strings {
        parse_recipient(arg, &mut recipients, &mut plugin_recipients)?;
    }

    for arg in file_strings {
        let f = File::open(&arg)?;
        let buf = BufReader::new(f);
        match read_recipients_list(&arg, buf, &mut recipients, &mut plugin_recipients) {
            Ok(()) => (),
            Err(_e) => { // if we failed to parse as recipients list, try to parse as identity file
                // Try parsing as a single multi-line SSH identity.
                match age::ssh::Identity::from_buffer(
                    BufReader::new(File::open(&arg)?),
                    Some(arg.clone()),
                ) {
                    Ok(age::ssh::Identity::Unsupported(k)) => {
                        return Err(error::EncryptError::UnsupportedKey(arg, k))
                    }
                    Ok(identity) => {
                        if let Ok(recipient) = age::ssh::Recipient::try_from(identity) {
                            recipients.push(Box::new(recipient));
                            continue;
                        }
                    }
                    Err(_) => (),
                }

                // Try parsing as multiple single-line age identities; this
                // handles native and plugin identities alike.
                let identity_file =
                    IdentityFile::from_file(arg.clone())?.with_callbacks(UiCallbacks);
                recipients.extend(identity_file.to_recipients()?);
            },
        }
    }

    // Collect the names of the plugins required by pasted plugin recipients.
    let mut plugin_names = plugin_recipients
        .iter()
        .map(|r| r.plugin())
        .collect::<Vec<_>>();
    plugin_names.sort_unstable();
    plugin_names.dedup();

    // Find the required plugins.
    for plugin_name in plugin_names {
        recipients.push(Box::new(plugin::RecipientPluginV1::new(
            plugin_name,
            &plugin_recipients,
            &[],
            UiCallbacks,
        )?))
    }

    Ok(recipients)
}

/// Reads file contents as a list of recipients
fn read_recipients_list<R: BufRead>(
    filename: &str,
    buf: R,
    recipients: &mut Vec<Box<dyn Recipient + Send>>,
    plugin_recipients: &mut Vec<plugin::Recipient>,
) -> io::Result<()> {
    for (line_number, line) in buf.lines().enumerate() {
        let line = line?;

        // Skip empty lines and comments
        if line.is_empty() || line.find('#') == Some(0) {
            continue;
        } else if parse_recipient(line, recipients, plugin_recipients).is_err() {
            // Return a line number in place of the line, so we don't leak the file
            // contents in error messages.
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                format!(
                    "recipients file {} contains non-recipient data on line {}",
                    filename,
                    line_number + 1
                ),
            ));
        }
    }

    Ok(())
}

/// Parses a recipient from a string.
fn parse_recipient(
    s: String,
    recipients: &mut Vec<Box<dyn Recipient + Send>>,
    plugin_recipients: &mut Vec<plugin::Recipient>,
) -> Result<(), error::EncryptError> {
    if let Ok(pk) = s.parse::<age::x25519::Recipient>() {
        recipients.push(Box::new(pk));
    } else if let Some(pk) = s.parse::<age::ssh::Recipient>().ok().map(Box::new) {
        recipients.push(pk);
    } else if let Ok(recipient) = s.parse::<plugin::Recipient>() {
        plugin_recipients.push(recipient);
    } else {
        return Err(error::EncryptError::InvalidRecipient(s));
    }

    Ok(())
}
