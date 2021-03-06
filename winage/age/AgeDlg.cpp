
// AgeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Age.h"
#include "AgeDlg.h"
#include "afxdialogex.h"
#include "winuser.h"
#include "ConfirmPassDlg.h"
#include "GenPassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define MALLOC_CHECK(ptr) { \
	if (ptr == NULL) { \
		MessageBox("Memory allocation error, aborting.", "Error", MB_OK | MB_ICONERROR); \
		goto cleanup; \
	} \
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
	ON_EN_CHANGE(PASSPHRASE_BOX, &CAgeDlg::OnEnChangeBox)
	ON_BN_CLICKED(RADIO_PASSPHRASE, &CAgeDlg::OnBnClickedPassphrase)
	ON_BN_CLICKED(RADIO_IDENTITY_RECIPIENT, &CAgeDlg::OnBnClickedIdentityRecipient)
	ON_BN_CLICKED(ENCRYPT_LABEL, &CAgeDlg::OnBnClickedLabel)
	ON_BN_CLICKED(IDC_ARMOR, &CAgeDlg::OnBnClickedArmor)
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
				exit(1);
			}
			// detect and handle authentication mode
			char* mode = get_decryption_mode(__argv[2]);
			if (!strcmp(mode, "recipients")) {
				this->CheckDlgButton(RADIO_IDENTITY_RECIPIENT, BST_CHECKED);
				this->CheckDlgButton(RADIO_PASSPHRASE, BST_UNCHECKED);
				this->GetDlgItem(RADIO_PASSPHRASE)->EnableWindow(false);
				this->OnBnClickedIdentityRecipient();
			}
			else if (!strcmp(mode, "passphrase")) {
				this->CheckDlgButton(RADIO_PASSPHRASE, BST_CHECKED);
				this->CheckDlgButton(RADIO_IDENTITY_RECIPIENT, BST_UNCHECKED);
				this->GetDlgItem(RADIO_IDENTITY_RECIPIENT)->EnableWindow(false);
				this->OnBnClickedPassphrase();
			}
			else { // error
				MessageBox("Not a valid age file. Exiting.", "Invalid File", MB_OK | MB_ICONERROR);
				exit(1);
			}
			free_rust_string(mode);
		}
	}
	if (__argc > 1 && !strcmp(__argv[1], "generate")) {
		CFileDialog* outputDiag = new CFileDialog(
			FALSE,
			"txt",
			"age-identity",
			OFN_OVERWRITEPROMPT,
			"Text files|*.txt||",
			NULL,
			0,
			TRUE
		);
		INT_PTR outputRes = outputDiag->DoModal();
		if (outputRes == IDOK) {
			CString output = outputDiag->GetPathName();
			if (strcmp(output.GetBuffer(), "")) {
				char* retMessage = generate_identity(output.GetBuffer());
				if (!strcmp(retMessage, "ok")) {
					CString msg = "Identity file successfully created at: ";
					msg += output;
					MessageBox(msg, "Identity created", MB_OK | MB_ICONINFORMATION);
				}
				else {
					CString msg = "Error generating identity file: ";
					msg += retMessage;
					MessageBox(msg, "Error", MB_OK | MB_ICONERROR);
				}
				free_rust_string(retMessage);
			}
		}
		exit(0);
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
		LPTSTR lastFour = inputFile + (bigSize - 5); // when counting backwards from end of string, must account for null byte
		if (bigSize > 5 && !strcmp(lastFour, ".age")) {
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
			passphrase = (LPTSTR)malloc(strlen(generated) + 1);
			MALLOC_CHECK(passphrase);
			strcpy_s(passphrase, strlen(generated) + 1, generated);
			free_rust_string(generated);
			GenPassDlg gpd(passphrase);
			gpd.DoModal();
		}
		else {
			passphrase = (LPTSTR)malloc(pathSize);
			MALLOC_CHECK(passphrase);
			this->GetDlgItem(PASSPHRASE_BOX)->GetWindowText(passphrase, pathSize);
			if (encrypting) { // confirm password
				ConfirmPassDlg confirmDlg = new ConfirmPassDlg;
				if (confirmDlg.DoModal() != IDOK) {
					goto cleanup;
				}
				else if (strcmp(passphrase, confirmDlg.confirmedPass.GetBuffer())) {
					MessageBox("Passphrases do not match.", "Mismatched Passphrase", MB_OK | MB_ICONERROR);
					goto cleanup;
				}
			}
		}

		if (!encrypting && !strcmp(passphrase, "")) {
			MessageBox("Must provide decryption passphrase.", "Missing Passphrase", MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	// select output filename
	CFileDialog* outputDiag = NULL;
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

	// fill out options for rust
	memset(cOptions, 0, sizeof(struct COptions));
	cOptions->input = inputFile;
	cOptions->encrypt = encrypting;
	cOptions->using_passphrase = usingPassphrase;
	cOptions->passphrase = passphrase;
	cOptions->max_work_factor = 0;
	cOptions->armor = armor;
	cOptions->output = output.GetBuffer();

	if (recipient != NULL && strcmp(recipient, "")) {
		if (PathFileExists(recipient)) {
			cOptions->recipients_file = recipient;
		}
		else {
			cOptions->recipient = recipient;
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
	MessageBox(res, "Message", MB_OK);
	free_rust_string(res);

	// change title back
	if (encrypting) {
		this->SetWindowText("age");
	} else {
		this->SetWindowText("age - Decrypt file");
	}

cleanup:
	free(inputFile);
	free(outputName);
	free(recipient);
	free(passphrase);
	free(cOptions);
	if (!rustMessage.Left(7).Compare("Success")) {
		exit(0);
	}

}


void CAgeDlg::OnEnChangeBox()
{
	// If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// Add your control notification handler code here
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


void CAgeDlg::OnBnClickedLabel()
{
}


void CAgeDlg::OnEnChangeFileSelector()
{
	// If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// Add your control notification handler code here
}

void CAgeDlg::OnBnClickedArmor()
{
	// Add your control notification handler code here
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
	return CDialogEx::PreTranslateMessage(pMsg);
}
