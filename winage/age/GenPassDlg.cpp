// GenPassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Age.h"
#include "GenPassDlg.h"
#include "afxdialogex.h"


// GenPassDlg dialog

IMPLEMENT_DYNAMIC(GenPassDlg, CDialogEx)

GenPassDlg::GenPassDlg(char* msg, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GEN_PASS_DIALOG, pParent)
{
	this->msg = msg;
}

GenPassDlg::~GenPassDlg()
{
}

void GenPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, GENERATED_PASSWORD_BOX, genPassBox);
}


BEGIN_MESSAGE_MAP(GenPassDlg, CDialogEx)
END_MESSAGE_MAP()


// GenPassDlg message handlers


BOOL GenPassDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add extra initialization here
	genPassBox.SetWindowText(this->msg);
	this->GotoDlgCtrl((CWnd*)&genPassBox);
	genPassBox.SetSel(0, -1);

	return FALSE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL GenPassDlg::PreTranslateMessage(MSG* pMsg)
{
	// Clicking the generated passphrase reselects it, so it stays easy to copy.
	if (IsSelectAllKey(pMsg) || pMsg->message == WM_LBUTTONUP) {
		SelectAllInFocusedEdit();
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
