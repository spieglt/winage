
// AgeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Age.h"
#include "AgeDlg.h"
#include "afxdialogex.h"
#include "winuser.h"
#include "ConfirmPassDlg.h"
#include "GenPassDlg.h"
#include "IdentityPassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define MALLOC_CHECK(ptr) { \
	if (ptr == NULL) { \
		MessageBox("Memory allocation error, aborting.", "Error", MB_OK | MB_ICONERROR); \
		goto cleanup; \
	} \
}

// The Rust side expects UTF-8; this MBCS build's strings are in the ANSI codepage.
static char* AnsiToUtf8(const char* ansi)
{
	if (ansi == NULL) {
		return NULL;
	}
	int wideLen = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
	if (wideLen <= 0) {
		return NULL;
	}
	wchar_t* wide = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
	if (wide == NULL) {
		return NULL;
	}
	MultiByteToWideChar(CP_ACP, 0, ansi, -1, wide, wideLen);
	char* utf8 = NULL;
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if (utf8Len > 0) {
		utf8 = (char*)malloc(utf8Len);
		if (utf8 != NULL) {
			WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8Len, NULL, NULL);
		}
	}
	SecureZeroMemory(wide, wideLen * sizeof(wchar_t)); // may hold a passphrase
	free(wide);
	return utf8;
}

static CStringW Utf8ToWide(const char* utf8)
{
	CStringW result;
	if (utf8 == NULL) {
		return result;
	}
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (wideLen <= 0) {
		return result;
	}
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.GetBuffer(wideLen), wideLen);
	result.ReleaseBuffer();
	return result;
}

// age asks for these while Rust frames are on the stack, which C++ exceptions must not
// unwind through.

static void AgeDisplayMessage(const char* message)
{
	try {
		CWnd* main = AfxGetMainWnd();
		MessageBoxW(main != NULL ? main->GetSafeHwnd() : NULL, Utf8ToWide(message),
			L"age", MB_OK | MB_ICONINFORMATION);
	}
	catch (...) {
	}
}

static int AgeConfirm(const char* message, const char* yes, const char* no)
{
	try {
		// A message box can't relabel its buttons, so the choices go in the body.
		CStringW body = Utf8ToWide(message);
		if (yes != NULL) {
			body += L"\n\nYes: ";
			body += Utf8ToWide(yes);
		}
		if (no != NULL) {
			body += L"\nNo: ";
			body += Utf8ToWide(no);
		}
		CWnd* main = AfxGetMainWnd();
		int response = MessageBoxW(main != NULL ? main->GetSafeHwnd() : NULL, body,
			L"age", MB_YESNO | MB_ICONQUESTION);
		return response == IDYES ? 1 : 0;
	}
	catch (...) {
		return -1;
	}
}

static char* AgeRequestPassphrase(const char* description)
{
	try {
		IdentityPassDlg dlg(Utf8ToWide(description));
		if (dlg.DoModal() != IDOK) {
			return NULL; // cancelled
		}
		return AnsiToUtf8(dlg.passphrase);
	}
	catch (...) {
		return NULL;
	}
}

static void AgeFreeString(char* s)
{
	if (s != NULL) {
		SecureZeroMemory(s, strlen(s));
		free(s);
	}
}

void RegisterAgeCallbacks()
{
	static const CCallbacks callbacks = {
		AgeDisplayMessage,
		AgeConfirm,
		AgeRequestPassphrase,
		AgeFreeString,
	};
	set_callbacks(&callbacks);
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CAgeDlg dialog



CAgeDlg::CAgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AGE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAgeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAgeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ENCRYPT_BUTTON, &CAgeDlg::OnBnClickedButton)
	ON_BN_CLICKED(RADIO_PASSPHRASE, &CAgeDlg::OnBnClickedPassphrase)
	ON_BN_CLICKED(RADIO_IDENTITY_RECIPIENT, &CAgeDlg::OnBnClickedIdentityRecipient)
END_MESSAGE_MAP()


// CAgeDlg message handlers

BOOL CAgeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Add extra initialization here
	this->CheckDlgButton(RADIO_PASSPHRASE, BST_CHECKED);
	if (__argc > 2) {
		this->GetDlgItem(INPUT_FILE_SELECTOR)->SetWindowText(__argv[2]);
	}
	if (__argc > 1 && !strcmp(__argv[1], "decrypt")) {
		this->SetWindowText("age - Decrypt file");
		this->GetDlgItem(ENCRYPT_LABEL)->SetWindowText("Select file to decrypt");
		this->GetDlgItem(ENCRYPT_BUTTON)->SetWindowText("Decrypt");
		this->GetDlgItem(PASSPHRASE_LABEL)->SetWindowText("Enter passphrase");
		this->GetDlgItem(INPUT_FILE_SELECTOR)->EnableWindow(false);
		this->GetDlgItem(IDC_ARMOR)->ShowWindow(false);
		this->GetDlgItem(RECIPIENT_LABEL)->SetWindowText("Select identity file");
		this->GetDlgItem(RADIO_IDENTITY_RECIPIENT)->SetWindowText("Identity");

		if (__argc > 2) { // second arg should be filename
			if (!PathFileExists(__argv[2])) {
				MessageBox("Not a valid age file. Exiting.", "Invalid File", MB_OK | MB_ICONERROR);
				EndDialog(IDCANCEL);
				return TRUE;
			}
			// detect and handle authentication mode
			char* pathUtf8 = AnsiToUtf8(__argv[2]);
			char* mode = pathUtf8 != NULL ? get_decryption_mode(pathUtf8) : NULL;
			free(pathUtf8);
			BOOL isRecipients = mode != NULL && !strcmp(mode, "recipients");
			BOOL isPassphrase = mode != NULL && !strcmp(mode, "passphrase");
			free_rust_string(mode);
			if (isRecipients) {
				this->CheckDlgButton(RADIO_IDENTITY_RECIPIENT, BST_CHECKED);
				this->CheckDlgButton(RADIO_PASSPHRASE, BST_UNCHECKED);
				this->GetDlgItem(RADIO_PASSPHRASE)->EnableWindow(false);
				this->OnBnClickedIdentityRecipient();
			}
			else if (isPassphrase) {
				this->CheckDlgButton(RADIO_PASSPHRASE, BST_CHECKED);
				this->CheckDlgButton(RADIO_IDENTITY_RECIPIENT, BST_UNCHECKED);
				this->GetDlgItem(RADIO_IDENTITY_RECIPIENT)->EnableWindow(false);
				this->OnBnClickedPassphrase();
			}
			else { // error
				MessageBox("Not a valid age file. Exiting.", "Invalid File", MB_OK | MB_ICONERROR);
				EndDialog(IDCANCEL);
				return TRUE;
			}
		}
	}
	if (__argc > 1 && !strcmp(__argv[1], "generate")) {
		CFileDialog outputDiag(
			FALSE,
			"txt",
			"age-identity",
			OFN_OVERWRITEPROMPT,
			"Text files|*.txt||",
			NULL,
			0,
			TRUE
		);
		INT_PTR outputRes = outputDiag.DoModal();
		if (outputRes == IDOK) {
			CString output = outputDiag.GetPathName();
			if (strcmp(output.GetBuffer(), "")) {
				char* outputUtf8 = AnsiToUtf8(output.GetBuffer());
				char* retMessage = outputUtf8 != NULL ? generate_identity(outputUtf8) : NULL;
				free(outputUtf8);
				if (retMessage != NULL && !strcmp(retMessage, "ok")) {
					CString msg = "Identity file successfully created at: ";
					msg += output;
					MessageBox(msg, "Identity created", MB_OK | MB_ICONINFORMATION);
				}
				else {
					CStringW msg = L"Error generating identity file: ";
					msg += Utf8ToWide(retMessage);
					MessageBoxW(this->m_hWnd, msg, L"Error", MB_OK | MB_ICONERROR);
				}
				free_rust_string(retMessage);
			}
		}
		// generate mode never shows the main dialog
		EndDialog(IDOK);
		return TRUE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CAgeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAgeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAgeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CAgeDlg::OnBnClickedButton()
{
	LPTSTR recipient = NULL;
	LPTSTR inputFile = NULL;
	LPTSTR passphrase = NULL;
	char* inputUtf8 = NULL;
	char* outputUtf8 = NULL;
	char* passphraseUtf8 = NULL;
	char* recipientUtf8 = NULL;
	CFileDialog* outputDiag = NULL;
	CString output;
	char* outputName = NULL;
	CString rustMessage = "";
	struct COptions* cOptions = (struct COptions*)malloc(sizeof(struct COptions));
	MALLOC_CHECK(cOptions);

	BOOL encrypting = !(__argc > 1 && !(strcmp(__argv[1], "decrypt")));
	BOOL usingPassphrase = this->IsDlgButtonChecked(RADIO_PASSPHRASE);
	BOOL armor = this->IsDlgButtonChecked(IDC_ARMOR);

	// get and verify input filepath
	int pathSize = this->GetDlgItem(INPUT_FILE_SELECTOR)->GetWindowTextLength() + 1;
	size_t bigSize = pathSize;
	inputFile = (LPTSTR)malloc(pathSize);
	MALLOC_CHECK(inputFile);
	this->GetDlgItem(INPUT_FILE_SELECTOR)->GetWindowText(inputFile, pathSize);
	if (!strcmp(inputFile, "")) {
		MessageBox("Must select file to encrypt or decrypt.", "No Input File Selected", MB_OK | MB_ICONERROR);
		goto cleanup;
	}
	if (!PathFileExists(inputFile)) {
		MessageBox("Input path does not point to a valid file.", "Must Select Input File", MB_OK | MB_ICONERROR);
		goto cleanup;
	}

	// make default output filename
	if (encrypting) { // add ".age" extension
		outputName = (char*)malloc(bigSize + 4);
		MALLOC_CHECK(outputName);
		strcpy_s(outputName, bigSize + 4, inputFile);
		strcat_s(outputName, bigSize + 4, ".age");
	} else { // chop ".age" extension if present
		// when counting backwards from end of string, must account for null byte;
		// the length check has to come first or the offset underflows
		if (bigSize > 5 && !strcmp(inputFile + (bigSize - 5), ".age")) {
			outputName = (char*)malloc(bigSize);
			MALLOC_CHECK(outputName);
			strncpy_s(outputName, bigSize, inputFile, bigSize - 5);
		} else {
			char* decrypted = ".decrypted";
			size_t decryptedLen = strlen(decrypted);
			outputName = (char*)malloc(bigSize + decryptedLen);
			MALLOC_CHECK(outputName);
			strcpy_s(outputName, bigSize + decryptedLen, inputFile);
			strcat_s(outputName, bigSize + decryptedLen, decrypted);
		}
	}

	// handle auth mode
	if (this->IsDlgButtonChecked(RADIO_IDENTITY_RECIPIENT)) {
		pathSize = this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->GetWindowTextLength() + 1;
		recipient = (LPTSTR)malloc(pathSize);
		MALLOC_CHECK(recipient);
		this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->GetWindowText(recipient, pathSize);

		if (!strcmp(recipient, "")) {
			if (encrypting) {
				MessageBox("Must paste a recipient's public key, specify a recipients file, or select file containing one or more identities.", "Missing Identity/Recipent", MB_OK | MB_ICONERROR);
			}
			else {
				MessageBox("Must select a file containing one or more identities.", "Missing Identity", MB_OK | MB_ICONERROR);
			}
			goto cleanup;
		}
	}
	else if (this->IsDlgButtonChecked(RADIO_PASSPHRASE)) {
		pathSize = this->GetDlgItem(PASSPHRASE_BOX)->GetWindowTextLength() + 1;
		if (pathSize == 1 && encrypting) { // empty string, get generated password from rust
			char* generated = get_passphrase();
			if (generated == NULL) {
				MessageBox("Could not generate a passphrase.", "Error", MB_OK | MB_ICONERROR);
				goto cleanup;
			}
			size_t generatedSize = strlen(generated) + 1;
			passphrase = (LPTSTR)malloc(generatedSize);
			if (passphrase != NULL) {
				strcpy_s(passphrase, generatedSize, generated);
			}
			free_rust_string(generated); // zeroes the passphrase
			MALLOC_CHECK(passphrase);
			GenPassDlg gpd(passphrase);
			gpd.DoModal();
		}
		else {
			passphrase = (LPTSTR)malloc(pathSize);
			MALLOC_CHECK(passphrase);
			this->GetDlgItem(PASSPHRASE_BOX)->GetWindowText(passphrase, pathSize);
			if (encrypting) { // confirm password
				ConfirmPassDlg confirmDlg;
				if (confirmDlg.DoModal() != IDOK) {
					goto cleanup;
				}
				BOOL matched = !strcmp(passphrase, confirmDlg.confirmedPass.GetBuffer());
				SecureZeroMemory(confirmDlg.confirmedPass.GetBuffer(), confirmDlg.confirmedPass.GetLength());
				confirmDlg.confirmedPass.ReleaseBuffer(0);
				if (!matched) {
					MessageBox("Passphrases do not match.", "Mismatched Passphrase", MB_OK | MB_ICONERROR);
					goto cleanup;
				}
			}
		}

		if (!encrypting && (passphrase == NULL || !strcmp(passphrase, ""))) {
			MessageBox("Must provide decryption passphrase.", "Missing Passphrase", MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	// select output filename
	if (!encrypting) {
		outputDiag = new CFileDialog(
			false,
			NULL,
			outputName,
			OFN_OVERWRITEPROMPT,
			NULL,
			NULL,
			0,
			TRUE
		);
	} else {
		outputDiag = new CFileDialog(
			false,
			"age",
			outputName,
			OFN_OVERWRITEPROMPT,
			"age files|*.age||",
			NULL,
			0,
			TRUE
		);
	}
	INT_PTR outputRes = outputDiag->DoModal();
	if (outputRes == IDOK) {
		output = outputDiag->GetPathName();
		if (!strcmp(output, "")) {
			goto cleanup;
		}
	}
	else {
		goto cleanup;
	}

	// fill out options for rust, converting strings to UTF-8
	inputUtf8 = AnsiToUtf8(inputFile);
	MALLOC_CHECK(inputUtf8);
	outputUtf8 = AnsiToUtf8(output.GetBuffer());
	MALLOC_CHECK(outputUtf8);
	if (passphrase != NULL) {
		passphraseUtf8 = AnsiToUtf8(passphrase);
		MALLOC_CHECK(passphraseUtf8);
	}
	memset(cOptions, 0, sizeof(struct COptions));
	cOptions->input = inputUtf8;
	cOptions->encrypt = encrypting;
	cOptions->using_passphrase = usingPassphrase;
	cOptions->passphrase = passphraseUtf8;
	cOptions->max_work_factor = 0;
	cOptions->armor = armor;
	cOptions->output = outputUtf8;

	if (recipient != NULL && strcmp(recipient, "")) {
		recipientUtf8 = AnsiToUtf8(recipient);
		MALLOC_CHECK(recipientUtf8);
		if (PathFileExists(recipient)) {
			cOptions->recipient_or_identity_file = recipientUtf8;
		}
		else {
			cOptions->recipient = recipientUtf8;
		}
	}

	// change window title to indicate we're busy
	if (encrypting) {
		this->SetWindowText("age - Encrypting...");
	} else {
		this->SetWindowText("age - Decrypting...");
	}

	// call main rust routine
	char* res = wrapper(cOptions);
	rustMessage = res;
	MessageBoxW(this->m_hWnd, Utf8ToWide(res), L"Message", MB_OK);
	free_rust_string(res);

	// change title back
	if (encrypting) {
		this->SetWindowText("age");
	} else {
		this->SetWindowText("age - Decrypt file");
	}

cleanup:
	if (passphrase != NULL) {
		SecureZeroMemory(passphrase, strlen(passphrase));
	}
	if (passphraseUtf8 != NULL) {
		SecureZeroMemory(passphraseUtf8, strlen(passphraseUtf8));
	}
	free(inputFile);
	free(outputName);
	free(recipient);
	free(passphrase);
	free(inputUtf8);
	free(outputUtf8);
	free(passphraseUtf8);
	free(recipientUtf8);
	free(cOptions);
	delete outputDiag;
	if (!rustMessage.Left(7).Compare("Success")) {
		EndDialog(IDOK);
	}

}


void CAgeDlg::OnBnClickedPassphrase()
{
	this->GetDlgItem(PASSPHRASE_BOX)->ShowWindow(SW_SHOW);
	this->GetDlgItem(PASSPHRASE_LABEL)->ShowWindow(SW_SHOW);
	this->GetDlgItem(RECIPIENT_LABEL)->ShowWindow(SW_HIDE);
	this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->ShowWindow(SW_HIDE);
}


void CAgeDlg::OnBnClickedIdentityRecipient()
{
	this->GetDlgItem(PASSPHRASE_BOX)->ShowWindow(SW_HIDE);
	this->GetDlgItem(PASSPHRASE_LABEL)->ShowWindow(SW_HIDE);
	this->GetDlgItem(RECIPIENT_LABEL)->ShowWindow(SW_SHOW);
	this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->ShowWindow(SW_SHOW);
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	char* aboutMsg =
		"age is a simple, modern and secure file encryption tool, format, and Go library.\r\n\r\n"
		"To generate a new identity, right-click a folder background and select \"Generate new age identity\".\r\n"
		"To encrypt a file, right-click it and select \"Encrypt with age\".\r\n"
		"To decrypt a file, double-click it and enter the passphrase or select the identity file.\r\n"
		"SSH private and public keys may be used in place of native age identities and recipients.\r\n\r\n"
		"age (original Go implementation by Ben Cartwright-Cox and Filippo Valsorda): https://age-encryption.org\r\n"
		"rage (Rust implementation on which this is based, by Jack Grigg): https://str4d.xyz/rage\r\n"
		"winage (this project): https://winage.spiegl.dev";
	this->GetDlgItem(IDC_ABOUT_MSG)->SetWindowText(aboutMsg);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAgeDlg::PreTranslateMessage(MSG* pMsg)
{
	if (IsSelectAllKey(pMsg)) {
		SelectAllInFocusedEdit();
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
