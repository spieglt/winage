// IdentityPassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Age.h"
#include "IdentityPassDlg.h"
#include "afxdialogex.h"


// IdentityPassDlg dialog

IMPLEMENT_DYNAMIC(IdentityPassDlg, CDialogEx)

IdentityPassDlg::IdentityPassDlg(const CStringW& description, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IDENTITY_PASS_DIALOG, pParent)
	, passphrase(_T(""))
	, description(description)
{

}

IdentityPassDlg::~IdentityPassDlg()
{
	if (!passphrase.IsEmpty()) {
		SecureZeroMemory(passphrase.GetBuffer(), passphrase.GetLength() * sizeof(TCHAR));
		passphrase.ReleaseBuffer(0);
	}
}

void IdentityPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_IDENTITY_PASS, passphrase);
}


BEGIN_MESSAGE_MAP(IdentityPassDlg, CDialogEx)
END_MESSAGE_MAP()


// IdentityPassDlg message handlers


BOOL IdentityPassDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CWnd* label = this->GetDlgItem(IDC_IDENTITY_PASS_DESC);
	if (label != NULL) {
		::SetWindowTextW(label->GetSafeHwnd(), description);
	}
	this->GetDlgItem(IDC_IDENTITY_PASS)->SetFocus();

	return FALSE; // focus was set explicitly
}


BOOL IdentityPassDlg::PreTranslateMessage(MSG* pMsg)
{
	if (IsSelectAllKey(pMsg)) {
		SelectAllInFocusedEdit();
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
