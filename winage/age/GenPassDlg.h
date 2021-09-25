#pragma once


// GenPassDlg dialog

class GenPassDlg : public CDialogEx
{
	DECLARE_DYNAMIC(GenPassDlg)

public:
	GenPassDlg(char* msg, CWnd* pParent = nullptr);   // standard constructor
	virtual ~GenPassDlg();
	char* msg;


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GEN_PASS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangePasswordBox();
	virtual BOOL OnInitDialog();
};
