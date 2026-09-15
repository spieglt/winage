// ConfirmPassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "age.h"
#include "ConfirmPassDlg.h"
#include "afxdialogex.h"


// ConfirmPassDlg dialog

IMPLEMENT_DYNAMIC(ConfirmPassDlg, CDialogEx)

ConfirmPassDlg::ConfirmPassDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CONFIRM_PASS_DIALOG, pParent)
	, confirmedPass(_T(""))
{

}

ConfirmPassDlg::~ConfirmPassDlg()
{
	// The caller zeroes this after comparing, but only when it gets that far; cancelling
	// the dialog would otherwise leave the passphrase in the freed buffer.
	if (!confirmedPass.IsEmpty()) {
		SecureZeroMemory(confirmedPass.GetBuffer(), confirmedPass.GetLength() * sizeof(TCHAR));
		confirmedPass.ReleaseBuffer(0);
	}
}

void ConfirmPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PASS_CONFIRMATION, confirmedPass);
}


BEGIN_MESSAGE_MAP(ConfirmPassDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &ConfirmPassDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// ConfirmPassDlg message handlers


void ConfirmPassDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();
}


BOOL ConfirmPassDlg::PreTranslateMessage(MSG* pMsg)
{
	if (IsSelectAllKey(pMsg)) {
		SelectAllInFocusedEdit();
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
