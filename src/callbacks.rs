//! Routes age's prompts to the front end.
//!
//! age's own `UiCallbacks` asks through a pinentry binary or a tty. winage is a GUI
//! subsystem binary with neither, so every request it makes comes back empty and
//! anything needing one — a passphrase-encrypted identity file, most plugin
//! interaction — fails without ever reaching the user. The front end registers
//! dialogs here instead.

use age::secrecy::SecretString;
use age::Callbacks;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int};
use std::ptr;
use std::sync::RwLock;

use crate::to_cstring;

/// Dialogs the front end registers at startup. A null entry means winage cannot make
/// that kind of request, and age is told so rather than left waiting on one.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CCallbacks {
    /// Shows an informational message.
    pub display_message: Option<extern "C" fn(message: *const c_char)>,
    /// Asks a yes/no question. 1 for yes, 0 for no, anything else for "could not ask".
    /// `no` may be null.
    pub confirm: Option<
        extern "C" fn(message: *const c_char, yes: *const c_char, no: *const c_char) -> c_int,
    >,
    /// Prompts for a passphrase. Returns a UTF-8 string owned by the caller, or null if
    /// the user cancelled. Whatever it returns is passed back to `free_string`.
    pub request_passphrase: Option<extern "C" fn(description: *const c_char) -> *mut c_char>,
    /// Frees a string returned by `request_passphrase`.
    pub free_string: Option<extern "C" fn(s: *mut c_char)>,
}

static REGISTERED: RwLock<Option<CCallbacks>> = RwLock::new(None);

/// Registers the front end's dialogs, or clears them when given null.
///
/// # Safety
///
/// `callbacks` must be null or point to a valid `CCallbacks`. The function pointers in
/// it must remain callable for the lifetime of the process, and must not unwind.
#[no_mangle]
pub unsafe extern "C" fn set_callbacks(callbacks: *const CCallbacks) {
    let new = if callbacks.is_null() {
        None
    } else {
        Some(*callbacks)
    };
    if let Ok(mut slot) = REGISTERED.write() {
        *slot = new;
    }
}

fn registered() -> Option<CCallbacks> {
    REGISTERED.read().ok().and_then(|slot| *slot)
}

/// Passed to age wherever it may need to ask the user something.
#[derive(Clone, Copy)]
pub(crate) struct WinageCallbacks;

impl Callbacks for WinageCallbacks {
    fn display_message(&self, message: &str) {
        if let (Some(show), Some(message)) = (
            registered().and_then(|c| c.display_message),
            to_cstring(message),
        ) {
            show(message.as_ptr());
        }
    }

    fn confirm(&self, message: &str, yes_string: &str, no_string: Option<&str>) -> Option<bool> {
        let ask = registered()?.confirm?;
        let message = to_cstring(message)?;
        let yes = to_cstring(yes_string)?;
        let no = no_string.and_then(to_cstring);
        let no_ptr = no.as_ref().map_or(ptr::null(), |s| s.as_ptr());

        match ask(message.as_ptr(), yes.as_ptr(), no_ptr) {
            1 => Some(true),
            0 => Some(false),
            _ => None,
        }
    }

    fn request_public_string(&self, _description: &str) -> Option<String> {
        // Only plugins ask for these, and there is no dialog for one yet. Returning
        // None tells the plugin to carry on without it.
        None
    }

    fn request_passphrase(&self, description: &str) -> Option<SecretString> {
        let callbacks = registered()?;
        let request = callbacks.request_passphrase?;
        let free = callbacks.free_string?;
        let description = to_cstring(description)?;

        let raw = request(description.as_ptr());
        if raw.is_null() {
            return None; // cancelled
        }

        let (passphrase, len) = {
            let bytes = unsafe { CStr::from_ptr(raw) }.to_bytes();
            (String::from_utf8_lossy(bytes).into_owned(), bytes.len())
        };
        // Don't leave the front end's copy in the freed allocation.
        unsafe { ptr::write_bytes(raw as *mut u8, 0, len) };
        free(raw);

        Some(SecretString::from(passphrase))
    }
}
