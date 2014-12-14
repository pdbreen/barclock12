#ifndef __WINCLOCK_H_
#define __WINCLOCK_H_

#include <windows.h>
#include <string.h>
#include <time.h>
#ifdef USE_FONT
#include <commdlg.h>
#endif

#define	NULLHWND		((HWND) 0)
#define	NULLHDC		((HDC) 0)
#define	NULLHMENU		((HMENU) 0)
#define	NULLHFONT		((HFONT) 0)
#define	NULLHBRUSH	((HBRUSH) 0)
#define	NULLHINSTANCE	((HINSTANCE) 0)

#define	IDN_APP		"WinClock"
#define	IDN_INI		"WinClock.ini"
#define	IDN_WINDOW	"ClockWin"

#define	IDI_APP		100

#define	ID_SHOWMENU	100
#define	ID_SETUP		101
#define	ID_FONT		102
#define	ID_ABOUT		103
#define	ID_EXIT		104

#define	ID_MENUSETUP	201
#define	ID_MENUFONT	202
#define	ID_MENUABOUT	203
#define	ID_MENUEXIT	204

#define	BUF_SIZE		128

typedef enum {
	ePosLeft,
	ePosRight,
	ePosToggle,
	ePosOff,
	ePosCnt
} Position;

typedef enum {
	eTypeNone 	= 0x0000,
	eTypeFirst 	= 0x0001,

	eTypeTime 	= eTypeFirst,
	eTypeDate 	= 0x0002,
	eTypeMem		= 0x0004,
	eTypeRes 		= 0x0008,
	eTypeDisk		= 0x0010,

	eTypeLast		= 0x0020,
	eTypeCnt		= 5
} ObjType;

typedef enum {
	eDateMDY,
	eDateDMY,
	eDateYMD,
	eDateAbbrev,
	eDateLong,
	eDateCnt
} DateFormat;

typedef enum {
	eTimeSeconds = 0x0001,
	eTime24Hour = 0x0002
} TimeOptions;

typedef struct {

	int left;				// Left objects
	int right;			// Right objects
	int toggle;			// Toggled objects

	short toggleCnt;		// Number of objects in toggle array

// Other information
	DateFormat dateFormat;
	TimeOptions timeOptions;
	short diskID;

     char sTime[2];
	char sDate[2];
	char sAMPM[2][5];

// Color information
	BOOL bTransparent;		// If TRUE, blend with caption bar
	COLORREF textColor;

	BOOL bShowWindow;		// If TRUE, show WinClock window if no others
	BOOL bEraseAlways;		// If TRUE, refresh all windows in 3.0

     short leftOffset;		// Additional left offset
	short rightOffset;		// Additional right offset

// The following are filled in at runtime
	HMENU hMenu;			// Handle to menu

// Handles to functions that we dynamically link to -
// these are NULL if the function does not exist
	DWORD (PASCAL FAR *lpfnGetHeapSpaces)(HANDLE);
	BOOL (PASCAL FAR *lpfnRedrawWindow)(HWND, const RECT FAR*, HRGN, UINT);

#ifdef USE_FONT
// Font information
	short fontSize;		// Point size
	short fontWeight;		// Used for bold
	BOOL bItalic;
	char fontName[LF_FACESIZE];

// The following are filled in at runtime
	HFONT hFont;			// Handle to font

	BOOL (PASCAL FAR *lpfnChooseFont)(CHOOSEFONT FAR *);
#endif

} BarData, FAR *PBarData;


// Main window procedure

LRESULT FAR PASCAL WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Dialog procedures

BOOL CALLBACK About(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Setup(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


// DLL Functions

BOOL FAR _export InstallFilter(HWND, PBarData);
void FAR _export RemoveFilter(void);

void FAR _export DrawBar(void);
void FAR _export DisplayMenu(HWND);
void FAR _export GetString(ObjType type, char FAR *pBuf, short len);

// Message filter

DWORD FAR PASCAL GetMsgFilter(int nCode, WORD wParam, LPMSG pMsg);

#endif
