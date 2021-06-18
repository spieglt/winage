use age::{
    armor::{ArmoredReader, ArmoredWriter, Format},
    cli_common::{
        file_io, read_identities, UiCallbacks,
    },
    plugin, Identity, IdentityFile, Recipient,
};
use i18n_embed::fluent::{fluent_language_loader, FluentLanguageLoader};
use lazy_static::lazy_static;
use rand::{
    distributions::{Distribution, Uniform},
    rngs::OsRng,
};
use rust_embed::RustEmbed;
use secrecy::SecretString;

mod error;
use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::{self, BufRead, BufReader};
use std::os::raw::{c_char, c_int, c_uchar};
use std::ptr::null_mut;

const BIP39_WORDLIST: &str = include_str!("..\\assets\\bip39-english.txt");

#[derive(RustEmbed)]
#[folder = "i18n"]
struct Translations;

// const TRANSLATIONS: Translations = Translations {};

lazy_static! {
    static ref LANGUAGE_LOADER: FluentLanguageLoader = fluent_language_loader!();
}

#[macro_export]
macro_rules! fl {
    ($message_id:literal) => {{
        i18n_embed_fl::fl!($crate::LANGUAGE_LOADER, $message_id)
    }};
}


#[repr(C)]
#[derive(Debug)]
pub struct COptions {
    input: *const c_char,
    encrypt: c_int, // bool: 0 == decrypting, 1 == encrypting
    using_passphrase: c_int, // bool: 0 == identity/recipients, 1 == passphrase
    passphrase: *const c_char,
    max_work_factor: c_uchar,
    armor: c_int, // bool: 0 == no armor, 1 == armor
    recipient: *const c_char,
    recipients_file: *const c_char,
    identity: *const c_char,
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
    recipients_file: Vec<String>,
    identity: Vec<String>,
    output: String,
}

#[no_mangle]
pub extern "C" fn wrapper(c_opts: *mut COptions) -> *const c_char {

    let opts: AgeOptions;
    unsafe {
        opts = convert(c_opts);
    }

    if opts.encrypt {
        match encrypt(&opts) {
            Ok(()) => CString::new(format!("Successfully encrypted {}", opts.output)).unwrap().into_raw(),
            Err(e) => CString::new(format!("Error: {}", e)).unwrap().into_raw(),
        }
    } else {
        match decrypt(&opts) {
            Ok(()) => CString::new(format!("Successfully decrypted {}", opts.output)).unwrap().into_raw(),
            Err(e) => CString::new(format!("Error: {}", e)).unwrap().into_raw(),
        }
    }
}

#[no_mangle]
pub extern "C" fn get_passphrase() -> *const c_char {
    let between = Uniform::from(0..2048);
    let mut rng = OsRng;
    let passphrase = (0..10).map(|_| {
        BIP39_WORDLIST
            .lines()
            .nth(between.sample(&mut rng))
            .expect("index is in range")
    })
    .fold(String::new(), |acc, s| {
        if acc.is_empty() {
            acc + s
        } else {
            acc + "-" + s
        }
    });
    CString::new(passphrase).unwrap().into_raw()
}

#[no_mangle]
pub unsafe extern "C" fn free_rust_string(ptr: *mut c_char) {
    if ptr != null_mut() {
        drop(CString::from_raw(ptr));
    }
}

#[no_mangle]
pub unsafe extern "C" fn get_decryption_mode(input: *const c_char) -> *const c_char {
    let rust_input = CStr::from_ptr(input).to_string_lossy().into_owned();
    let input_file = match file_io::InputReader::new(Some(rust_input)) {
        Ok(i) => i,
        Err(e) => return CString::new(format!("InputReader::new() failed: {}", e)).unwrap().into_raw(),
    };
    let decryptor = match age::Decryptor::new(ArmoredReader::new(input_file)) {
        Ok(d) => d,
        Err(e) => return CString::new(format!("Decryptor::new() failed: {}", e)).unwrap().into_raw(),
    };
    match decryptor {
        age::Decryptor::Passphrase(_) => return CString::new("passphrase").unwrap().into_raw(),
        age::Decryptor::Recipients(_) => return CString::new("recipients").unwrap().into_raw(),
    };
}

unsafe fn convert(c_opts: *mut COptions) -> AgeOptions {

    let input = if !(*c_opts).input.is_null() {
        CStr::from_ptr((*c_opts).input).to_string_lossy().into_owned()
    } else {
        "".to_string()
    };

    let passphrase = if !(*c_opts).passphrase.is_null() {
        let p = CStr::from_ptr((*c_opts).passphrase).to_string_lossy().into_owned();
        Some(p)
    } else {
        None
    };

    let max_work_factor = match (*c_opts).max_work_factor {
        i if i < 1 => None,
        _ => Some((*c_opts).max_work_factor),
    };

    // for recipient, recipients_file, identity, if null, put empty vec in rust options. if not, make vec with len 1.
    let (mut recipient, mut recipients_file, mut identity) = (vec![], vec![], vec![]);
    if !(*c_opts).recipient.is_null() {
        recipient = vec![CStr::from_ptr((*c_opts).recipient).to_string_lossy().into_owned()];
    }
    if !(*c_opts).recipients_file.is_null() {
        recipients_file = vec![CStr::from_ptr((*c_opts).recipients_file).to_string_lossy().into_owned()];
    }
    if !(*c_opts).identity.is_null() {
        identity = vec![CStr::from_ptr((*c_opts).identity).to_string_lossy().into_owned()];
    }

    let output = if !(*c_opts).output.is_null() {
        CStr::from_ptr((*c_opts).output).to_string_lossy().into_owned()
    } else {
        "".to_string()
    };

    AgeOptions{
        input: input,
        encrypt: (*c_opts).encrypt != 0,
        using_passphrase: (*c_opts).using_passphrase != 0,
        passphrase: passphrase,
        max_work_factor: max_work_factor,
        armor: (*c_opts).armor != 0,
        recipient: recipient,
        recipients_file: recipients_file,
        identity: identity,
        output: output,
    }
}

fn encrypt(opts: &AgeOptions) -> Result<(), error::EncryptError> {
    // if using passphrase but passphrase but not provided, generate. 
    // TODO: return to c++? return int for "yes, receive generated password" then 
    // c++ calls another function to read that value and launches pop-up
    let encryptor = if opts.using_passphrase {
        let passphrase = match &opts.passphrase {
            Some(p) => SecretString::new(p.to_string()),
            None => return Err(error::EncryptError::PassphraseTimedOut), // TODO: make a proper error for this. we should have received generated password from c++ already if we're using passphrase.
        };
        age::Encryptor::with_user_passphrase(passphrase)
    } else {
        // if not using passphrase, use recipients/identity
        age::Encryptor::with_recipients(read_recipients(
            opts.recipient.clone(),
            opts.recipients_file.clone(),
            opts.identity.clone(),
        )?)
    };


    // then io::copy, output, and armor
    let mut input = file_io::InputReader::new(Some(opts.input.clone()))?;

    let (format, output_format) = if opts.armor {
        (Format::AsciiArmor, file_io::OutputFormat::Text)
    } else {
        (Format::Binary, file_io::OutputFormat::Binary)
    };

    // Create an output to the user-requested location.
    let output = file_io::OutputWriter::new(Some(opts.output.clone()), output_format, 0o666)?;
    let mut output = encryptor.wrap_output(ArmoredWriter::wrap_output(output, format)?)?;

    io::copy(&mut input, &mut output)?;
    output
        .finish()
        .and_then(|armor| armor.finish())?;

    Ok(())
}

fn decrypt(opts: &AgeOptions) -> Result<(), error::DecryptError> {
    let input_file = file_io::InputReader::new(Some(opts.input.clone()))?;
    let decryptor = age::Decryptor::new(ArmoredReader::new(input_file))?;
    let output = opts.output.clone();
    match decryptor {
        age::Decryptor::Passphrase(decryptor) => {
            // TODO: how do we ensure that a passphrase was sent with AgeOptions?
            let p = if opts.passphrase.is_none() {
                return Err(error::DecryptError::MissingIdentities) // TODO: make proper error here
            } else {
                SecretString::new(opts.passphrase.clone().unwrap())
            };
            decryptor
                .decrypt(&p, opts.max_work_factor)
                .map_err(|e| e.into())
                .and_then(|input| write_output(input, Some(output)))
        },
        age::Decryptor::Recipients(decryptor) => {
            let identities = read_identities(
                opts.identity.clone(),
                error::DecryptError::IdentityNotFound,
                #[cfg(feature = "ssh")]
                error::DecryptError::UnsupportedKey,
            )?;

            if identities.is_empty() {
                return Err(error::DecryptError::MissingIdentities);
            }

            decryptor
                .decrypt(identities.iter().map(|i| i.as_ref() as &dyn Identity))
                .map_err(|e| e.into())
                .and_then(|input| write_output(input, Some(output)))
        },
    }
}

fn write_output<R: io::Read>(
    mut input: R,
    output: Option<String>,
) -> Result<(), error::DecryptError> {
    let mut output = file_io::OutputWriter::new(output, file_io::OutputFormat::Unknown, 0o666)?;

    io::copy(&mut input, &mut output)?;

    Ok(())
}

/// Reads recipients from the provided arguments.
fn read_recipients(
    recipient_strings: Vec<String>,
    recipients_file_strings: Vec<String>,
    identity_strings: Vec<String>,
) -> Result<Vec<Box<dyn Recipient>>, error::EncryptError> {
    let mut recipients: Vec<Box<dyn Recipient>> = vec![];
    let mut plugin_recipients: Vec<plugin::Recipient> = vec![];
    let mut plugin_identities: Vec<plugin::Identity> = vec![];

    for arg in recipient_strings {
        parse_recipient(arg, &mut recipients, &mut plugin_recipients)?;
    }

    for arg in recipients_file_strings {
        let f = File::open(&arg)?;
        let buf = BufReader::new(f);
        read_recipients_list(&arg, buf, &mut recipients, &mut plugin_recipients)?;
    }

    for filename in identity_strings {
        // Try parsing as a single multi-line SSH identity.
        #[cfg(feature = "ssh")]
        match age::ssh::Identity::from_buffer(
            BufReader::new(File::open(&filename)?),
            Some(filename.clone()),
        ) {
            Ok(age::ssh::Identity::Unsupported(k)) => {
                return Err(error::EncryptError::UnsupportedKey(filename, k))
            }
            Ok(identity) => {
                if let Ok(recipient) = age::ssh::Recipient::try_from(identity) {
                    recipients.push(Box::new(recipient));
                    continue;
                }
            }
            Err(_) => (),
        }

        // Try parsing as multiple single-line age identities.
        let identity_file = IdentityFile::from_file(filename.clone())?;
        let (new_ids, new_plugin_ids) = identity_file.split_into();
        for identity in new_ids {
            recipients.push(Box::new(identity.to_public()));
        }
        plugin_identities.extend(new_plugin_ids);
    }

    // Collect the names of the required plugins.
    let mut plugin_names = plugin_recipients
        .iter()
        .map(|r| r.plugin())
        .chain(plugin_identities.iter().map(|i| i.plugin()))
        .collect::<Vec<_>>();
    plugin_names.sort_unstable();
    plugin_names.dedup();

    // Find the required plugins.
    for plugin_name in plugin_names {
        recipients.push(Box::new(plugin::RecipientPluginV1::new(
            plugin_name,
            &plugin_recipients,
            &plugin_identities,
            UiCallbacks,
        )?))
    }

    Ok(recipients)
}

/// Reads file contents as a list of recipients
fn read_recipients_list<R: BufRead>(
    filename: &str,
    buf: R,
    recipients: &mut Vec<Box<dyn Recipient>>,
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
    recipients: &mut Vec<Box<dyn Recipient>>,
    plugin_recipients: &mut Vec<plugin::Recipient>,
) -> Result<(), error::EncryptError> {
    if let Ok(pk) = s.parse::<age::x25519::Recipient>() {
        recipients.push(Box::new(pk));
    } else if let Some(pk) = {
        #[cfg(feature = "ssh")]
        {
            s.parse::<age::ssh::Recipient>().ok().map(Box::new)
        }

        #[cfg(not(feature = "ssh"))]
        None
    } {
        recipients.push(pk);
    } else if let Ok(recipient) = s.parse::<plugin::Recipient>() {
        plugin_recipients.push(recipient);
    } else {
        return Err(error::EncryptError::InvalidRecipient(s));
    }

    Ok(())
}
