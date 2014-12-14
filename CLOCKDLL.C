#include "winclock.h"
#include <dos.h>

// Bar formatting options -
// we need a pointer to the bar data block
PBarData pBarData;

// Utility values
short toggleID = 0;
short toggleCount = 0;			// Show date if greater than 0
Position clickPos = ePosOff;
RECT oldLeft = { 0, 0, 0, 0 };	// Used for invalidation on size change
RECT oldRight = { 0, 0, 0, 0 };
HWND hPrevWnd = NULLHWND;		// Used for invalidation on window change


// Static definitions for hooks and chains 
HOOKPROC lpprocGetMsgFilter = NULL;
HHOOK lplpfnNextGetMsgFilter= NULL;

// Global variables (Defined in DLL's DGROUP)
HWND hBarWnd = NULLHWND;

// Handle to the DLL instance from Libinit.asm
HANDLE LIBINST;



/********************************************************************
** LibMain is called by the libentry assembler code.
** The messages  are registered and local storage is initialized.
********************************************************************/

int FAR PASCAL LibMain(HANDLE hInstance, WORD a, WORD b, LPSTR c)
{
	LIBINST = hInstance;

	/* set up local heap space */
	return 1;
}


/********************************************************************
** WEP is called by Windows just before the DLL code is discarded.
** It always returns TRUE (go ahead).
********************************************************************/

int FAR PASCAL WEP(int a)
{
   return TRUE;
}


BOOL FAR _export InstallFilter(HWND hWnd, PBarData pData)
{
	// If the window handle is valid and
     // we are not already installed
	if (IsWindow(hWnd) && (hBarWnd == NULLHWND)) {

     	// Create the message filter instance
		lpprocGetMsgFilter = (HOOKPROC) MakeProcInstance((FARPROC) GetMsgFilter, LIBINST);

          // If we could create it
		if (lpprocGetMsgFilter != NULL) {

			// Install the message filter and
               // save the previous hook value
			lplpfnNextGetMsgFilter = (HHOOK) SetWindowsHook(WH_GETMESSAGE, lpprocGetMsgFilter);

			// Store the bar window handle -
			// we will need to send messages to it
			hBarWnd = hWnd;
			pBarData = pData;
		}
	}

     // Return TRUE if the filter was installed
	return (hBarWnd != NULLHWND);
}


void FAR _export RemoveFilter(void)
{
	// Destroy everything that we
	// created to install this
	if (lpprocGetMsgFilter != NULL) {

     	// Restore the previous hook
		UnhookWindowsHook(WH_GETMESSAGE, lpprocGetMsgFilter);

          // Destroy the filter proc instance
		FreeProcInstance((FARPROC) lpprocGetMsgFilter);
	}

	// Set everything back to NULL;
	lpprocGetMsgFilter = NULL;
	lplpfnNextGetMsgFilter = NULL;

	hBarWnd = NULLHWND;
	pBarData = NULL;

	return;
}


static BOOL NEAR IsTaskWindow(HWND hwnd)
{
	LONG Style;

	// If the window is a valid, non-iconic
	// window and is not our window
	if (IsWindow(hwnd) &&
	    IsWindowVisible(hwnd) &&
         (hwnd != hBarWnd) &&
	    (IsIconic(hwnd) != TRUE)) {

		// Get the window style
		Style = GetWindowLong(hwnd, GWL_STYLE);

		// Only overlapped windows with the system menu
		return (((Style & (WS_CHILD | WS_POPUP)) == 0) &&
			   ((Style & WS_SYSMENU) != 0));
	}

	// Not a task window
	return FALSE;
}


static HWND NEAR GetBarWindow(void)
{
	HWND hWnd;

	// Start with the first child of the desktop window
	hWnd = GetWindow(GetDesktopWindow(), GW_CHILD);

	// Loop while the window is not a task window
     // and we have not exhausted all the windows
	while ((hWnd != NULLHWND) && (IsTaskWindow(hWnd) != TRUE))
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);

	// Show our window if there is no where
	// else to put the clock information
	if (pBarData->bShowWindow) {
		ShowWindow(hBarWnd, (hWnd == NULLHWND)? SW_SHOW:SW_HIDE);
		if (hWnd == NULLHWND) hWnd = hBarWnd;
	}

	// Return the handle to the window that
	// we found or NULLHWND indicating that
	// there are no windows in which we can
	// draw our information
	return hWnd;
}


static void NEAR GetCaptionRect(HWND hwnd, LPRECT pRect)
{
	RECT WinRect;
	LONG Style;
	short width;

	// Get the rectangle for the entire window
	GetWindowRect(hwnd, &WinRect);

	// Initialize most of the rectangle
	pRect->left = 0;
	pRect->top = 0;
	pRect->right = WinRect.right - WinRect.left;

	// Determine the size of the icons in the caption -
	// 1 is added because it seems needed!
	width = (GetSystemMetrics(SM_CXSIZE) + 1);

	// Determine the window style of the window
	// in which we are going to draw our information
	Style = GetWindowLong(hwnd, GWL_STYLE);

	// Take care of the icons that can
	// appear in the caption
	if ((Style & WS_MINIMIZEBOX) != 0) pRect->right -= width;
	if ((Style & WS_MAXIMIZEBOX) != 0) pRect->right -= width;
	if ((Style & WS_SYSMENU) != 0) pRect->left += width;

	// If the window has a thick border,
     // the rectange is adjusted accordingly
	if ((Style & WS_THICKFRAME) != 0) {

		pRect->left += GetSystemMetrics(SM_CXFRAME);
		pRect->right -= GetSystemMetrics(SM_CXFRAME);
		pRect->top += GetSystemMetrics(SM_CYFRAME);

	// If the window has a regular border,
     // the rectange is adjusted accordingly
	} else if ((Style & WS_BORDER) != 0) {

		pRect->left += GetSystemMetrics(SM_CXBORDER);
		pRect->right -= GetSystemMetrics(SM_CXBORDER);
		pRect->top += GetSystemMetrics(SM_CYBORDER);
	}

	// The bottom of the rectangle is calculated
	// from the top and the size of the caption bar
	pRect->bottom = pRect->top +
				 GetSystemMetrics(SM_CYCAPTION) -
				 (2 * GetSystemMetrics(SM_CYBORDER));

	// Adjust the left and right by
	// factoring in the offsets
     pRect->left += pBarData->leftOffset;
	pRect->right -= pBarData->rightOffset;

	return;
}


void FAR _export GetString(ObjType id, char FAR *pBuf, short len)
{
	struct dfree diskSpace;
	struct tm *t;
	time_t secs;
	DateFormat dt;
	DWORD userFree;
	DWORD gdiFree;
	WORD userPct;
	WORD gdiPct;

	// If this is a time or date
	if ((id == eTypeDate) || (id == eTypeTime)) {

		// Get the current time
		time(&secs);
		t = localtime(&secs);

		// this is a time
		if (id == eTypeTime) {

			switch (pBarData->timeOptions) {

				case 0:
					wsprintf(pBuf, " %d%s%02d %s ",
								(t->tm_hour % 12),
								pBarData->sTime,
								t->tm_min,
								pBarData->sAMPM[t->tm_hour / 12]);
					break;

				case eTimeSeconds:
					wsprintf(pBuf, " %d%s%02d%s%02d %s ",
								(t->tm_hour % 12),
								pBarData->sTime,
								t->tm_min,
								pBarData->sTime,
								t->tm_sec,
								pBarData->sAMPM[t->tm_hour / 12]);
					break;

				case eTime24Hour:
					wsprintf(pBuf, " %02d%s%02d ",
								t->tm_hour,
								pBarData->sTime,
								t->tm_min);
					break;

				case eTime24Hour | eTimeSeconds:
					wsprintf(pBuf, " %02d%s%02d%s%02d ",
								t->tm_hour,
								pBarData->sTime,
								t->tm_min,
								pBarData->sTime,
								t->tm_sec);
					break;
			}

		// Otherwise, its a date
		} else {

			dt = pBarData->dateFormat;

			switch (dt) {

				case eDateMDY:
					wsprintf(pBuf, " %d%s%02d%s%02d ",
								t->tm_mon + 1,
                                        pBarData->sDate,
								t->tm_mday,
								pBarData->sDate,
								t->tm_year % 100);
					break;

				case eDateDMY:
					wsprintf(pBuf, " %d%s%02d%s%02d ",
								t->tm_mday,
								pBarData->sDate,
								t->tm_mon + 1,
								pBarData->sDate,
								t->tm_year % 100);
					break;

				case eDateYMD:
					wsprintf(pBuf, " %d%s%02d%s%02d ",
								t->tm_year % 100,
								pBarData->sDate,
								t->tm_mon + 1,
								pBarData->sDate,
								t->tm_mday);
					break;

				case eDateAbbrev:
					strftime(pBuf, len, "%a, %b %d, %Y ", t);
					break;

				case eDateLong:
					strftime(pBuf, len, "%A, %B %d, %Y ", t);
					break;
			}
		}

	// Otherwise
	} else {

		// Switch on the data to display
		switch (id) {

			// Handle resource text
			case eTypeRes:
				if (pBarData->lpfnGetHeapSpaces != NULL) {
					userFree = (*(pBarData->lpfnGetHeapSpaces))(GetModuleHandle("USER"));
					gdiFree = (*(pBarData->lpfnGetHeapSpaces))(GetModuleHandle("GDI"));

					userPct = (((DWORD)LOWORD(userFree) * 100L)/
								((DWORD) HIWORD(userFree)));
					gdiPct = (((DWORD)LOWORD(gdiFree) * 100L)/
								((DWORD) HIWORD(gdiFree)));

					wsprintf(pBuf, " User: %d%% GDI: %d%% ", userPct, gdiPct);
				}
				break;

			// Handle memory text
			case eTypeMem:
				wsprintf(pBuf, " Mem: %ld K ", (DWORD) (GetFreeSpace(0)/1024L));
				break;

			// Handle disk text
			case eTypeDisk:
				getdfree(pBarData->diskID, &diskSpace);
                    if (diskSpace.df_sclus != 0xFFFF) {
					wsprintf(pBuf, " Disk %c: %ld K ",
						    (char) (64 + pBarData->diskID),
						    (DWORD) (((long) diskSpace.df_avail
						    		    * (long) diskSpace.df_bsec
								    * (long) diskSpace.df_sclus)/1024L));
				}
				break;
		}
	}

	return;
}


static void NEAR GetBarInfo(Position pos, HWND hwnd, HDC hDC,
					   LPRECT pRect, char FAR *pBuf, short len)
{
	char FAR *pStart;
	short width = 0;
	ObjType list;
	ObjType obj;
	short l;
	HDC hTmpDC;

#ifdef USE_FONT
	HANDLE hOld;
#endif

	// Empty the buffer
	*pBuf = 0;

	// If we are toggled
	if ((toggleCount > 0) && (clickPos == pos)) {

		// Find the proper item in the toggle array
		for (obj = eTypeFirst, l = toggleID; obj < eTypeLast; obj <<= 1) {

			if (pBarData->toggle & obj) {
				l--;

				if (l == 0) break;
			}
		}

		// Get the string of the toggled item
		GetString(obj, pBuf, len);

	} else {

		// Get all the data for the specified position
		list = (pos == ePosLeft)? pBarData->left:pBarData->right;

		// Build the string by concatenating
		// the strings for the position together
		for (obj = eTypeFirst, pStart = pBuf; obj < eTypeLast; obj <<= 1) {

			// If this object is on this side
			if (list & obj) {

				GetString(obj, pStart, len);

				l = strlen(pStart);
				pStart += l;
				len -= l;
			}
		}
	}

	// Get the caption rectangle
	GetCaptionRect(hwnd, pRect);

	if (*pBuf) {

		// Create a DC if one was not specified
		if (hDC == NULLHDC) {

			hDC = hTmpDC = GetWindowDC(hwnd);

#ifdef USE_FONT
			if (hTmpDC != NULLHDC)
				hOld = SelectObject(hTmpDC, pBarData->hFont);
#endif
		}

		// Modify the size of the rectangle based
		// on the actual size of the string
		if (hDC != NULLHDC) {

			width = LOWORD(GetTextExtent(hDC, pBuf, strlen(pBuf)));

			if (hTmpDC) {

#ifdef USE_FONT
				SelectObject(hTmpDC, hOld);
#endif

				ReleaseDC(hwnd, hTmpDC);
			}
		}
	}

	// If there is not enough room for the content and
	// a margin of 10 pixels, we set the width to 0 to
	// keep from displaying any of it
	if (width + 10 >= (pRect->right - pRect->left)) {
		width = 0;
		*pBuf = 0;
	}

	// Position the content either to
	// the left or the right in the caption
	if (pos == ePosLeft)
		pRect->right = pRect->left + width;
	else
		pRect->left = pRect->right - width;

	return;
}


void FAR _export DrawBar(void)
{
	TEXTMETRIC Metrics;
	char buf[2][BUF_SIZE];
	HBRUSH hBrush = NULLHBRUSH;
	HBRUSH hOldBrush;
	COLORREF tstColor;
	COLORREF bkColor;
	UINT fuOptions;
	BOOL bErase;
	BOOL bActive;
	RECT rect[2];
	HWND hwnd;
	HDC hDC;
	short tmp;
	short i;
#ifdef USE_FONT
	HANDLE hOld;
#endif

	// Get the window to draw in
	if (((hwnd = GetBarWindow()) != NULLHWND) &&
	    ((hDC = GetWindowDC(hwnd)) != NULLHDC)) {

#ifdef USE_FONT
		// Use the new font
		hOld = SelectObject(hDC, pBarData->hFont);
#endif

		// Get all the bar information
		GetBarInfo(ePosLeft, hwnd, hDC, &(rect[0]), buf[0], BUF_SIZE);
		GetBarInfo(ePosRight, hwnd, hDC, &(rect[1]), buf[1], BUF_SIZE);

		// If the left and the right overlap,
		// favor the left over the right
		if (rect[1].left < rect[0].right) {
			rect[0].right = rect[0].left;
			buf[0][0] = 0;
		}

		// Decrement the date count - when this
		// reaches 0, the time will be displayed
		if (toggleCount > 0) toggleCount--;
		if (toggleCount == 0) toggleID = 0;

		// If the rectangle has gotten smaller since we last
		// drew it or we have moved to a new window,
		// we force an invalidation in the caption bar
		if ((IsTaskWindow(hPrevWnd) || (hPrevWnd == hBarWnd)) &&
		    ((hPrevWnd != hwnd) ||
			((rect[0].right - rect[0].left) <
			 (oldLeft.right - oldLeft.left)) ||
			((rect[1].right - rect[1].left) <
			 (oldRight.right - oldRight.left)))) {

			// Try to use the new RedrawWindow function
			// that is a part of Win3.1
			if (pBarData->lpfnRedrawWindow != NULL) {
				(*(pBarData->lpfnRedrawWindow))(hPrevWnd, NULL, (HRGN) 0,
										  RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);

			} else if (pBarData->bEraseAlways) {

				// If we couldn't handle it using
				// the Win3.1 API, resort to
				// invalidation of the whole window
				InvalidateRect(NULLHWND, NULL, TRUE);
			}
		}

		oldLeft = rect[0];
		oldRight = rect[1];
		hPrevWnd = hwnd;

		// The hard part - try to determine which
		// system colors to use, either active or
		// the inactive
		//
		// NOTE:  We want to err on the side of drawing
		//		the window active rather than inactive
		//		so we assume we are active unless we
		//		can prove otherwise!
		bActive = TRUE;

		// the first thing we need to do is determine if
		// we can query colors
		if ((hwnd != GetActiveWindow()) &&
		    (GetDeviceCaps(hDC, RASTERCAPS) & RC_BITBLT)) {

			bkColor = GetSysColor(COLOR_INACTIVECAPTION);

			for (i = 0; bActive && i < 2; i++) {

               	tmp = (i == 0)? rect[i].right:rect[i].left;

				if (*(buf[i])) {
					if ((tstColor = GetPixel(hDC, tmp, rect[i].bottom - 1)) != (COLORREF)(-1L))
						bActive = tstColor != bkColor;

					else if ((tstColor = GetPixel(hDC, tmp, rect[i].top)) != (COLORREF)(-1L))
						bActive = tstColor != bkColor;
				}
			}
		}

		// If we have determined that the
		// clock window is active
		if (bActive) {

			bkColor = GetSysColor(COLOR_ACTIVECAPTION);
			bErase = (bkColor != GetNearestColor(hDC, bkColor));

			// Use the colors for an active caption
			SetBkColor(hDC, bkColor);
			SetTextColor(hDC, GetSysColor(COLOR_CAPTIONTEXT));

		// Otherwise, the window is not active
		} else {

			bkColor = GetSysColor(COLOR_INACTIVECAPTION);
			bErase = (bkColor != GetNearestColor(hDC, bkColor));

			// Use the colors for an inactive caption
			SetBkColor(hDC, bkColor);
			SetTextColor(hDC, GetSysColor(COLOR_INACTIVECAPTIONTEXT));
		}

		// If we are not blending with the caption bar
		if (pBarData->bTransparent != TRUE) {

			// Override the text color
			SetTextColor(hDC, pBarData->textColor);
		}

		// Center the text vertically
		tmp = rect[0].top;
		GetTextMetrics(hDC, &Metrics);

		if ((rect[0].bottom - rect[0].top) > Metrics.tmHeight)
			tmp += ((rect[0].bottom - rect[0].top) - Metrics.tmHeight)/2;

		if (bErase && ((hBrush = CreateSolidBrush(bkColor)) != NULLHBRUSH)) {
			UnrealizeObject(hBrush);
			SetBrushOrg(hDC, 0, 0);
			hOldBrush = SelectObject(hDC, hBrush);
			SetBkMode(hDC, TRANSPARENT);
			fuOptions = ETO_CLIPPED;
		} else {
			fuOptions = ETO_CLIPPED | ETO_OPAQUE;
		}

		for (i = 0; i < 2; i++) {

			if (*(buf[i])) {

				if (bErase && hBrush) {
					FillRect(hDC, &(rect[i]), hBrush);
				} 

				// Draw the text
				ExtTextOut(hDC, rect[i].left, tmp,
						 fuOptions, &(rect[i]),
						 buf[i], strlen(buf[i]), (LPINT) NULL);
			}
		}

		// Clean up if we had to make a brush
		if (hBrush != NULLHBRUSH) {
			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}

#ifdef USE_FONT
		SelectObject(hDC, hOld);
#endif

		// Release the DC
		ReleaseDC(hwnd, hDC);
	}

	return;
}


void FAR _export DisplayMenu(HWND hOwner)
{
	char Buf[BUF_SIZE];
	RECT WinRect;
	RECT Rect;
	HWND hwnd;

	// If there is a window we can draw in
	if ((hwnd = GetBarWindow()) != NULLHWND) {

		// Get the rectangle where the content is located
		GetBarInfo(clickPos, hwnd, NULLHDC, &Rect, Buf, BUF_SIZE);

		// Get the rectangle for the window
		GetWindowRect(hwnd, &WinRect);

		// Adjust the rectangle so that the top-left
		// is just underneath the rectangle bottom-left
		OffsetRect(&Rect, WinRect.left, WinRect.top);

		// Display the menu
		TrackPopupMenu(pBarData->hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
					Rect.left, Rect.bottom, 0,
					hOwner, &Rect);
	}

	return;
}


DWORD FAR PASCAL GetMsgFilter(int nCode, WORD wParam, LPMSG pMsg)
{
	char Buf[BUF_SIZE];
	RECT WinRect;
	RECT Rect;
	DWORD stop;
	BOOL bSwap;
	int key;
	HDC hDC;
	Position pos;

	// If we should not ignore the message and
	// the message occurs in a window we are
	// interested in and we are not currently
	// displaying the date
	if ((nCode >= 0) && (pMsg->hwnd == GetBarWindow())) {

		// Switch on the message type
		switch (pMsg->message) {

			// If a left button down occurred in the
			// non-client area (which is where we added
			// our control), we need to respond if
			// it is within our control
			case WM_NCLBUTTONDOWN:
			case WM_NCLBUTTONDBLCLK:

				hDC = GetWindowDC(pMsg->hwnd);

				// Determine where our control is
				// located in screen coordinates
				for (clickPos = ePosOff, pos = ePosLeft;
					hDC != NULLHDC && pos <= ePosRight; pos++) {

					GetBarInfo(pos, pMsg->hwnd, hDC, &Rect, Buf, BUF_SIZE);

					GetWindowRect(pMsg->hwnd, &WinRect);
					OffsetRect(&Rect, WinRect.left, WinRect.top);

					// If our control contains the point
					if (PtInRect(&Rect, *((LPPOINT) &(pMsg->lParam)))) {
						clickPos = pos;
						break;
					}
				}

				if (hDC != NULLHDC) ReleaseDC(pMsg->hwnd, hDC);

				// If we are handling it
				if (clickPos != ePosOff) {

					// Set the message ID so that it is
					// ignored (we handled this message)
					pMsg->message = WM_NULL;

					// We delay for a small time here - if
					// the button is still down after the
					// delay, then we show the menu, otherwise
					// we show the date
					stop = GetTickCount() + 200;
					while (GetTickCount() < stop);

					// Determine which key to test
					bSwap = SwapMouseButton(TRUE);
					SwapMouseButton(bSwap);

					key = (bSwap)? VK_RBUTTON:VK_LBUTTON;

					// if the mouse is still down
					if ((GetAsyncKeyState(key) & 0x8000) != 0) {

						// Display the menu
						PostMessage(hBarWnd, WM_COMMAND, ID_SHOWMENU, 0);

					} else {

						// Toggle the next item
						toggleID = ((toggleID + 1) % (pBarData->toggleCnt + 1));
						toggleCount = (toggleID)? 4:0;
						DrawBar();
					}
				}
				break;
		}
	}

	// Always call the default procedure
	// (next in hook chain)
	return DefHookProc(nCode, wParam, (LONG) pMsg,
				    (FARPROC FAR *) &lplpfnNextGetMsgFilter);
}

