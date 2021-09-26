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
	ON_EN_CHANGE(GENERATED_PASSWORD_BOX, &GenPassDlg::OnEnChangePasswordBox)
END_MESSAGE_MAP()


// GenPassDlg message handlers


void GenPassDlg::OnEnChangePasswordBox()
{
	// If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// Add your control notification handler code here
}


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
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0)
		{
			CWnd* wnd = GetFocus();
			if (wnd && IsEditOrEditBrowse(wnd)) {
				((CEdit*)wnd)->SetSel(0, -1);
			}
		}
	}
	else if (pMsg->message == WM_LBUTTONUP)
	{
		CWnd* wnd = GetFocus();
		if (wnd && IsEditOrEditBrowse(wnd)) {
			((CEdit*)wnd)->SetSel(0, -1);
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
