
// Age.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CAgeApp:
// See Age.cpp for the implementation of this class
//

class CAgeApp : public CWinApp
{
public:
	CAgeApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CAgeApp theApp;

inline BOOL IsEditOrEditBrowse(CWnd* pWnd)
{
	if (!pWnd) return FALSE;
	HWND hWnd = pWnd->GetSafeHwnd();
	if (hWnd == NULL)
		return FALSE;

	const char* editName = "Edit";
	const char* ebcName = "MFCEditBrowse";
	char className[14];
	return ::GetClassName(hWnd, className, 14) &&
		(!_tcsicmp(className, editName) || !_tcsicmp(className, ebcName));
}

// MFC dialogs don't handle Ctrl+A in edit controls; each dialog's
// PreTranslateMessage does it with these.
inline BOOL IsSelectAllKey(const MSG* pMsg)
{
	return pMsg->message == WM_KEYDOWN && pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0;
}

inline void SelectAllInFocusedEdit()
{
	CWnd* wnd = CWnd::GetFocus();
	if (wnd && IsEditOrEditBrowse(wnd)) {
		((CEdit*)wnd)->SetSel(0, -1);
	}
}
