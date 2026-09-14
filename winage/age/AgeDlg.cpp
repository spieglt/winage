
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
		MessageBox(_T("Memory allocation error, aborting."), _T("Error"), MB_OK | MB_ICONERROR); \
		goto cleanup; \
	} \
}

// WideToUtf8 returns NULL only on a failed allocation or an unconvertible string, so it
// gets its own message rather than MALLOC_CHECK's.
#define UTF8_CHECK(ptr) { \
	if (ptr == NULL) { \
		MessageBox(_T("Could not convert a path or passphrase to UTF-8."), \
			_T("Error"), MB_OK | MB_ICONERROR); \
		goto cleanup; \
	} \
}

// The Rust side takes UTF-8. This is a Unicode build, so the front end holds UTF-16 and
// the conversion is lossless in both directions for every path Windows can name.
static char* WideToUtf8(const wchar_t* wide)
{
	if (wide == NULL) {
		return NULL;
	}
	// No WC_ERR_INVALID_CHARS: a lone surrogate in a filename is legal on NTFS and
	// should become U+FFFD rather than fail the whole operation.
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if (utf8Len <= 0) {
		return NULL;
	}
	char* utf8 = (char*)malloc(utf8Len);
	if (utf8 == NULL) {
		return NULL;
	}
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8Len, NULL, NULL);
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
		return WideToUtf8(dlg.passphrase);
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
		this->GetDlgItem(INPUT_FILE_SELECTOR)->SetWindowText(CommandLineArg(2));
	}
	if (__argc > 1 && !wcscmp(CommandLineArg(1), L"decrypt")) {
		this->SetWindowText(_T("age - Decrypt file"));
		this->GetDlgItem(ENCRYPT_LABEL)->SetWindowText(_T("Select file to decrypt"));
		this->GetDlgItem(ENCRYPT_BUTTON)->SetWindowText(_T("Decrypt"));
		this->GetDlgItem(PASSPHRASE_LABEL)->SetWindowText(_T("Enter passphrase"));
		this->GetDlgItem(INPUT_FILE_SELECTOR)->EnableWindow(false);
		this->GetDlgItem(IDC_ARMOR)->ShowWindow(false);
		this->GetDlgItem(RECIPIENT_LABEL)->SetWindowText(_T("Select identity file"));
		this->GetDlgItem(RADIO_IDENTITY_RECIPIENT)->SetWindowText(_T("Identity"));

		if (__argc > 2) { // second arg should be filename
			if (!PathFileExists(CommandLineArg(2))) {
				MessageBox(_T("Not a valid age file. Exiting."), _T("Invalid File"), MB_OK | MB_ICONERROR);
				EndDialog(IDCANCEL);
				return TRUE;
			}
			// detect and handle authentication mode
			char* pathUtf8 = WideToUtf8(CommandLineArg(2));
			char* mode = pathUtf8 != NULL ? get_decryption_mode(pathUtf8) : NULL;
			free(pathUtf8);
			BOOL isRecipients = mode != NULL && !strcmp(mode, "recipients");
			BOOL isPassphrase = mode != NULL && !strcmp(mode, "passphrase");
			// Anything else is the reason the file could not be read, and says more
			// than "not a valid age file" does.
			CStringW modeError;
			if (!isRecipients && !isPassphrase) {
				modeError = mode != NULL
					? Utf8ToWide(mode)
					: CStringW(L"Could not convert the file path to UTF-8.");
			}

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
				CString msg = _T("Could not read this age file.\n\n");
				msg += modeError;
				MessageBox(msg, _T("Invalid File"), MB_OK | MB_ICONERROR);
				EndDialog(IDCANCEL);
				return TRUE;
			}
		}
	}
	if (__argc > 1 && !wcscmp(CommandLineArg(1), L"generate")) {
		CFileDialog outputDiag(
			FALSE,
			_T("txt"),
			_T("age-identity"),
			OFN_OVERWRITEPROMPT,
			_T("Text files|*.txt||"),
			NULL,
			0,
			TRUE
		);
		INT_PTR outputRes = outputDiag.DoModal();
		if (outputRes == IDOK) {
			CString output = outputDiag.GetPathName();
			if (!output.IsEmpty()) {
				char* outputUtf8 = WideToUtf8(output);
				char* retMessage = outputUtf8 != NULL ? generate_identity(outputUtf8) : NULL;
				free(outputUtf8);
				if (retMessage != NULL && !strcmp(retMessage, "ok")) {
					CString msg = _T("Identity file successfully created at: ");
					msg += output;
					MessageBox(msg, _T("Identity created"), MB_OK | MB_ICONINFORMATION);
				}
				else {
					CString msg = _T("Error generating identity file: ");
					msg += Utf8ToWide(retMessage);
					MessageBox(msg, _T("Error"), MB_OK | MB_ICONERROR);
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
	LPTSTR outputName = NULL;
	CString rustMessage;
	struct COptions* cOptions = (struct COptions*)malloc(sizeof(struct COptions));
	MALLOC_CHECK(cOptions);

	BOOL encrypting = !(__argc > 1 && !(wcscmp(CommandLineArg(1), L"decrypt")));
	BOOL usingPassphrase = this->IsDlgButtonChecked(RADIO_PASSPHRASE);
	BOOL armor = this->IsDlgButtonChecked(IDC_ARMOR);

	// get and verify input filepath. Every size below this point counts characters, not
	// bytes; only the mallocs scale by sizeof(TCHAR).
	int pathSize = this->GetDlgItem(INPUT_FILE_SELECTOR)->GetWindowTextLength() + 1;
	size_t bigSize = pathSize;
	inputFile = (LPTSTR)malloc(pathSize * sizeof(TCHAR));
	MALLOC_CHECK(inputFile);
	this->GetDlgItem(INPUT_FILE_SELECTOR)->GetWindowText(inputFile, pathSize);
	if (inputFile[0] == _T('\0')) {
		MessageBox(_T("Must select file to encrypt or decrypt."), _T("No Input File Selected"), MB_OK | MB_ICONERROR);
		goto cleanup;
	}
	if (!PathFileExists(inputFile)) {
		MessageBox(_T("Input path does not point to a valid file."), _T("Must Select Input File"), MB_OK | MB_ICONERROR);
		goto cleanup;
	}

	// make default output filename
	if (encrypting) { // add ".age" extension
		outputName = (LPTSTR)malloc((bigSize + 4) * sizeof(TCHAR));
		MALLOC_CHECK(outputName);
		_tcscpy_s(outputName, bigSize + 4, inputFile);
		_tcscat_s(outputName, bigSize + 4, _T(".age"));
	} else { // chop ".age" extension if present
		// when counting backwards from end of string, must account for null byte;
		// the length check has to come first or the offset underflows
		if (bigSize > 5 && !_tcscmp(inputFile + (bigSize - 5), _T(".age"))) {
			outputName = (LPTSTR)malloc(bigSize * sizeof(TCHAR));
			MALLOC_CHECK(outputName);
			_tcsncpy_s(outputName, bigSize, inputFile, bigSize - 5);
		} else {
			LPCTSTR decrypted = _T(".decrypted");
			size_t decryptedLen = _tcslen(decrypted);
			outputName = (LPTSTR)malloc((bigSize + decryptedLen) * sizeof(TCHAR));
			MALLOC_CHECK(outputName);
			_tcscpy_s(outputName, bigSize + decryptedLen, inputFile);
			_tcscat_s(outputName, bigSize + decryptedLen, decrypted);
		}
	}

	// handle auth mode
	if (this->IsDlgButtonChecked(RADIO_IDENTITY_RECIPIENT)) {
		pathSize = this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->GetWindowTextLength() + 1;
		recipient = (LPTSTR)malloc(pathSize * sizeof(TCHAR));
		MALLOC_CHECK(recipient);
		this->GetDlgItem(RECIPIENT_FILE_SELECTOR)->GetWindowText(recipient, pathSize);

		if (recipient[0] == _T('\0')) {
			if (encrypting) {
				MessageBox(_T("Must paste a recipient's public key, specify a recipients file, or select file containing one or more identities."), _T("Missing Identity/Recipent"), MB_OK | MB_ICONERROR);
			}
			else {
				MessageBox(_T("Must select a file containing one or more identities."), _T("Missing Identity"), MB_OK | MB_ICONERROR);
			}
			goto cleanup;
		}
	}
	else if (this->IsDlgButtonChecked(RADIO_PASSPHRASE)) {
		pathSize = this->GetDlgItem(PASSPHRASE_BOX)->GetWindowTextLength() + 1;
		if (pathSize == 1 && encrypting) { // empty string, get generated password from rust
			char* generated = get_passphrase();
			if (generated == NULL) {
				MessageBox(_T("Could not generate a passphrase."), _T("Error"), MB_OK | MB_ICONERROR);
				goto cleanup;
			}
			// The wordlist is ASCII, so the UTF-8 length bounds the UTF-16 length.
			size_t generatedSize = strlen(generated) + 1;
			passphrase = (LPTSTR)malloc(generatedSize * sizeof(TCHAR));
			if (passphrase != NULL) {
				CStringW wide = Utf8ToWide(generated);
				_tcscpy_s(passphrase, generatedSize, wide);
				SecureZeroMemory(wide.GetBuffer(), wide.GetLength() * sizeof(TCHAR));
				wide.ReleaseBuffer(0);
			}
			free_rust_string(generated); // zeroes the passphrase
			MALLOC_CHECK(passphrase);
			GenPassDlg gpd(passphrase);
			gpd.DoModal();
		}
		else {
			passphrase = (LPTSTR)malloc(pathSize * sizeof(TCHAR));
			MALLOC_CHECK(passphrase);
			this->GetDlgItem(PASSPHRASE_BOX)->GetWindowText(passphrase, pathSize);
			if (encrypting) { // confirm password
				ConfirmPassDlg confirmDlg;
				if (confirmDlg.DoModal() != IDOK) {
					goto cleanup;
				}
				BOOL matched = !_tcscmp(passphrase, confirmDlg.confirmedPass);
				SecureZeroMemory(confirmDlg.confirmedPass.GetBuffer(),
					confirmDlg.confirmedPass.GetLength() * sizeof(TCHAR));
				confirmDlg.confirmedPass.ReleaseBuffer(0);
				if (!matched) {
					MessageBox(_T("Passphrases do not match."), _T("Mismatched Passphrase"), MB_OK | MB_ICONERROR);
					goto cleanup;
				}
			}
		}

		if (!encrypting && (passphrase == NULL || passphrase[0] == _T('\0'))) {
			MessageBox(_T("Must provide decryption passphrase."), _T("Missing Passphrase"), MB_OK | MB_ICONERROR);
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
			_T("age"),
			outputName,
			OFN_OVERWRITEPROMPT,
			_T("age files|*.age||"),
			NULL,
			0,
			TRUE
		);
	}
	INT_PTR outputRes = outputDiag->DoModal();
	if (outputRes == IDOK) {
		output = outputDiag->GetPathName();
		if (output.IsEmpty()) {
			goto cleanup;
		}
	}
	else {
		goto cleanup;
	}

	// fill out options for rust, converting strings to UTF-8
	inputUtf8 = WideToUtf8(inputFile);
	UTF8_CHECK(inputUtf8);
	outputUtf8 = WideToUtf8(output);
	UTF8_CHECK(outputUtf8);
	if (passphrase != NULL) {
		passphraseUtf8 = WideToUtf8(passphrase);
		UTF8_CHECK(passphraseUtf8);
	}
	memset(cOptions, 0, sizeof(struct COptions));
	cOptions->input = inputUtf8;
	cOptions->encrypt = encrypting;
	cOptions->using_passphrase = usingPassphrase;
	cOptions->passphrase = passphraseUtf8;
	cOptions->max_work_factor = 0;
	cOptions->armor = armor;
	cOptions->output = outputUtf8;

	if (recipient != NULL && recipient[0] != _T('\0')) {
		recipientUtf8 = WideToUtf8(recipient);
		UTF8_CHECK(recipientUtf8);
		if (PathFileExists(recipient)) {
			cOptions->recipient_or_identity_file = recipientUtf8;
		}
		else {
			cOptions->recipient = recipientUtf8;
		}
	}

	// change window title to indicate we're busy
	if (encrypting) {
		this->SetWindowText(_T("age - Encrypting..."));
	} else {
		this->SetWindowText(_T("age - Decrypting..."));
	}

	// call main rust routine
	char* res = wrapper(cOptions);
	rustMessage = Utf8ToWide(res);
	MessageBox(rustMessage, _T("Message"), MB_OK);
	free_rust_string(res);

	// change title back
	if (encrypting) {
		this->SetWindowText(_T("age"));
	} else {
		this->SetWindowText(_T("age - Decrypt file"));
	}

cleanup:
	if (passphrase != NULL) {
		SecureZeroMemory(passphrase, _tcslen(passphrase) * sizeof(TCHAR));
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
	if (!rustMessage.Left(7).Compare(_T("Success"))) {
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
	LPCTSTR aboutMsg =
		_T("age is a simple, modern and secure file encryption tool, format, and Go library.\r\n\r\n")
		_T("To generate a new identity, right-click a folder background and select \"Generate new age identity\".\r\n")
		_T("To encrypt a file, right-click it and select \"Encrypt with age\".\r\n")
		_T("To decrypt a file, double-click it and enter the passphrase or select the identity file.\r\n")
		_T("SSH private and public keys may be used in place of native age identities and recipients.\r\n\r\n")
		_T("age (original Go implementation by Ben Cartwright-Cox and Filippo Valsorda): https://age-encryption.org\r\n")
		_T("rage (Rust implementation on which this is based, by Jack Grigg): https://str4d.xyz/rage\r\n")
		_T("winage (this project): https://winage.spiegl.dev");
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
