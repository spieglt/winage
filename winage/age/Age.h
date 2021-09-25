
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

static BOOL IsEditOrEditBrowse(CWnd* pWnd)
{
	if (!pWnd) return FALSE;
	HWND hWnd = pWnd->GetSafeHwnd();
	if (hWnd == NULL)
		return FALSE;

	char* editName = "Edit";
	char* ebcName = "MFCEditBrowse";
	char className[14];
	return ::GetClassName(hWnd, className, 14) &&
		(!_tcsicmp(className, editName) || !_tcsicmp(className, ebcName));
}
