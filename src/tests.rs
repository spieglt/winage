//! Round-trip tests driving the same FFI entry points the C++ front end calls.

use super::*;
use std::fs;
use std::path::{Path, PathBuf};
use std::ptr;

fn scratch(name: &str) -> PathBuf {
    let mut p = std::env::temp_dir();
    p.push(format!("winage-test-{}-{}", std::process::id(), name));
    p
}

/// Copies the returned message and frees it the way the C++ side does.
fn take_message(ptr: *const c_char) -> String {
    assert!(!ptr.is_null(), "FFI returned a null string");
    let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
    unsafe { free_rust_string(ptr as *mut c_char) };
    s
}

fn run(
    input: &Path,
    output: &Path,
    encrypt: bool,
    passphrase: Option<&str>,
    recipient: Option<&str>,
    recipient_file: Option<&Path>,
    armor: bool,
) -> String {
    let input_c = CString::new(input.to_str().unwrap()).unwrap();
    let output_c = CString::new(output.to_str().unwrap()).unwrap();
    let pass_c = passphrase.map(|p| CString::new(p).unwrap());
    let rec_c = recipient.map(|r| CString::new(r).unwrap());
    let recf_c = recipient_file.map(|r| CString::new(r.to_str().unwrap()).unwrap());

    let mut opts = COptions {
        input: input_c.as_ptr(),
        encrypt: if encrypt { 1 } else { 0 },
        using_passphrase: if passphrase.is_some() { 1 } else { 0 },
        passphrase: pass_c.as_ref().map_or(ptr::null(), |c| c.as_ptr()),
        max_work_factor: 0,
        armor: if armor { 1 } else { 0 },
        recipient: rec_c.as_ref().map_or(ptr::null(), |c| c.as_ptr()),
        recipient_or_identity_file: recf_c.as_ref().map_or(ptr::null(), |c| c.as_ptr()),
        output: output_c.as_ptr(),
    };
    take_message(wrapper(&mut opts))
}

fn public_key_of(identity_file: &Path) -> String {
    let contents = fs::read_to_string(identity_file).unwrap();
    contents
        .lines()
        .find_map(|l| l.strip_prefix("# public key: "))
        .expect("identity file records its public key")
        .to_string()
}

fn make_identity(name: &str) -> PathBuf {
    let path = scratch(name);
    let path_c = CString::new(path.to_str().unwrap()).unwrap();
    let msg = take_message(unsafe { generate_identity(path_c.as_ptr()) });
    assert_eq!(msg, "ok", "generate_identity failed");
    path
}

#[test]
fn passphrase_round_trip() {
    let plain = scratch("pp.txt");
    let enc = scratch("pp.txt.age");
    let dec = scratch("pp.out.txt");
    fs::write(&plain, b"hello age").unwrap();

    let msg = run(&plain, &enc, true, Some("correct horse"), None, None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, Some("correct horse"), None, None, false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"hello age");

    for p in [plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn wrong_passphrase_fails() {
    let plain = scratch("wrong.txt");
    let enc = scratch("wrong.txt.age");
    let dec = scratch("wrong.out.txt");
    fs::write(&plain, b"secret").unwrap();

    run(&plain, &enc, true, Some("right"), None, None, false);
    let msg = run(&enc, &dec, false, Some("wrong"), None, None, false);
    assert!(msg.starts_with("Error"), "{}", msg);

    for p in [plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn armored_round_trip() {
    let plain = scratch("armor.txt");
    let enc = scratch("armor.txt.age");
    let dec = scratch("armor.out.txt");
    fs::write(&plain, b"armored payload").unwrap();

    run(&plain, &enc, true, Some("pw"), None, None, true);
    let armored = fs::read_to_string(&enc).unwrap();
    assert!(armored.starts_with("-----BEGIN AGE ENCRYPTED FILE-----"));

    let msg = run(&enc, &dec, false, Some("pw"), None, None, false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"armored payload");

    for p in [plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn identity_file_round_trip() {
    let id = make_identity("id.txt");
    let plain = scratch("id-plain.txt");
    let enc = scratch("id-plain.txt.age");
    let dec = scratch("id-plain.out.txt");
    fs::write(&plain, b"to a recipient").unwrap();

    // Encrypting with an identity file exercises IdentityFile::to_recipients.
    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"to a recipient");

    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn pasted_recipient_round_trip() {
    let id = make_identity("pasted-id.txt");
    let recipient = public_key_of(&id);
    let plain = scratch("pasted.txt");
    let enc = scratch("pasted.txt.age");
    let dec = scratch("pasted.out.txt");
    fs::write(&plain, b"pasted key").unwrap();

    let msg = run(&plain, &enc, true, None, Some(&recipient), None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"pasted key");

    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn recipients_list_round_trip() {
    let id_a = make_identity("multi-a.txt");
    let id_b = make_identity("multi-b.txt");
    let list = scratch("recipients.txt");
    fs::write(
        &list,
        format!(
            "# recipients\n{}\n\n{}\n",
            public_key_of(&id_a),
            public_key_of(&id_b)
        ),
    )
    .unwrap();

    let plain = scratch("multi.txt");
    let enc = scratch("multi.txt.age");
    fs::write(&plain, b"two recipients").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&list), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    // Either identity alone must be able to decrypt it.
    for (i, id) in [&id_a, &id_b].iter().enumerate() {
        let dec = scratch(&format!("multi.out{}.txt", i));
        let msg = run(&enc, &dec, false, None, None, Some(id), false);
        assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
        assert_eq!(fs::read(&dec).unwrap(), b"two recipients");
        let _ = fs::remove_file(dec);
    }

    for p in [id_a, id_b, list, plain, enc] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn non_ascii_paths_round_trip() {
    // The C++ side hands us UTF-8; umlauts in a path must survive.
    let plain = scratch("grüße-öäü.txt");
    let enc = scratch("grüße-öäü.txt.age");
    let dec = scratch("grüße-öäü.out.txt");
    fs::write(&plain, b"umlauts").unwrap();

    let msg = run(&plain, &enc, true, Some("pw"), None, None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);
    assert!(enc.exists(), "encrypted file not written to the umlaut path");

    let msg = run(&enc, &dec, false, Some("pw"), None, None, false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"umlauts");

    for p in [plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn decryption_mode_detection() {
    let plain = scratch("mode.txt");
    fs::write(&plain, b"x").unwrap();

    let pass_enc = scratch("mode-pass.age");
    run(&plain, &pass_enc, true, Some("pw"), None, None, false);
    let path_c = CString::new(pass_enc.to_str().unwrap()).unwrap();
    let mode = take_message(unsafe { get_decryption_mode(path_c.as_ptr()) });
    assert_eq!(mode, "passphrase");

    let id = make_identity("mode-id.txt");
    let rec_enc = scratch("mode-rec.age");
    run(&plain, &rec_enc, true, None, None, Some(&id), false);
    let path_c = CString::new(rec_enc.to_str().unwrap()).unwrap();
    let mode = take_message(unsafe { get_decryption_mode(path_c.as_ptr()) });
    assert_eq!(mode, "recipients");

    for p in [plain, pass_enc, rec_enc, id] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn decryption_mode_rejects_non_age_file() {
    let junk = scratch("junk.bin");
    fs::write(&junk, b"not an age file at all").unwrap();
    let path_c = CString::new(junk.to_str().unwrap()).unwrap();
    let mode = take_message(unsafe { get_decryption_mode(path_c.as_ptr()) });
    assert_ne!(mode, "passphrase");
    assert_ne!(mode, "recipients");
    let _ = fs::remove_file(junk);
}

#[test]
fn null_pointers_are_rejected() {
    let msg = take_message(wrapper(ptr::null_mut()));
    assert!(msg.starts_with("Error"), "{}", msg);

    let msg = take_message(unsafe { get_decryption_mode(ptr::null()) });
    assert!(msg.contains("null"), "{}", msg);

    // Freeing null must be a no-op rather than a crash.
    unsafe { free_rust_string(ptr::null_mut()) };
}

#[test]
fn missing_recipient_is_an_error() {
    let plain = scratch("norecip.txt");
    let enc = scratch("norecip.age");
    fs::write(&plain, b"x").unwrap();
    let msg = run(&plain, &enc, true, None, None, None, false);
    assert!(msg.starts_with("Error"), "{}", msg);
    for p in [plain, enc] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn generated_passphrase_is_ten_distinct_words() {
    let p = take_message(get_passphrase());
    let words: Vec<&str> = p.split('-').collect();
    assert_eq!(words.len(), 10, "{}", p);
    assert!(words.iter().all(|w| !w.is_empty()), "{}", p);
    assert!(
        words
            .iter()
            .all(|w| w.chars().all(|c| c.is_ascii_lowercase())),
        "{}",
        p
    );
    // Two calls must not produce the same passphrase.
    let q = take_message(get_passphrase());
    assert_ne!(p, q);
}

#[test]
fn messages_have_no_bidi_isolation_marks() {
    // age localizes errors with fluent, which wraps interpolated values in
    // U+2068/U+2069; the MFC dialogs render those as mojibake.
    let raw = "Could not find '\u{2068}age-plugin-pq\u{2069}' on the PATH.".to_string();
    assert_eq!(
        take_message(to_c_string(raw)),
        "Could not find 'age-plugin-pq' on the PATH."
    );
}

#[test]
fn embedded_null_does_not_panic() {
    let msg = take_message(to_c_string("bad\0message".to_string()));
    assert!(msg.contains("null byte"), "{}", msg);
}

// Passphrase-protected identity files. These drive the same callback table the MFC
// front end registers, with the dialogs replaced by canned answers.

use crate::callbacks::{set_callbacks, CCallbacks};
use std::sync::{Mutex, MutexGuard};

/// The callback table is process-wide, so tests that install their own take turns.
fn callback_lock() -> MutexGuard<'static, ()> {
    static LOCK: Mutex<()> = Mutex::new(());
    LOCK.lock().unwrap_or_else(|e| e.into_inner())
}

static PROMPT_ANSWER: Mutex<Option<String>> = Mutex::new(None);
static PROMPTS: Mutex<Vec<String>> = Mutex::new(Vec::new());
static MESSAGES: Mutex<Vec<String>> = Mutex::new(Vec::new());

extern "C" fn test_request_passphrase(description: *const c_char) -> *mut c_char {
    let description = unsafe { CStr::from_ptr(description) }
        .to_string_lossy()
        .into_owned();
    PROMPTS.lock().unwrap().push(description);

    match PROMPT_ANSWER.lock().unwrap().clone() {
        Some(p) => CString::new(p).unwrap().into_raw(),
        None => ptr::null_mut(), // the user dismissed the dialog
    }
}

extern "C" fn test_free_string(s: *mut c_char) {
    if !s.is_null() {
        drop(unsafe { CString::from_raw(s) });
    }
}

extern "C" fn test_display_message(message: *const c_char) {
    let message = unsafe { CStr::from_ptr(message) }
        .to_string_lossy()
        .into_owned();
    MESSAGES.lock().unwrap().push(message);
}

/// Installs callbacks answering every passphrase request with `answer`, or cancelling
/// when it is `None`.
fn install_callbacks(answer: Option<&str>) {
    *PROMPT_ANSWER.lock().unwrap() = answer.map(|s| s.to_string());
    PROMPTS.lock().unwrap().clear();
    MESSAGES.lock().unwrap().clear();

    let callbacks = CCallbacks {
        display_message: Some(test_display_message),
        confirm: None,
        request_passphrase: Some(test_request_passphrase),
        free_string: Some(test_free_string),
    };
    unsafe { set_callbacks(&callbacks) };
}

fn clear_callbacks() {
    unsafe { set_callbacks(ptr::null()) };
}

/// Builds an identity file that is itself encrypted to a passphrase, the way a user
/// would by encrypting their identity with winage.
fn make_encrypted_identity(name: &str, passphrase: &str) -> PathBuf {
    let plain = make_identity(&format!("{}-plain", name));
    let encrypted = scratch(name);
    let msg = run(&plain, &encrypted, true, Some(passphrase), None, None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);
    let _ = fs::remove_file(plain);
    encrypted
}

#[test]
fn encrypted_identity_round_trip() {
    let _guard = callback_lock();
    install_callbacks(Some("identity passphrase"));

    let id = make_encrypted_identity("enc-id.age", "identity passphrase");
    let plain = scratch("enc-id-plain.txt");
    let enc = scratch("enc-id-plain.txt.age");
    let dec = scratch("enc-id-plain.out.txt");
    fs::write(&plain, b"behind a passphrase").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"behind a passphrase");

    // The prompt says which file it is asking about, with age's bidi isolation marks
    // already stripped.
    let prompts = PROMPTS.lock().unwrap().clone();
    assert!(
        prompts.iter().any(|p| p.contains("enc-id.age")),
        "{:?}",
        prompts
    );
    assert!(
        prompts.iter().all(|p| !p.contains('\u{2068}')),
        "{:?}",
        prompts
    );

    clear_callbacks();
    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn encrypted_identity_cancelled_prompt_is_an_error() {
    let _guard = callback_lock();
    install_callbacks(Some("pw"));

    let id = make_encrypted_identity("cancel-id.age", "pw");
    let plain = scratch("cancel.txt");
    let enc = scratch("cancel.txt.age");
    let dec = scratch("cancel.out.txt");
    fs::write(&plain, b"x").unwrap();
    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    // Dismissing the dialog has to surface as an error, not a hang or a panic.
    install_callbacks(None);
    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Error"), "{}", msg);

    clear_callbacks();
    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn encrypted_identity_wrong_passphrase_is_an_error() {
    let _guard = callback_lock();
    install_callbacks(Some("right"));

    let id = make_encrypted_identity("wrong-id.age", "right");
    let plain = scratch("wrongid.txt");
    let enc = scratch("wrongid.txt.age");
    let dec = scratch("wrongid.out.txt");
    fs::write(&plain, b"x").unwrap();
    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    install_callbacks(Some("wrong"));
    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Error"), "{}", msg);

    clear_callbacks();
    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn encrypted_identity_without_callbacks_is_an_error() {
    let _guard = callback_lock();
    clear_callbacks();

    let id = make_encrypted_identity("nocb-id.age", "pw");
    let plain = scratch("nocb.txt");
    let enc = scratch("nocb.txt.age");
    fs::write(&plain, b"x").unwrap();

    // Encryption reads the recipients out of the identity file, so with no way to ask
    // for the passphrase it must fail rather than quietly produce an unreadable file.
    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Error"), "{}", msg);

    for p in [id, plain, enc] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn encrypted_identity_that_does_not_match_warns() {
    let _guard = callback_lock();
    install_callbacks(Some("pw"));

    let recipient_id = make_identity("mismatch-recipient.txt");
    let other_id = make_encrypted_identity("mismatch-other.age", "pw");
    let plain = scratch("mismatch.txt");
    let enc = scratch("mismatch.txt.age");
    let dec = scratch("mismatch.out.txt");
    fs::write(&plain, b"x").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&recipient_id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&other_id), false);
    assert!(msg.starts_with("Error"), "{}", msg);

    // age warns through display_message that the identity decrypted but matched nothing.
    let messages = MESSAGES.lock().unwrap().clone();
    assert!(
        messages.iter().any(|m| m.contains("didn't match")),
        "{:?}",
        messages
    );

    clear_callbacks();
    for p in [recipient_id, other_id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn plain_identity_files_still_work_with_callbacks_installed() {
    let _guard = callback_lock();
    install_callbacks(Some("unused"));

    let id = make_identity("plain-with-cb.txt");
    let plain = scratch("plainwithcb.txt");
    let enc = scratch("plainwithcb.txt.age");
    let dec = scratch("plainwithcb.out.txt");
    fs::write(&plain, b"no prompt expected").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);
    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"no prompt expected");

    // An unencrypted identity file must not prompt for anything.
    let prompts = PROMPTS.lock().unwrap().clone();
    assert!(prompts.is_empty(), "{:?}", prompts);

    clear_callbacks();
    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

// The front end is a Unicode build, so it hands us UTF-8 converted straight from
// UTF-16 with no code page in between. These cover what that makes reachable and the
// old MBCS build could not express.

/// A path no single Windows ANSI code page can represent: four scripts plus an
/// astral-plane character, which is a surrogate pair in UTF-16 and four bytes in UTF-8.
const MIXED_SCRIPT: &str = "日本語-Ελληνικά-Русский-עברית-🔐";

#[test]
fn mixed_script_paths_round_trip() {
    let plain = scratch(&format!("{}.txt", MIXED_SCRIPT));
    let enc = scratch(&format!("{}.txt.age", MIXED_SCRIPT));
    let dec = scratch(&format!("{}.out.txt", MIXED_SCRIPT));
    fs::write(&plain, b"many scripts").unwrap();

    let msg = run(&plain, &enc, true, Some("pw"), None, None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);
    assert!(enc.exists(), "encrypted file not written to the mixed-script path");

    let msg = run(&enc, &dec, false, Some("pw"), None, None, false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"many scripts");

    // The success message carries the path back out to the front end intact.
    assert!(msg.contains(MIXED_SCRIPT), "{}", msg);

    for p in [plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn non_ascii_passphrase_round_trips() {
    // A passphrase outside the code page used to be '?'-substituted before winage saw
    // it, which silently produced a file the real age CLI could not open.
    let passphrase = "правильная-лошадь-🔑-日本";
    let plain = scratch("pw-unicode.txt");
    let enc = scratch("pw-unicode.txt.age");
    let dec = scratch("pw-unicode.out.txt");
    fs::write(&plain, b"unicode passphrase").unwrap();

    let msg = run(&plain, &enc, true, Some(passphrase), None, None, false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, Some(passphrase), None, None, false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"unicode passphrase");

    // A passphrase that differs only past the ASCII range must still fail.
    let dec2 = scratch("pw-unicode.out2.txt");
    let msg = run(&enc, &dec2, false, Some("правильная-лошадь-🔑-日语"), None, None, false);
    assert!(msg.starts_with("Error"), "{}", msg);

    for p in [plain, enc, dec, dec2] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn mixed_script_identity_and_recipient_file() {
    let id = make_identity(&format!("{}-id.txt", MIXED_SCRIPT));
    let list = scratch(&format!("{}-recipients.txt", MIXED_SCRIPT));
    fs::write(&list, format!("{}\n", public_key_of(&id))).unwrap();

    let plain = scratch(&format!("{}-payload.txt", MIXED_SCRIPT));
    let enc = scratch(&format!("{}-payload.txt.age", MIXED_SCRIPT));
    let dec = scratch(&format!("{}-payload.out.txt", MIXED_SCRIPT));
    fs::write(&plain, b"recipient file with a unicode name").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&list), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"recipient file with a unicode name");

    for p in [id, list, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}

#[test]
fn mixed_script_encrypted_identity() {
    let _guard = callback_lock();
    install_callbacks(Some("パスワード-🔐"));

    let id = make_encrypted_identity(&format!("{}-enc-id.age", MIXED_SCRIPT), "パスワード-🔐");
    let plain = scratch(&format!("{}-enc.txt", MIXED_SCRIPT));
    let enc = scratch(&format!("{}-enc.txt.age", MIXED_SCRIPT));
    let dec = scratch(&format!("{}-enc.out.txt", MIXED_SCRIPT));
    fs::write(&plain, b"unicode all the way down").unwrap();

    let msg = run(&plain, &enc, true, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully encrypted"), "{}", msg);

    let msg = run(&enc, &dec, false, None, None, Some(&id), false);
    assert!(msg.starts_with("Successfully decrypted"), "{}", msg);
    assert_eq!(fs::read(&dec).unwrap(), b"unicode all the way down");

    // The prompt names the file, so the path survives the trip back out too.
    let prompts = PROMPTS.lock().unwrap().clone();
    assert!(
        prompts.iter().any(|p| p.contains(MIXED_SCRIPT)),
        "{:?}",
        prompts
    );

    clear_callbacks();
    for p in [id, plain, enc, dec] {
        let _ = fs::remove_file(p);
    }
}
