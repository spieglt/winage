
// AgeDlg.h : header file
//

#pragma once
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "bcrypt.lib")

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
	afx_msg void OnEnChangeBox();
	afx_msg void OnBnClickedPassphrase();
	afx_msg void OnBnClickedIdentityRecipient();
	afx_msg void OnBnClickedLabel();
	afx_msg void OnEnChangeFileSelector();
	afx_msg void OnBnClickedArmor();
};

struct COptions {
	const char* input;
	BOOL encrypt;
	BOOL using_passphrase;
	const char* passphrase;
	unsigned char max_work_factor;
	BOOL armor;
	const char* recipient;
	const char* recipients_file;
	const char* identity;
	const char* output;
};

extern "C" {
	char* wrapper(COptions *opts);
	char* get_passphrase();
	void free_rust_string(char* ptr);
	char* get_decryption_mode(char* input);
}
