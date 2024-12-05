#include <windows.h>
#include <algorithm> 
#include <commctrl.h>
#include <fstream>
#include <commdlg.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

#define MAX_TABS 10
#define IDM_NEW 1
#define IDM_SAVE 2
#define IDM_OPEN 3
#define IDM_CLOSE 4


HINSTANCE hInst;
HWND hwndMain, hwndTab, hwndEdit[MAX_TABS];
std::vector<std::string> fileNames(MAX_TABS);
int currentTab = -1;  
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddNewTab(const char* title = "New Tab");
void SaveFile(HWND hwndEdit, int tabIndex);
void OpenFile();
void CloseTab(int tabIndex);
bool isTabSaved[MAX_TABS] = { true };  

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MainWindow";

    if (!RegisterClass(&wc)) return -1;

    hwndMain = CreateWindow(wc.lpszClassName, "Simple Text Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    if (!hwndMain) return -1;

    HMENU hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING, IDM_NEW, "New");
    AppendMenu(hMenu, MF_STRING, IDM_SAVE, "Save");
    AppendMenu(hMenu, MF_STRING, IDM_OPEN, "Open");
    AppendMenu(hMenu, MF_STRING, IDM_CLOSE, "Close");
    SetMenu(hwndMain, hMenu);

    ShowWindow(hwndMain, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES;
        InitCommonControlsEx(&icex);

        hwndTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, 800, 30, hwnd, NULL, hInst, NULL);

        for (int i = 0; i < MAX_TABS; i++) {
            hwndEdit[i] = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL | WS_HSCROLL, 0, 30, 800, 570, hwnd, NULL, hInst, NULL);
            ShowWindow(hwndEdit[i], SW_HIDE);
        }
        break;
    }
    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetWindowPos(hwndTab, NULL, 0, 0, rc.right, 30, SWP_NOZORDER);
        if (currentTab >= 0) {
            SetWindowPos(hwndEdit[currentTab], NULL, 0, 30, rc.right, rc.bottom - 30, SWP_NOZORDER);
        }
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDM_NEW:
            AddNewTab();
            break;
        case IDM_SAVE:
            if (currentTab >= 0) {
                SaveFile(hwndEdit[currentTab], currentTab);
            }
            break;
        case IDM_OPEN:
            OpenFile();
            break;
        case IDM_CLOSE:
            if (currentTab >= 0) {
                CloseTab(currentTab);
            }
            break;
        }
        break;
    }
    case WM_NOTIFY: {
        if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
            int iPage = TabCtrl_GetCurSel(hwndTab);
            ShowWindow(hwndEdit[currentTab], SW_HIDE);
            ShowWindow(hwndEdit[iPage], SW_SHOW);
            currentTab = iPage;
        } else if (((LPNMHDR)lParam)->code == NM_CLICK) {
            NMHDR* nmhdr = (NMHDR*)lParam;
            int iPage = TabCtrl_GetCurSel(hwndTab);
            RECT rcItem;
            TabCtrl_GetItemRect(hwndTab, iPage, &rcItem);

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwndTab, &pt);
            if (pt.x >= rcItem.right - 20 && pt.x <= rcItem.right) {
                CloseTab(iPage);
            }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void AddNewTab(const char* title) {
    if (currentTab < MAX_TABS - 1) {
        currentTab++;
        TCITEM tie;
        tie.mask = TCIF_TEXT;
        std::string displayName = title;
        displayName += "*";  
        tie.pszText = const_cast<LPSTR>(displayName.c_str());
        TabCtrl_InsertItem(hwndTab, currentTab, &tie);
        ShowWindow(hwndEdit[currentTab], SW_SHOW);
        TabCtrl_SetCurSel(hwndTab, currentTab);
        isTabSaved[currentTab] = false;
        SetWindowText(hwndEdit[currentTab], "");
    }
}


void SaveFile(HWND hwndEdit, int tabIndex) {
    if (fileNames[tabIndex].empty()) {
        CHAR fileName[MAX_PATH] = "";
        OPENFILENAME ofn = { 0 };

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwndMain;
        ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "txt";

        if (GetSaveFileName(&ofn)) {
            fileNames[tabIndex] = fileName;
        } else {
            return;
        }
    }

    DWORD textLength = GetWindowTextLength(hwndEdit);
    char* textBuffer = new char[textLength + 1];
    GetWindowText(hwndEdit, textBuffer, textLength + 1);

    std::ofstream outFile(fileNames[tabIndex]);
    if (outFile.is_open()) {
        outFile << textBuffer;
        outFile.close();
    }

    delete[] textBuffer;
    isTabSaved[tabIndex] = true;
    std::string displayName = fileNames[tabIndex];
    if (displayName.back() == '*') {
        displayName.pop_back();  
    }

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = const_cast<LPSTR>(displayName.c_str());
    TabCtrl_SetItem(hwndTab, tabIndex, &tie);
}


void OpenFile() {
    if (currentTab < MAX_TABS - 1) {
        CHAR fileName[MAX_PATH] = "";
        OPENFILENAME ofn = { 0 };

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwndMain;
        ofn.lpstrFilter = "Text Files\0*.txt\0Python Files\0*.py\0C++ Files\0*.cpp\0All Files\0*.*\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrDefExt = "txt";

        if (GetOpenFileName(&ofn)) {
            std::ifstream inFile(fileName);
            if (inFile.is_open()) {

                std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

                std::string fixedContent;
                for (size_t i = 0; i < fileContent.size(); ++i) {
                    if (fileContent[i] == '\n' && (i == 0 || fileContent[i-1] != '\r')) {
                        fixedContent += "\r\n"; 
                    } else {
                        fixedContent += fileContent[i];
                    }
                }

                AddNewTab(fileName);

                SetWindowText(hwndEdit[currentTab], "");
                SendMessage(hwndEdit[currentTab], EM_SETSEL, -1, -1);  
                SendMessage(hwndEdit[currentTab], EM_REPLACESEL, 0, (LPARAM)fixedContent.c_str());  

                fileNames[currentTab] = fileName;
                isTabSaved[currentTab] = true;

                std::string displayName = fileNames[currentTab];
                if (displayName.back() == '*') {
                    displayName.pop_back(); 
                }

                TCITEM tie;
                tie.mask = TCIF_TEXT;
                tie.pszText = const_cast<LPSTR>(displayName.c_str());
                TabCtrl_SetItem(hwndTab, currentTab, &tie);

                inFile.close();
            }
        }
    }
}

void CloseTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex > currentTab) return;
    SetWindowText(hwndEdit[tabIndex], "");
    ShowWindow(hwndEdit[tabIndex], SW_HIDE);
    TabCtrl_DeleteItem(hwndTab, tabIndex);
    for (int i = tabIndex; i < currentTab; i++) {
        hwndEdit[i] = hwndEdit[i + 1];
        fileNames[i] = fileNames[i + 1];
    }
    currentTab--;
    if (currentTab >= 0) {
        ShowWindow(hwndEdit[currentTab], SW_SHOW);
        TabCtrl_SetCurSel(hwndTab, currentTab);
    } else {
        currentTab = -1; 
    }
}

