
// AgeDlg.h : header file
//

#pragma once
// Libraries the Rust static library depends on.
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ntdll.lib")

// CAgeDlg dialog
class CAgeDlg : public CDialogEx
{
// Construction
public:
	CAgeDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AGE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton();
	afx_msg void OnBnClickedPassphrase();
	afx_msg void OnBnClickedIdentityRecipient();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

struct COptions {
	const char* input;
	BOOL encrypt;
	BOOL using_passphrase;
	const char* passphrase;
	unsigned char max_work_factor;
	BOOL armor;
	const char* recipient;
	const char* recipient_or_identity_file;
	const char* output;
};

extern "C" {
	// Dialogs the Rust side calls when age needs to ask the user something. A null
	// entry means winage cannot make that kind of request, and age is told so rather
	// than left waiting on one.
	struct CCallbacks {
		void (*display_message)(const char* message);
		// 1 for yes, 0 for no, anything else for "could not ask". no may be null.
		int (*confirm)(const char* message, const char* yes, const char* no);
		// Returns a UTF-8 string, or null if the user cancelled. Whatever it returns
		// is handed back to free_string.
		char* (*request_passphrase)(const char* description);
		void (*free_string)(char* s);
	};

	char* wrapper(COptions *opts);
	char* get_passphrase();
	void free_rust_string(char* ptr);
	char* get_decryption_mode(char* input);
	char* generate_identity(char* output_path);
	void set_callbacks(const CCallbacks* callbacks);
}

// Hands the dialogs above to the Rust side. Call once at startup.
void RegisterAgeCallbacks();
