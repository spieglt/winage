#pragma once


// IdentityPassDlg dialog
//
// Shown when age asks for the passphrase protecting an identity file. The description
// comes from age and names the file being opened.

class IdentityPassDlg : public CDialogEx
{
	DECLARE_DYNAMIC(IdentityPassDlg)

public:
	IdentityPassDlg(const CStringW& description, CWnd* pParent = nullptr);
	virtual ~IdentityPassDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IDENTITY_PASS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CString passphrase;

private:
	CStringW description;
};
