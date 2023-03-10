#include <windows.h>
#include "Plugin.h"
#include "main.h"
#include "resource.h"
#include <stdio.h>

#pragma comment(lib,"OLLYDBG.LIB")
#pragma warning(disable:4996)

#define REVISION 1.01

#define BULLSEYE_CENTER_X_OFFSET		15
#define BULLSEYE_CENTER_Y_OFFSET		18

HINSTANCE hinst;  // DLL instance
HWND      hwmain; // Handle of main OllyDbg window
HWND      hwedt;  // Edit control inside the combobox
HWND      hwerr;  // Error message
HWND      hwMDI;
HWND	  hwcmd; //


char		szMainWindowClassName[] = "WindowFinderMainWindow"; // Name of the Main Window class.

HWND		g_hwndMainWnd = NULL;
HANDLE		g_hApplicationMutex = NULL;
DWORD		g_dwLastError = 0;
BOOL		g_bStartSearchWindow = FALSE;
HCURSOR		g_hCursorSearchWindow = NULL;
HCURSOR		g_hCursorPrevious = NULL;
HBITMAP		g_hBitmapFinderToolFilled;
HBITMAP		g_hBitmapFinderToolEmpty;
HWND		g_hwndFoundWindow = NULL;
HPEN		g_hRectanglePen = NULL;





static WNDPROC   oldhweditproc;  // Original window procedure of hwedit
static WNDPROC   oldOllyWndProc; // Original window procedure of hwedit
static int       iShowCmdbar;

static char      CmdbarWinClass[32];   // Name of command line window class
char szODPath[MAX_PATH],szODIni[MAX_PATH],szIni[MAX_PATH],szPluginPath[MAX_PATH],szMacDefFile[MAX_PATH];


BOOL APIENTRY DllMain(HMODULE hModule,int reason,LPVOID lpReserved)
{
	
	
	if(reason==DLL_PROCESS_ATTACH)
	{		  
		hinst = hModule;
	}
	return true;
}


// Synopsis :
// 1. This function checks a hwnd to see if it is actually the "Search Window" Dialog's or Main Window's
// own window or one of their children. If so a FALSE will be returned so that these windows will not
// be selected. 
//
// 2. Also, this routine checks to see if the hwnd to be checked is already a currently found window.
// If so, a FALSE will also be returned to avoid repetitions.
extc BOOL CheckWindowValidity (HWND hwndDialog, HWND hwndToCheck)
{
	HWND hwndTemp = NULL;
	BOOL bRet = TRUE;

	// The window must not be NULL.
	if (hwndToCheck == NULL)
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

	// It must also be a valid window as far as the OS is concerned.
	if (IsWindow(hwndToCheck) == FALSE)
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

	// Ensure that the window is not the current one which has already been found.
	if (hwndToCheck == g_hwndFoundWindow)
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

	// It must also not be the main window itself.
	if (hwndToCheck == g_hwndMainWnd)
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

	// It also must not be the "Search Window" dialog box itself.
	if (hwndToCheck == hwndDialog)
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

	// It also must not be one of the dialog box's children...
	hwndTemp = GetParent (hwndToCheck);
	if ((hwndTemp == hwndDialog) || (hwndTemp == g_hwndMainWnd))
	{
		bRet = FALSE;
		goto CheckWindowValidity_0;
	}

CheckWindowValidity_0:

	return bRet;
}


extc long DoMouseMove 
	(
	HWND hwndDialog, 
	UINT message, 
	WPARAM wParam, 
	LPARAM lParam
	)
{
	POINT		screenpoint;
	HWND		hwndFoundWindow = NULL;
	//	char		szText[256];
	long		lRet = 0;

	// Must use GetCursorPos() instead of calculating from "lParam".
	GetCursorPos (&screenpoint);  	

	// Determine the window that lies underneath the mouse cursor.
	hwndFoundWindow = WindowFromPoint (screenpoint);

	// Check first for validity.
	if (CheckWindowValidity (hwndDialog, hwndFoundWindow))
	{
		// We have just found a new window.

		// Display some information on this found window.
		//DisplayInfoOnFoundWindow (hwndDialog, hwndFoundWindow);

		// If there was a previously found window, we must instruct it to refresh itself. 
		// This is done to remove any highlighting effects drawn by us.
		if (g_hwndFoundWindow)
		{
			RefreshWindow (g_hwndFoundWindow);
		}

		// Indicate that this found window is now the current global found window.
		g_hwndFoundWindow = hwndFoundWindow;

		// We now highlight the found window.
		HighlightFoundWindow (hwndDialog, g_hwndFoundWindow);
	}
	return lRet;
}


// Synopsis :
// 1. Handler for WM_LBUTTONUP message sent to the "Search Window" dialog box.
// 
// 2. We restore the screen cursor to the previous one.
//
// 3. We stop the window search operation and release the mouse capture.
extc long DoMouseUp
	(
	HWND hwndDialog, 
	UINT message, 
	WPARAM wParam, 
	LPARAM lParam
	)
{
	long lRet = 0;

	// If we had a previous cursor, set the screen cursor to the previous one.
	// The cursor is to stay exactly where it is currently located when the 
	// left mouse button is lifted.
	if (g_hCursorPrevious)
	{
		SetCursor (g_hCursorPrevious);
	}

	// If there was a found window, refresh it so that its highlighting is erased. 
	if (g_hwndFoundWindow)
	{
		RefreshWindow (g_hwndFoundWindow);
	}

//////////////////////////////////////////////////////////////////  핵심 코드 //////////////////////////////////////////////////////////////////	
	POINT		screenpoint;
	HWND		Hwnd_to_attach = NULL;
	GetCursorPos (&screenpoint);  	
	Hwnd_to_attach = WindowFromPoint (screenpoint);

	ULONG idProc = NULL; 
	GetWindowThreadProcessId(Hwnd_to_attach,&idProc );	

	if(!GetAsyncKeyState(VK_ESCAPE))
	{
		Attachtoactiveprocess(idProc);
	}

//////////////////////////////////////////////////////////////////  핵심 코드 //////////////////////////////////////////////////////////////////	
	// Set the bitmap on the Finder Tool icon to be the bitmap with the bullseye bitmap.
	SetFinderToolImage (hwndDialog, TRUE);

	// Very important : must release the mouse capture.
	ReleaseCapture ();

	// Make the main window appear normally.
	//ShowWindow (g_hwndMainWnd, SW_SHOWNORMAL);
	
	ShowWindow (hwndDialog, SW_MAXIMIZE);

	// Set the global search window flag to FALSE.
	g_bStartSearchWindow = FALSE;

	UpdateWindow(Hwnd_to_attach);

	return lRet;
}



// Synopsis :
// 1. This routine sets the Finder Tool icon to contain an appropriate bitmap.
//
// 2. If bSet is TRUE, we display the BullsEye bitmap. Otherwise the empty window
// bitmap is displayed.
extc BOOL SetFinderToolImage (HWND hwndDialog, BOOL bSet)
{
	HBITMAP hBmpToSet = NULL;
	BOOL bRet = TRUE;

	if (bSet)
	{
		// Set a FILLED image.
		hBmpToSet = g_hBitmapFinderToolFilled;
	}
	else
	{
		// Set an EMPTY image.
		hBmpToSet = g_hBitmapFinderToolEmpty;
	}

	SendMessage(hwcmd, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmpToSet);
	/*
	SendDlgItemMessage
		(
		(HWND)hwndDialog, // handle of dialog box 
		(int)hwcmd, // identifier of control 
		(UINT)STM_SETIMAGE, // message to send 
		(WPARAM)IMAGE_BITMAP, // first message parameter 
		(LPARAM)hBmpToSet // second message parameter 
		);*/

	return bRet;
}





// Synopsis :
// 1. This routine moves the mouse cursor hotspot to the exact 
// centre position of the bullseye in the finder tool static control.
//
// 2. This function, when used together with DoSetFinderToolImage(),
// gives the illusion that the bullseye image has indeed been transformed
// into a cursor and can be moved away from the Finder Tool Static
// control.
extc BOOL MoveCursorPositionToBullsEye (HWND hwndDialog)
{
	BOOL bRet = FALSE;
	HWND hwndToolFinder = NULL;
	RECT rect;
	POINT screenpoint;

	// Get the window handle of the Finder Tool static control.
	//		hwndToolFinder = GetDlgItem (hwndDialog, IDC_STATIC_ICON_FINDER_TOOL);

	if (hwndToolFinder)
	{
		// Get the screen coordinates of the static control,
		// add the appropriate pixel offsets to the center of 
		// the bullseye and move the mouse cursor to this exact
		// position.
		GetWindowRect (hwndToolFinder, &rect);
		screenpoint.x = rect.left + BULLSEYE_CENTER_X_OFFSET;
		screenpoint.y = rect.top + BULLSEYE_CENTER_Y_OFFSET;
		SetCursorPos (screenpoint.x, screenpoint.y);
	}
	return bRet;
}





// Synopsis :
// 1. This function starts the window searching operation.
//
// 2. A very important part of this function is to capture 
// all mouse activities from now onwards and direct all mouse 
// messages to the "Search Window" dialog box procedure.
extc long SearchWindow (HWND hwndDialog)
{
	long lRet = 0;

	// Set the global "g_bStartSearchWindow" flag to TRUE.
	g_bStartSearchWindow = TRUE;

	// Display the empty window bitmap image in the Finder Tool static control.
	SetFinderToolImage (hwndDialog, FALSE);

	MoveCursorPositionToBullsEye (hwndDialog);


	ShowWindow (hwndDialog, SW_MINIMIZE);

	// Set the screen cursor to the BullsEye cursor.
	if (g_hCursorSearchWindow)
	{
		g_hCursorPrevious = SetCursor (g_hCursorSearchWindow);
	}
	else
	{
		g_hCursorPrevious = NULL;
	}

	// Very important : capture all mouse activities from now onwards and
	// direct all mouse messages to the "Search Window" dialog box procedure.
	SetCapture (hwndDialog);

	// Hide the main window.
//	ShowWindow (g_hwndMainWnd, SW_HIDE);
//	ShowWindow (hwndDialog, SW_MINIMIZE);

	return lRet;
}


extc long DisplayInfoOnFoundWindow (HWND hwndDialog, HWND hwndFoundWindow)
{
	RECT		rect;              // Rectangle area of the found window.
	char		szText[256];
	char		szClassName[100];
	long		lRet = 0;

	// Get the screen coordinates of the rectangle of the found window.
	GetWindowRect (hwndFoundWindow, &rect);

	// Get the class name of the found window.
	GetClassName (hwndFoundWindow, szClassName, sizeof (szClassName) - 1);


	// Display some information on the found window.
	wsprintf
		(
		szText, "Window Handle == 0x%08X.\r\nClass Name : %s.\r\nRECT.left == %d.\r\nRECT.top == %d.\r\nRECT.right == %d.\r\nRECT.bottom == %d.\r\n",
		hwndFoundWindow,
		szClassName, 
		rect.left,
		rect.top,
		rect.right,
		rect.bottom
		);



	return lRet;
}



extc long RefreshWindow (HWND hwndWindowToBeRefreshed)
{
	long lRet = 0;

	InvalidateRect (hwndWindowToBeRefreshed, NULL, TRUE);
	UpdateWindow (hwndWindowToBeRefreshed);
	RedrawWindow (hwndWindowToBeRefreshed, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

	return lRet;
}



// Performs a highlighting of a found window.
// Comments below will demonstrate how this is done.
extc long HighlightFoundWindow (HWND hwndDialog, HWND hwndFoundWindow)
{
	HDC		hWindowDC = NULL;  // The DC of the found window.
	HGDIOBJ	hPrevPen = NULL;   // Handle of the existing pen in the DC of the found window.
	HGDIOBJ	hPrevBrush = NULL; // Handle of the existing brush in the DC of the found window.
	RECT		rect;              // Rectangle area of the found window.
	long		lRet = 0;

	// Get the screen coordinates of the rectangle of the found window.
	GetWindowRect (hwndFoundWindow, &rect);

	// Get the window DC of the found window.
	hWindowDC = GetWindowDC (hwndFoundWindow);

	if (hWindowDC)
	{
		// Select our created pen into the DC and backup the previous pen.
		hPrevPen = SelectObject (hWindowDC, g_hRectanglePen);

		// Select a transparent brush into the DC and backup the previous brush.
		hPrevBrush = SelectObject (hWindowDC, GetStockObject(HOLLOW_BRUSH));

		// Draw a rectangle in the DC covering the entire window area of the found window.
		Rectangle (hWindowDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

		// Reinsert the previous pen and brush into the found window's DC.
		SelectObject (hWindowDC, hPrevPen);

		SelectObject (hWindowDC, hPrevBrush);

		// Finally release the DC.
		ReleaseDC (hwndFoundWindow, hWindowDC);
	}

	return lRet;
}



/*
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/


extc int _export cdecl ODBG_Plugindata(char shortname[32])
{
	strcpy(shortname,"RoyalStep");
	return PLUGIN_VERSION;
}


LRESULT CALLBACK OllySubclassProc(HWND hw,UINT msg,WPARAM wp,LPARAM lp)
{
	int wmId, wmEvent;
	//	PAINTSTRUCT ps;
	//	HDC hdc;	
	BOOL bRet = FALSE;  // Default return value.

	switch (msg)
	{
	case WM_KEYDOWN:		
		break;

	case WM_CREATE:			
		break;

	case WM_MOUSEMOVE :
		bRet = TRUE;
		if (g_bStartSearchWindow)
		{
			DoMouseMove(hw, msg, wp, lp);
		}
		break;

	case WM_LBUTTONUP:
		bRet = TRUE;
		if (g_bStartSearchWindow)
		{
			DoMouseUp(hw, msg, wp, lp);
			//MessageBox(0,"gg","gg",0);
		}
		break;

	case WM_LBUTTONDOWN:		
		wmId    = LOWORD(wp);
		wmEvent = HIWORD(wp);	

		if(wmId ==  WM_COMMAND)
		
		{			
			SearchWindow(hw);		
		}
		break;
	}

	switch(wp)
	{
	case 0x401:
		// 버튼 눌릴때 동작하는 부분
		//MessageBox(0,"Action","MessageBox",0);
		SearchWindow(hw);
		break;				
	}	


	return CallWindowProc(oldOllyWndProc,hw,msg,wp,lp);	
}


extc int _export cdecl ODBG_Plugininit(int ollydbgversion,HWND hw, ulong *features)
{	
	TCHAR temp[255];


	if(ollydbgversion<PLUGIN_VERSION)
		return -1;

	hwmain=hw;
	hwMDI = (HWND)Plugingetvalue(VAL_HWCLIENT);
	
	sprintf(temp, "Made by KJM Ver.%1.2f", REVISION);
	Addtolist(0,-1,temp);

	if(Registerpluginclass(CmdbarWinClass,NULL,hinst,OllySubclassProc)<0) {
		return -1;
	}	

	hwcmd = CreateWindow("Static",NULL,
		WS_CHILD |  SS_BITMAP| WS_VISIBLE | SS_LEFT| SS_NOTIFY , 		
		650,0,25,21,
		hwmain,(HMENU)0x401,hinst,NULL);

	oldOllyWndProc = (WNDPROC)SetWindowLong(hwmain,GWL_WNDPROC,(long)OllySubclassProc);	

	if(oldOllyWndProc == 0) {
		Unregisterpluginclass(CmdbarWinClass);				
		return -1;
	} 

	iShowCmdbar = GetPrivateProfileInt("Option","Show Command Bar Window",CMDBAR_SHOW,szIni);
	if(!CreateCmdbarWindow()) {
		Unregisterpluginclass(CmdbarWinClass);
		SetWindowLong(hwmain,GWL_WNDPROC,(long)oldOllyWndProc);			
		return -1;
	}
	return 0;
}

HWND CreateCmdbarWindow(void) 
{	
	RECT rc;
	BOOL bRet = FALSE;

	//hinst = (HINSTANCE)Plugingetvalue(VAL_HINST);
	

  g_hCursorSearchWindow = LoadCursor (hinst, MAKEINTRESOURCE(IDC_CURSOR_SEARCH_WINDOW));
  if (g_hCursorSearchWindow == NULL)
  {
    bRet = FALSE;
	//goto InitialiseResources_0;
  }  

//  g_hRectanglePen = CreatePen (PS_SOLID, 3, RGB(256, 0, 0));
  g_hRectanglePen = CreatePen (PS_DOT, 8, RGB(255, 0, 0));
  if (g_hRectanglePen == NULL)
  {
    bRet = FALSE;
	goto InitialiseResources_0;
  }  

  g_hBitmapFinderToolFilled = LoadBitmap (hinst, MAKEINTRESOURCE(IDB_BITMAP_FULL));

  if (g_hBitmapFinderToolFilled == NULL)
  {
    bRet = FALSE;
	goto InitialiseResources_0;
  }
  SendMessage(hwcmd, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_hBitmapFinderToolFilled);


  g_hBitmapFinderToolEmpty = LoadBitmap (hinst, MAKEINTRESOURCE(IDB_BITMAP_no));
  if (g_hBitmapFinderToolEmpty == NULL)
  {
    bRet = FALSE;
	goto InitialiseResources_0;
  }

  // All went well. Return TRUE.
  bRet = TRUE;

InitialiseResources_0:

//  return bRet;
	// Bring window to top
	SetForegroundWindow(hwcmd);  
	return hwcmd;
};


BOOL UninitialiseResources()
{
  BOOL bRet = TRUE;

  if (g_hCursorSearchWindow)
  {
    // No need to destroy g_hCursorSearchWindow. It was not created using 
	// CreateCursor().
  }

  if (g_hRectanglePen)
  {
    bRet = DeleteObject (g_hRectanglePen);
	g_hRectanglePen = NULL;
  }

  if (g_hBitmapFinderToolFilled)
  {
	DeleteObject (g_hBitmapFinderToolFilled);
	g_hBitmapFinderToolFilled = NULL;
  }

  if (g_hBitmapFinderToolEmpty)
  {
	DeleteObject (g_hBitmapFinderToolEmpty);
	g_hBitmapFinderToolEmpty = NULL;
  }
    
  return bRet;
}

extc void _export cdecl ODBG_Pluginaction(int origin, int action, void *item)
{
	TCHAR temp[255];

	switch(origin)
	{	
	
	case PM_MAIN:		
		switch(action)
		{
			
		default :
			break;
		}

	case PM_DISASM:
		switch(action)
		{		
		case MENU_TEST:
			break;

		case MENU_ABOUT:
			sprintf(temp, "Olly Drattacher Ver %1.2f Made By KJM", REVISION);			
			MessageBox(hwmain,temp,"Drattacher",MB_OK);
			break;
		default:
			break;
		}
	default :
		break;

	}	
}

extc int _export cdecl ODBG_Pluginmenu(int origin, char data[4096], void *item)
{
	switch(origin)
	{
	case PM_MAIN : 
		strcpy(data,"2 About"
			);
		return 1;
	case PM_DISASM:
		if(Getstatus() == STAT_NONE)
		{
			return 0;
		}
		
		return 1;

	case PM_THREADS:
		if(Getstatus() == STAT_NONE)
		{
			return 0;
		}		
		return 1;

	case PM_BREAKPOINTS :
		return 1;

	case PM_MODULES :
		return 1;

	default:
		break;
	};
	return 0;

}

extc int _export cdecl ODBG_Pluginclose(void)
{
	//if(MyAlloc)
	{
		//메모리 할당 해제
	}
	UninitialiseResources();
	Addtolist(0,-1,"==== Close Galtae Plugin ====");
	return 0;
}




