#define MENU_HELLOOLLY_DIALOG	0
#define MENU_TEST				1
#define MENU_ABOUT				2


#define PNAME   "CommandBar"
#define PVERS   "3.00.108"
#define ANAME   "Gigapede"
#define CNAME   "TBD\n  Wayne\n  psyCK0\n  mfn"
#define CNAM2   "TBD Wayne psyCK0 mfn"

#define TOOLBAR_HIGHT   23
#define STATUSBAR_HIGHT 20
#define CMDBAR_HIGHT    25

#define CMDBAR_SHOW      1
#define CMDBAR_HIDE      0

#define TAG_CMDBAR     0x6C6D430AL     // Command line record type in .udd file

#define ID_HWBOX       1001            // Identifier of hwbox
#define ID_HWERR       1002            // Identifier of hwerr
#define ID_HWSTC       1003            // Identifier of hwerr

#define NHIST          32              // Number of commands in the history


extc HWND CreateCmdbarWindow(void);
extc BOOL SetFinderToolImage (HWND hwndDialog, BOOL bSet);
extc BOOL MoveCursorPositionToBullsEye (HWND hwndDialog);
extc long SearchWindow (HWND hwndDialog);
extc long DisplayInfoOnFoundWindow (HWND hwndDialog, HWND hwndFoundWindow);
extc long RefreshWindow (HWND hwndWindowToBeRefreshed);
extc long HighlightFoundWindow (HWND hwndDialog, HWND hwndFoundWindow);
extc BOOL CALLBACK SearchWindowDialogProc
(
  HWND hwndDlg, // handle to dialog box 
  UINT uMsg, // message 
  WPARAM wParam, // first message parameter 
  LPARAM lParam // second message parameter 
); 