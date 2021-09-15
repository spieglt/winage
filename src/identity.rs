use age::x25519;
use std::fs::File;
use std::io::{Result, Write};
use secrecy::ExposeSecret;

pub fn generate(identity_path: &str) -> Result<()> {    
    let sk = x25519::Identity::generate();
    let pk = sk.to_public();
    let mut out_file = File::create(identity_path)?;
    // TODO: internationalization
    writeln!(out_file, "# created: {}", chrono::Local::now().to_rfc3339_opts(chrono::SecondsFormat::Secs, true))?;
    writeln!(out_file, "# public key: {}", pk)?;
    writeln!(out_file, "{}", sk.to_string().expose_secret())?;
    Ok(())
}
