#pragma once


// GenPassDlg dialog

class GenPassDlg : public CDialogEx
{
	DECLARE_DYNAMIC(GenPassDlg)

public:
	GenPassDlg(LPCTSTR msg, CWnd* pParent = nullptr);   // standard constructor
	virtual ~GenPassDlg();
	LPCTSTR msg;


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GEN_PASS_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CEdit genPassBox;
};
