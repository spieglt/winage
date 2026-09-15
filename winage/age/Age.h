
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

// MFC enters through wWinMain in a Unicode build, so the CRT fills in __wargv. Guard
// anyway and hand back an empty string rather than null: everything winage does with
// argv arrives from the shell integration, so a null here would break the common path
// instead of failing visibly.
inline LPCWSTR CommandLineArg(int index)
{
	return (__wargv != NULL && index >= 0 && index < __argc) ? __wargv[index] : L"";
}

inline BOOL IsEditOrEditBrowse(CWnd* pWnd)
{
	if (!pWnd) return FALSE;
	HWND hWnd = pWnd->GetSafeHwnd();
	if (hWnd == NULL)
		return FALSE;

	LPCTSTR editName = _T("Edit");
	LPCTSTR ebcName = _T("MFCEditBrowse");
	TCHAR className[14];
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
