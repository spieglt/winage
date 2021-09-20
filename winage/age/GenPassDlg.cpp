// GenPassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Age.h"
#include "GenPassDlg.h"
#include "afxdialogex.h"


// GenPassDlg dialog

IMPLEMENT_DYNAMIC(GenPassDlg, CDialogEx)

GenPassDlg::GenPassDlg(char* msg, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GEN_PW_DIALOG, pParent)
{
	this->msg = msg;
}

GenPassDlg::~GenPassDlg()
{
}

void GenPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(GenPassDlg, CDialogEx)
	ON_EN_CHANGE(GENERATED_PASSWORD_BOX, &GenPassDlg::OnEnChangePasswordBox)
END_MESSAGE_MAP()


// GenPassDlg message handlers


void GenPassDlg::OnEnChangePasswordBox()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


BOOL GenPassDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	this->GetDlgItem(GENERATED_PASSWORD_BOX)->SetWindowTextA(this->msg);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
