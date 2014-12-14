/*
	WinClock(tm) v1.2

	Copyright (c) 1993  Patrick Breen
	All rights reserved.

	How to reach me:

		Patrick Breen
		3920 Mystic Valley Parkway #1119
		Medford, MA 02155

		(617) 396-2673

		Internet:		pbreen@world.std.com
		CompuServe: 	70312,743
*/

#include "winclock.h"

// Global data for all bar information
BarData barData;

// Application instance
HINSTANCE hBarInstance;

// Forward declaration of static functions
void NEAR ReadProfile(PBarData pData);
void NEAR WriteProfile(PBarData pData);
void NEAR SetProfileInt(char FAR *pSection, short n);

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				LPSTR lpCmd, int nCmdShow)
{
	WNDPROC lpfnWndProc;
	WNDCLASS WindowClass;
	HINSTANCE hUser;
	HINSTANCE hKernel;
	HWND hBarWnd;
	MSG msg;

#ifdef USE_FONT
	HINSTANCE hCommDlg;
	OFSTRUCT of;
#endif

	// We can only have one instance running
	if (hPrevInstance != NULLHINSTANCE)
		return (0);

	// Set the global instance
	hBarInstance = hInstance;

	// Clear the data structure
	memset(&barData, 0, sizeof(BarData));

	// Load all dynamic functions that we need
#ifdef USE_FONT
	if ((OpenFile("commdlg.dll", &of, OF_EXIST) != HFILE_ERROR) &&
	    ((hCommDlg = LoadLibrary("COMMDLG.DLL")) > 32)) {

		(FARPROC) (barData.lpfnChooseFont) = GetProcAddress(hCommDlg, "ChooseFont");
	}
#endif

	if ((hUser = LoadLibrary("USER.DLL")) > 32) {

		(FARPROC) (barData.lpfnRedrawWindow) = GetProcAddress(hUser, "RedrawWindow");
	}

	if ((hKernel = LoadLibrary("KERNEL.DLL")) > 32) {

		(FARPROC) (barData.lpfnGetHeapSpaces) = GetProcAddress(hKernel, "GetHeapSpaces");
	}

	// Create the window procedure instance
	lpfnWndProc = (WNDPROC) MakeProcInstance((FARPROC) WndProc, hInstance);

	// If we could create the instance
	if (lpfnWndProc != NULL) {

		// Define the bar window class
		memset((void *) &WindowClass, 0, sizeof(WindowClass));
		WindowClass.lpfnWndProc   = lpfnWndProc;
		WindowClass.hInstance     = hInstance;
		WindowClass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		WindowClass.lpszClassName = IDN_WINDOW;

		// Register the window class
		if (RegisterClass(&WindowClass)) {

			// Create the bar window
			hBarWnd = CreateWindow(IDN_WINDOW, "",
							   WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME,
							   0, 0,
							   400, -4,
							   NULL, NULL,
							   hInstance, NULL );

			// If we could create the bar window
			if (hBarWnd != NULLHWND) {

				// Create the popup menu -
				// The contents of the menu is based upon
				// the availability of commdlg.dll - if this
				// DLL is present, extra options are added
				if ((barData.hMenu = CreatePopupMenu()) != NULLHMENU) {

					// Add the menu items to the menu
					AppendMenu(barData.hMenu, MF_ENABLED, ID_MENUSETUP, "Setup...");

#ifdef USE_FONT
					// Changing fonts is only supported if
					// the commdlg.dll exists - this saves
					// me from creating a font dialog
					if (barData.lpfnChooseFont != NULL) {
						AppendMenu(barData.hMenu, MF_ENABLED, ID_MENUFONT, "Font...");
					}

					// These items always exist
					AppendMenu(barData.hMenu, MF_SEPARATOR, 0, 0);
#endif

					AppendMenu(barData.hMenu, MF_ENABLED, ID_MENUABOUT, "About...");
					AppendMenu(barData.hMenu, MF_SEPARATOR, 0, 0);
					AppendMenu(barData.hMenu, MF_ENABLED, ID_MENUEXIT, "Exit");

					// Initialize the bar data
					ReadProfile(&barData);

					// Install the message filter that allows
					// us to detect a mouse down within the bar
					if (InstallFilter(hBarWnd, &barData)) {

						// Start the message loop
						while (GetMessage(&msg, NULL, NULL, NULL)) {
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}

					// Failed to install filter, back out
					} else {
						DestroyWindow(hBarWnd);
					}

					// Destroy the menu
					DestroyMenu(barData.hMenu);

				// Failed to create the menu, back out
				} else {
					DestroyWindow(hBarWnd);
				}
			}

			// All done with the class
			UnregisterClass(IDN_WINDOW, hInstance);
		}

		// Free the procedure instance
		FreeProcInstance((FARPROC) lpfnWndProc);
	}

	// Free all library handles
	if (hUser > 32) FreeLibrary(hUser);
	if (hKernel > 32) FreeLibrary(hKernel);
#ifdef USE_FONT
	if (hCommDlg > 32) FreeLibrary(hCommDlg);
#endif

	// Return something
	return (msg.wParam);
}


#ifdef USE_FONT
static void NEAR CacheFont(void)
{
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));

	lf.lfHeight = barData.fontSize;
	lf.lfItalic = barData.bItalic;
	lf.lfWeight = barData.fontWeight;
	strcpy(lf.lfFaceName, barData.fontName);

	if (barData.hFont != NULLHFONT)
		DeleteObject(barData.hFont);

	barData.hFont = CreateFontIndirect(&lf);
	return;
}


static void NEAR SetFont(void)
{
	CHOOSEFONT cf;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));

	lf.lfHeight = barData.fontSize;
	lf.lfItalic = barData.bItalic;
	lf.lfWeight = barData.fontWeight;
	strcpy(lf.lfFaceName, barData.fontName);

	memset(&cf, 0, sizeof(CHOOSEFONT));

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.lpLogFont = &lf;
	cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	cf.nFontType = SCREEN_FONTTYPE;

	if ((barData.lpfnChooseFont != NULL) &&
	    (*(barData.lpfnChooseFont))(&cf)) {

		barData.fontSize = lf.lfHeight;
		barData.bItalic = lf.lfItalic;
		barData.fontWeight = lf.lfWeight;
		strcpy(barData.fontName, lf.lfFaceName);

		WriteProfile(&barData);
		CacheFont();
	}

	return;
}
#endif


LRESULT FAR PASCAL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DLGPROC dlgprc;

	switch (msg) {

		case WM_CREATE:
			// Create a timer with a resolution of 1 second -
			// this is the guts of the clock
			SetTimer(hWnd, 1, 1000, NULL);
			break;

		case WM_TIMER:
			// Draw the contents of the bar
			DrawBar();
			break;

		case WM_COMMAND:
			// If this is a command
			// (either from the menu or one
			//  that we sent to ourselves)
			if (HIWORD(lParam) == 0) {

				// Switch on menu ID
				switch (wParam) {

					case ID_MENUSETUP:
					case ID_MENUABOUT:
					case ID_MENUEXIT:
						// Post ourselves a command so
						// that we don't crash when we
						// destroy ourselves
						PostMessage(hWnd, WM_COMMAND, wParam - 100, 0);
						break;

					case ID_SHOWMENU:
						// Show the menu to the user
						DisplayMenu(hWnd);
						break;

					case ID_ABOUT:
						// Show the about box
						dlgprc = (DLGPROC) MakeProcInstance(About, hBarInstance);
						DialogBox(hBarInstance, "About", NULLHWND, dlgprc);
						FreeProcInstance((FARPROC) dlgprc);
						break;

					case ID_SETUP:
						// Show the setup dialog
						dlgprc = (DLGPROC) MakeProcInstance(Setup, hBarInstance);
						DialogBox(hBarInstance, "Setup", NULLHWND, dlgprc);
						FreeProcInstance((FARPROC) dlgprc);
						break;

#ifdef USE_FONT
					case ID_FONT:
						// Show the font dialog
						// (available only if commdlg.dll is present)
						SetFont();
						break;
#endif

					case ID_EXIT:
						// Remove the message filter
						// and destroy the window
						RemoveFilter();
						DestroyWindow(hWnd);
						break;
				}

				return 0;
			}
			break;

		case WM_DESTROY:
			// Cause the entire desktop to redraw to remove
			// visual residue of ourselves
			InvalidateRect(NULLHWND, NULL, FALSE);

			// Dispose of the timer we created
			KillTimer(hWnd, 1);

			// And we are out of here!
			PostQuitMessage(0);
			return 0;
	}

	// Call DefProc
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


BOOL CALLBACK About(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bResult = FALSE;

	switch (msg) {

		case WM_COMMAND:
			// Any command message causes this
			// dialog to go awaw 
			EndDialog(hwndDlg, wParam);
			bResult = TRUE;
			break;
	}

	return bResult;
}


static Position NEAR GetObjectPos(PBarData pData, ObjType obj)
{
	Position pos = ePosOff;

	if (pData->left & obj)
		pos = ePosLeft;
	else if (pData->right & obj)
		pos = ePosRight;
	else if (pData->toggle & obj)
		pos = ePosToggle;

	return pos;
}


static void NEAR SetObjectPos(PBarData pData, ObjType obj, Position pos)
{
	// Turn bit off in all positions
	pData->left &= ~obj;
	pData->right &= ~obj;
	pData->toggle &= ~obj;

	// Turn on bit in proper position
	if (pos == ePosLeft)
		pData->left |= obj;
	else if (pos == ePosRight)
		pData->right |= obj;
	else if (pos == ePosToggle)
		pData->toggle |= obj;

	return;
}


static short NEAR GetRadioIdx(HWND hwnd, short base, short cnt)
{
	short i;

	for (i = 0; i < cnt; i++) {
		if (IsDlgButtonChecked(hwnd, base + i))
			break;
	}

	return i;
}


BOOL CALLBACK Setup(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bResult = FALSE;
	char buf[BUF_SIZE];
	short i;

	switch (msg) {

		case WM_INITDIALOG:
			// Initialize the object positions
			for (i = 0; i < eTypeCnt; i++) {
				CheckDlgButton(hwndDlg,
							110 + (i * ePosCnt) +
							GetObjectPos(&barData, 1 << i), 1);
			}

			// Initialize the drive letter control
			buf[0] = (64 + barData.diskID);
			buf[1] = 0;
			SetDlgItemText(hwndDlg, 100, buf);

			// Initialize time format
			CheckDlgButton(hwndDlg, 102,
						((barData.timeOptions & eTime24Hour) != 0));
			CheckDlgButton(hwndDlg, 103,
						((barData.timeOptions & eTimeSeconds) != 0));

			// Initialize date format and date strings
			CheckDlgButton(hwndDlg, 105 + barData.dateFormat, 1);

			for (i = 0; i < eDateCnt; i++) {
				barData.dateFormat = i;
				GetString(eTypeDate, buf, BUF_SIZE);
				SetDlgItemText(hwndDlg, 105 + i, buf);
			}

			barData.dateFormat = GetRadioIdx(hwndDlg, 105, eDateCnt);
			break;

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					// Update object positions
					for (i = 0; i < eTypeCnt; i++) {

						SetObjectPos(&barData, (1 << i),
								   GetRadioIdx(hwndDlg, 110 + (i * ePosCnt), ePosCnt));
					}

					// No valid items so we force
					// the time to display on the right
					if ((barData.left == eTypeNone) &&
					    (barData.right == eTypeNone))
						barData.right = eTypeTime;

					// Update drive letter
					GetDlgItemText(hwndDlg, 100, buf, 5);
					if ((buf[0] > 64) && (buf[0] < 92))
						barData.diskID = buf[0] - 64;

					// Update time options
					barData.timeOptions = 0;
					if (IsDlgButtonChecked(hwndDlg, 102))
						barData.timeOptions += eTime24Hour;
					if (IsDlgButtonChecked(hwndDlg, 103))
						barData.timeOptions += eTimeSeconds;

					// Update date format
					barData.dateFormat = GetRadioIdx(hwndDlg, 105, eDateCnt);

					// Write the profile
					WriteProfile(&barData);

					// Fall through...

				case IDCANCEL:
					EndDialog(hwndDlg, wParam);
					bResult = TRUE;
					break;
			}
			break;
	}

	return bResult;
}


static void NEAR UpdateToggleCnt(PBarData pData)
{
	ObjType type = pData->toggle;

	for (pData->toggleCnt = 0; type; type >>= 1) {
		if (type & 0x1) pData->toggleCnt++;
	}

	return;
}

static short NEAR ReadProfileInt(char FAR *pType, short def)
{
	return GetPrivateProfileInt(IDN_APP, pType, def, IDN_INI);
}


static void NEAR ReadProfile(PBarData pData)
{
	// Initialize the data from the users profile
	pData->left = ReadProfileInt("Left", eTypeDate);
	pData->right = ReadProfileInt("Right", eTypeTime);
	pData->toggle = ReadProfileInt("Toggle", eTypeRes | eTypeMem | eTypeDisk);

	pData->timeOptions = ReadProfileInt("TimeFormat", 0);
	pData->dateFormat = ReadProfileInt("DateFormat", eDateMDY);
	pData->diskID = ReadProfileInt("DiskID", 3);

	pData->leftOffset = ReadProfileInt("LeftOffset", 0);
	pData->rightOffset = ReadProfileInt("RightOffset", 0);

	pData->bShowWindow = ReadProfileInt("ShowWindow", 1);
	pData->bEraseAlways = ReadProfileInt("EraseAlways", 1);
	pData->bTransparent = ReadProfileInt("SystemColors", 1);

	if (!pData->bTransparent) {

		pData->textColor = RGB(ReadProfileInt("TextColorR", 0x00),
						   ReadProfileInt("TextColorG", 0x00),
						   ReadProfileInt("TextColorB", 0x00));
	}

#ifdef USE_FONT
	pData->fontSize = ReadProfileInt("FontSize", -13);
	pData->fontWeight = ReadProfileInt("Bold", 700);
	pData->bItalic = ReadProfileInt("Italic", 0);
	pData->hFont = NULLHFONT;

	GetPrivateProfileString(IDN_APP, "FontName", "System", pData->fontName, LF_FACESIZE, IDN_INI);

	CacheFont();
#endif USE_FONT

	UpdateToggleCnt(pData);

	// Read international date settings
	GetProfileString("intl", "sDate", "/", pData->sDate, 2);
	GetProfileString("intl", "sTime", ":", pData->sTime, 2);
	GetProfileString("intl", "s1159", "AM", pData->sAMPM[0], 5);
	GetProfileString("intl", "s2359", "PM", pData->sAMPM[1], 5);

	return;
}


static void NEAR SetProfileInt(char FAR *pType, short n)
{
	char Buf[BUF_SIZE];

	wsprintf(Buf, "%d", n);
	WritePrivateProfileString(IDN_APP, pType, Buf, IDN_INI);

	return;
}

static void NEAR WriteProfile(PBarData pData)
{
	SetProfileInt("Left", pData->left);
	SetProfileInt("Right", pData->right);
	SetProfileInt("Toggle", pData->toggle);

	SetProfileInt("TimeFormat", pData->timeOptions);
	SetProfileInt("DateFormat", pData->dateFormat);
	SetProfileInt("DiskID", pData->diskID);

	SetProfileInt("ShowWindow", pData->bShowWindow);
	SetProfileInt("SystemColors", pData->bTransparent);
	SetProfileInt("EraseAlways", pData->bEraseAlways);

	SetProfileInt("LeftOffset", pData->leftOffset);
	SetProfileInt("RightOffset", pData->rightOffset);

#ifdef USE_FONT
	SetProfileInt("FontSize", pData->fontSize);
	SetProfileInt("Bold", pData->fontWeight);
	SetProfileInt("Italic", pData->bItalic);

	WritePrivateProfileString(IDN_APP, "FontName", pData->fontName, IDN_INI);
#endif

	UpdateToggleCnt(pData);

	return;
}


