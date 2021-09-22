#pragma once


// ConfirmPassDlg dialog

class ConfirmPassDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ConfirmPassDlg)

public:
	ConfirmPassDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~ConfirmPassDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONFIRM_PASS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CString confirmedPass;
};
