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
