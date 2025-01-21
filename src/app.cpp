#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <cstdlib>
#include <fileapi.h>
#include <fstream>
#include <inicpp.h>
#include <powrprof.h>

#define APP_ICON_RESOURCE 100

NOTIFYICONDATA nid;
HWND hSliderWindow, hSlider, hTextLabel;
HHOOK hMouseHook;
HPOWERNOTIFY hPowerNotify;
bool isVisible = false;
std::string rwPath;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SliderWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

bool IsExecutable(const std::string &path) {
  DWORD fileAttributes = GetFileAttributes(path.c_str());
  if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    return false;

  size_t pos = path.rfind(".exe");
  if (pos == std::string::npos || pos != path.length() - 4)
    return false;

  HANDLE fileHandle =
      CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (fileHandle == INVALID_HANDLE_VALUE)
    return false;

  CloseHandle(fileHandle);
  return true;
}

void ShowErrorPopup(const char *errorMessage) {
  MessageBox(NULL, errorMessage, "Error", MB_ICONERROR | MB_OK);
}

int GetWindowHeight(HWND hControl) {
  RECT rect;
  if (GetWindowRect(hControl, &rect))
    return rect.bottom - rect.top;
  return -1;
}

void CreateSliderWindow(HINSTANCE hInstance) {
  WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
  wcex.lpfnWndProc = SliderWindowProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = TEXT("SliderWindowClass");
  RegisterClassEx(&wcex);

  hSliderWindow = CreateWindowEx(WS_EX_TOPMOST, wcex.lpszClassName,
                                 TEXT("Slider"), WS_POPUP | WS_BORDER, 0, 0,
                                 220, 50, NULL, NULL, hInstance, NULL);
  SetWindowPos(hSliderWindow, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

  RECT rc;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
  SetWindowPos(hSliderWindow, HWND_TOPMOST, rc.right - 300,
               rc.bottom - GetWindowHeight(hSliderWindow), 220, 50,
               SWP_NOACTIVATE);

  hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
}

void showContextMenu(HWND hwnd) {
  POINT pt;
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();
  AppendMenu(hMenu, MF_STRING, 1002, TEXT("Exit"));
  SetForegroundWindow(hwnd);

  TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd,
                 NULL);
  DestroyMenu(hMenu);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  std::ifstream iniFile("config.ini");
  if (!iniFile.is_open()) {
    ini::IniFile newIni;
    newIni["CONFIG"]["RWPATH"] = "changeme";
    newIni.save("config.ini");

    ShowErrorPopup("Please check your config.ini file and adjust the settings "
                   "to match your preferences.");
    return 1;
  }

  ini::IniFile iniParser(iniFile);
  rwPath = iniParser["CONFIG"]["RWPATH"].as<std::string>();

  if (rwPath == "changeme" || rwPath.empty() || !IsExecutable(rwPath)) {
    ShowErrorPopup(
        "Please check your config.ini file and set RWEverything path");
    return 1;
  }

  InitCommonControls();

  WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
  wcex.lpfnWndProc = WindowProc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = TEXT("TrayAppClass");
  RegisterClassEx(&wcex);

  HWND hWnd = CreateWindowEx(0, wcex.lpszClassName, TEXT("TrayApp"), 0, 0, 0, 0,
                             0, NULL, NULL, hInstance, NULL);

  SetWindowText(hWnd, TEXT("Brightness Control"));

  nid = {sizeof(NOTIFYICONDATA)};
  nid.hWnd = hWnd;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_USER + 1;
  nid.hIcon = LoadIcon(hInstance,
                       MAKEINTRESOURCE(APP_ICON_RESOURCE)); // Set the tray icon
  lstrcpy(nid.szTip, TEXT("Brightness Control"));
  Shell_NotifyIcon(NIM_ADD, &nid);
  hPowerNotify = RegisterPowerSettingNotification(
      hWnd, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE);

  CreateSliderWindow(hInstance);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Shell_NotifyIcon(NIM_DELETE, &nid);
  UnhookWindowsHookEx(hMouseHook);
  UnregisterPowerSettingNotification(hPowerNotify);
  return (int)msg.wParam;
}

void updateText() {
  int brightness = SendMessage(hSlider, TBM_GETPOS, 0, 0);
  std::string text = "Brightness: " + std::to_string(brightness);
  SetWindowText(hTextLabel, text.c_str());
}

void changeBrightness() {
  char tempDirPath[MAX_PATH];
  char tempFileName[MAX_PATH];

  if (GetTempPath(MAX_PATH, tempDirPath) == 0)
    return;

  if (GetTempFileName(tempDirPath, "RWB", 0, tempFileName) == 0)
    return;

  char buffer[20];
  int brightness = SendMessage(hSlider, TBM_GETPOS, 0, 0);
  std::snprintf(buffer, 20, "mul(Local4, %d)", brightness);

  std::ofstream file(tempFileName);
  file << "> Local0 = rPCIe32(0, 2, 0, 0x10)\n> Local1 = and(Local0, "
          "0xFFFFFFFFFFFFFFF0)\n> Local2 = or(Local1, 0xC8254)\n> Local3 = "
          "r32(Local2)\n> Local4 = shr(Local3, 16)\n> Local5 = ";
  file << buffer;
  file << "\n> Local5 = div(Local5, 100)\n> Local6 = shl(Local4, 16)\n> Local6 "
          "= or(Local6, Local5)\n> w32 Local2 Local6\n> RwExit\n";
  file.close();
  std::string cmd = rwPath + " /Command=" + tempFileName;

  STARTUPINFO startupInfo = {sizeof(STARTUPINFO)};
  startupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION process;
  if (CreateProcessA(nullptr, const_cast<char *>(cmd.c_str()), nullptr, nullptr,
                     FALSE, 0, nullptr, nullptr, &startupInfo, &process)) {
    WaitForSingleObject(process.hProcess, INFINITE);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
  }

  DeleteFile(tempFileName);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  if (uMsg == WM_USER + 1) {
    if (LOWORD(lParam) == WM_LBUTTONUP) {
      ShowWindow(hSliderWindow, isVisible ? SW_HIDE : SW_SHOW);
      isVisible = !isVisible;
    } else if (LOWORD(lParam) == WM_RBUTTONUP)
      showContextMenu(hwnd);

  } 
  else if (uMsg == WM_POWERBROADCAST &&
             (wParam == PBT_APMRESUMEAUTOMATIC ||
              wParam == PBT_APMRESUMESUSPEND ||
              wParam == PBT_APMPOWERSTATUSCHANGE))
    changeBrightness();

  else if (uMsg == WM_DESTROY || (uMsg == WM_COMMAND && LOWORD(wParam) == 1002))
    PostQuitMessage(0);

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK SliderWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam) {
  switch (uMsg) {
  case WM_CREATE: {
    hTextLabel = CreateWindowEx(
        0, TEXT("STATIC"), TEXT("Brightness: 50"),
        WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 5, 200, 20, hwnd, NULL,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    hSlider = CreateWindowEx(
        0, TRACKBAR_CLASS, NULL, WS_CHILD | WS_VISIBLE | TBS_HORZ, 10,
        GetWindowHeight(hTextLabel), 200, 30, hwnd, NULL,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(5, 100));
    SendMessage(hSlider, TBM_SETPOS, TRUE, 50);
    return 0;
  }
  case WM_ACTIVATE: {
    if (wParam == WA_INACTIVE) {
      ShowWindow(hwnd, SW_HIDE);
      isVisible = false;
    }
    return 0;
  }
  case WM_HSCROLL:
    if ((HWND)lParam == hSlider) {
      updateText();
      if (LOWORD(wParam) == SB_ENDSCROLL)
        changeBrightness();
    }
    return 0;
  case WM_CLOSE:
    ShowWindow(hwnd, SW_HIDE);
    isVisible = false;
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_NOTIFY: {
    if (((LPNMHDR)lParam)->hwndFrom == hSlider) {
      LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;

      switch (nmcd->dwDrawStage) {
      case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

      case CDDS_ITEMPREPAINT: {
        if (nmcd->dwItemSpec == TBCD_THUMB) {
          HDC hdc = nmcd->hdc;
          RECT rect = nmcd->rc;

          HBRUSH hBrush = CreateSolidBrush(RGB(0, 122, 204));
          HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

          int cornerRadius = 5;
          RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom,
                    cornerRadius, cornerRadius);

          SelectObject(hdc, hOldBrush);
          DeleteObject(hBrush);

          return CDRF_SKIPDEFAULT;
        }
        break;
      }
      }
    }
    break;
  }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION &&
      (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN)) {
    MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
    if (pMouseStruct != NULL) {
      POINT pt = pMouseStruct->pt;
      RECT windowRect;
      GetWindowRect(hSliderWindow, &windowRect);

      if (isVisible && !PtInRect(&windowRect, pt)) {
        ShowWindow(hSliderWindow, SW_HIDE);
        isVisible = false;
      }
    }
  }
  return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}