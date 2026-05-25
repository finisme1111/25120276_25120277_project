// ============================================================
//  main.cpp - He thong Quan ly Nha xe - GUI Win32
//  Compile: g++ -std=c++17 -DUNICODE -D_UNICODE -o parking_gui.exe main.cpp
//           -lgdi32 -lcomctl32 -lcomdlg32 -mwindows
// ============================================================
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "ParkingSystem.hpp"

#pragma comment(lib, "comctl32.lib")

// ── ID controls ───────────────────────────────────────────
#define IDC_TAB          100
#define IDC_BTN_ENTRY    201
#define IDC_BTN_EXIT     202
#define IDC_BTN_ADDUSER  203
#define IDC_BTN_DELUSER  204
#define IDC_BTN_SEARCH   205
#define IDC_BTN_LOADMAP  206
#define IDC_BTN_UNDO     207
#define IDC_BTN_SRCHPLATE 208
#define IDC_EDIT_ENTRYID 301
#define IDC_EDIT_EXITID  302
#define IDC_EDIT_UID     303
#define IDC_EDIT_UNAME   304
#define IDC_EDIT_UPLATE  305
#define IDC_EDIT_SRCHID  306
#define IDC_EDIT_MAPFILE 307
#define IDC_EDIT_PLATE   308
#define IDC_COMBO_ROLE   309
#define IDC_LIST_USERS   401
#define IDC_LIST_SLOTS   402
#define IDC_LIST_HIST    403
#define IDC_LIST_UNDO    404
#define IDC_STATIC_MSG   501
#define IDC_STATIC_DASH  502
#define IDC_STATIC_REV   503
#define IDC_MAP_PANEL    601

// ── Globals ───────────────────────────────────────────────
static HINSTANCE hInst;
static HWND hMain, hTab;
static HWND hPanels[7];   // 0=Dashboard 1=Map 2=Users 3=Gate 4=Slots 5=History 6=Revenue
static ParkingSystem* g_sys = nullptr;
static HFONT hFontMono, hFontUI, hFontBold;

// ── Helper: wstring <-> string (UTF-8) ────────────────────
static std::wstring toW(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    if (!w.empty() && w.back()==0) w.pop_back();
    return w;
}
static std::string toU(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    if (!s.empty() && s.back()==0) s.pop_back();
    return s;
}
static std::string getEditText(HWND hEdit) {
    int len = GetWindowTextLengthW(hEdit) + 1;
    std::wstring w(len, 0);
    GetWindowTextW(hEdit, &w[0], len);
    if (!w.empty() && w.back()==0) w.pop_back();
    return toU(w);
}
static void setStatic(HWND h, const std::string& s) {
    SetWindowTextW(h, toW(s).c_str());
}

// ── Helper: tao control ───────────────────────────────────
static HWND mkEdit(HWND p, int id, int x, int y, int w, int h, bool ro=false) {
    DWORD style = WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL;
    if (ro) style |= ES_READONLY;
    HWND hw = CreateWindowExW(0,L"EDIT",L"",style,x,y,w,h,p,(HMENU)(UINT_PTR)id,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    return hw;
}
static HWND mkBtn(HWND p, int id, const wchar_t* txt, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(0,L"BUTTON",txt,WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                              x,y,w,h,p,(HMENU)(UINT_PTR)id,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    return hw;
}
static HWND mkLabel(HWND p, const wchar_t* txt, int x, int y, int w, int h, bool bold=false) {
    HWND hw = CreateWindowExW(0,L"STATIC",txt,WS_CHILD|WS_VISIBLE,
                              x,y,w,h,p,(HMENU)0,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)(bold?hFontBold:hFontUI), TRUE);
    return hw;
}
static HWND mkStatic(HWND p, int id, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(WS_EX_CLIENTEDGE,L"STATIC",L"",
                              WS_CHILD|WS_VISIBLE|SS_LEFT,x,y,w,h,p,(HMENU)(UINT_PTR)id,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)hFontMono, TRUE);
    return hw;
}
static HWND mkCombo(HWND p, int id, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(0,L"COMBOBOX",L"",
                              WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                              x,y,w,h,p,(HMENU)(UINT_PTR)id,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    return hw;
}

// ── ListView helper ───────────────────────────────────────
static HWND mkListView(HWND p, int id, int x, int y, int w, int h) {
    HWND hw = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
        x,y,w,h,p,(HMENU)(UINT_PTR)id,hInst,0);
    SendMessageW(hw, WM_SETFONT, (WPARAM)hFontMono, TRUE);
    ListView_SetExtendedListViewStyle(hw, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    return hw;
}
static void lvAddCol(HWND lv, int idx, const wchar_t* txt, int w) {
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
    col.cx = w; col.pszText = (LPWSTR)txt; col.iSubItem = idx;
    ListView_InsertColumn(lv, idx, &col);
}
static void lvAddRow(HWND lv, const std::vector<std::wstring>& cells) {
    int row = ListView_GetItemCount(lv);
    LVITEMW item = {};
    item.mask = LVIF_TEXT; item.iItem = row; item.iSubItem = 0;
    item.pszText = (LPWSTR)cells[0].c_str();
    ListView_InsertItem(lv, &item);
    for (int i = 1; i < (int)cells.size(); i++) {
        ListView_SetItemText(lv, row, i, (LPWSTR)cells[i].c_str());
    }
}
static void lvClear(HWND lv) { ListView_DeleteAllItems(lv); }

// ── Refresh functions ─────────────────────────────────────
static HWND hLvUsers, hLvSlots, hLvHist, hLvUndo;
static HWND hStatDash, hStatRev, hStatMsg;
static HWND hMapEdit;

static void refreshDashboard() {
    auto st = g_sys->getStats();
    std::ostringstream o;
    o << "  Tong o do xe  : " << st.total << "\n"
      << "  Dang su dung  : " << st.occ   << "\n"
      << "  Con trong     : " << (st.total - st.occ) << "\n"
      << "  Nguoi dung    : " << st.users  << "\n"
      << "  Lich su       : " << st.hist   << " ban ghi\n"
      << "  Doanh thu     : " << st.revenue << " VND\n"
      << "  Ban do        : " << (st.mapLoaded ? "Da tai (BFS toi uu)" : "Chua tai") << "\n";
    setStatic(hStatDash, o.str());
}

static void refreshUsers() {
    lvClear(hLvUsers);
    int n = 0;
    for (const auto& r : g_sys->getUserList()) {
        lvAddRow(hLvUsers, {toW(std::to_string(++n)), toW(r[0]), toW(r[1]), toW(r[2]), toW(r[3])});
    }
}

static void refreshSlots() {
    lvClear(hLvSlots);
    for (const auto& r : g_sys->getSlotList()) {
        bool occ = (r[1] == "1");
        std::wstring status = occ ? L"Co xe" : L"Trong";
        lvAddRow(hLvSlots, {toW(r[0]), status, toW(r[2]), toW(r[3])});
    }
}

static void refreshHistory() {
    lvClear(hLvHist);
    auto hist = g_sys->getHistoryList(200);
    for (int i = (int)hist.size()-1; i >= 0; i--) {
        const auto& r = hist[i];
        lvAddRow(hLvHist, {toW(r[0]), toW(r[1]), toW(r[2]), toW(r[3]), toW(r[4])});
    }
}

static void refreshRevenue() {
    setStatic(hStatRev, g_sys->getRevenueText());
}

static void refreshUndo() {
    lvClear(hLvUndo);
    int n = 0;
    for (const auto& s : g_sys->getUndoList())
        lvAddRow(hLvUndo, {toW(std::to_string(++n)), toW(s)});
}

static void refreshMap() {
    auto lines = g_sys->getMapLines();
    std::wstring text;
    for (const auto& l : lines) { text += toW(l); text += L"\r\n"; }
    SetWindowTextW(hMapEdit, text.c_str());
}

static void refreshAll() {
    refreshDashboard();
    refreshUsers();
    refreshSlots();
    refreshHistory();
    refreshRevenue();
    refreshUndo();
    refreshMap();
}

// ── Tao cac panel ─────────────────────────────────────────
static void createPanelDashboard(HWND parent) {
    HWND p = hPanels[0];
    mkLabel(p, L"TONG QUAN HE THONG", 10, 10, 400, 24, true);
    hStatDash = mkStatic(p, IDC_STATIC_DASH, 10, 40, 500, 180);
}

static void createPanelMap(HWND parent) {
    HWND p = hPanels[1];
    mkLabel(p, L"File ban do:", 10, 10, 80, 22);
    HWND hMapFile = mkEdit(p, IDC_EDIT_MAPFILE, 95, 8, 300, 24);
    SetWindowTextW(hMapFile, L"map.txt");
    mkBtn(p, IDC_BTN_LOADMAP, L"Tai ban do", 405, 8, 100, 24);
    mkLabel(p, L"So do bai xe (P=SV trong, X=SV co xe, T=GV trong, G=GV co xe, E=Cong):", 10, 38, 600, 20);
    hMapEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
        10, 62, 760, 420, p, (HMENU)0, hInst, 0);
    SendMessageW(hMapEdit, WM_SETFONT, (WPARAM)hFontMono, TRUE);
}

static void createPanelUsers(HWND parent) {
    HWND p = hPanels[2];
    // Form them nguoi dung
    mkLabel(p, L"THEM NGUOI DUNG MOI", 10, 8, 250, 20, true);
    mkLabel(p, L"Ma so (8 chu so):", 10, 34, 130, 20);
    mkEdit(p, IDC_EDIT_UID, 145, 32, 140, 22);
    mkLabel(p, L"Ho ten:", 300, 34, 60, 20);
    mkEdit(p, IDC_EDIT_UNAME, 365, 32, 180, 22);
    mkLabel(p, L"Bien so xe:", 10, 62, 90, 20);
    mkEdit(p, IDC_EDIT_UPLATE, 105, 60, 140, 22);
    mkLabel(p, L"Loai:", 260, 62, 40, 20);
    HWND hCombo = mkCombo(p, IDC_COMBO_ROLE, 305, 60, 130, 80);
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Sinh vien");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Giang vien");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Khach");
    SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
    mkBtn(p, IDC_BTN_ADDUSER, L"Them nguoi dung", 450, 58, 130, 26);

    // Tim kiem / Xoa
    mkLabel(p, L"Tim kiem / Xoa theo ma so:", 10, 96, 200, 20, true);
    mkEdit(p, IDC_EDIT_SRCHID, 215, 94, 150, 22);
    mkBtn(p, IDC_BTN_SEARCH,  L"Tim kiem", 375, 92, 90, 24);
    mkBtn(p, IDC_BTN_DELUSER, L"Xoa",      475, 92, 70, 24);

    // Thong bao
    hStatMsg = mkStatic(p, IDC_STATIC_MSG, 10, 122, 760, 40);

    // Danh sach
    mkLabel(p, L"DANH SACH NGUOI DUNG:", 10, 168, 250, 20, true);
    hLvUsers = mkListView(p, IDC_LIST_USERS, 10, 190, 760, 300);
    lvAddCol(hLvUsers, 0, L"STT",      45);
    lvAddCol(hLvUsers, 1, L"Ma so",   100);
    lvAddCol(hLvUsers, 2, L"Ho ten",  220);
    lvAddCol(hLvUsers, 3, L"Bien so", 130);
    lvAddCol(hLvUsers, 4, L"Loai",    110);
}

static void createPanelGate(HWND parent) {
    HWND p = hPanels[3];
    mkLabel(p, L"XE VAO CONG", 10, 8, 200, 22, true);
    mkLabel(p, L"Ma so (8 chu so):", 10, 36, 130, 20);
    mkEdit(p, IDC_EDIT_ENTRYID, 145, 34, 160, 24);
    mkBtn(p, IDC_BTN_ENTRY, L"Xe VAO cong", 315, 32, 120, 26);

    mkLabel(p, L"XE RA CONG", 10, 72, 200, 22, true);
    mkLabel(p, L"Ma so (8 chu so):", 10, 100, 130, 20);
    mkEdit(p, IDC_EDIT_EXITID, 145, 98, 160, 24);
    mkBtn(p, IDC_BTN_EXIT, L"Xe RA cong", 315, 96, 120, 26);

    mkStatic(p, IDC_STATIC_MSG+1, 10, 130, 760, 80);

    mkLabel(p, L"XE DANG DO TRONG BAI:", 10, 220, 300, 20, true);
    hLvSlots = mkListView(p, IDC_LIST_SLOTS, 10, 244, 760, 260);
    lvAddCol(hLvSlots, 0, L"O #",      60);
    lvAddCol(hLvSlots, 1, L"Trang thai",100);
    lvAddCol(hLvSlots, 2, L"Bien so",  160);
    lvAddCol(hLvSlots, 3, L"Ma so",    120);
}

static void createPanelSlots(HWND parent) {
    HWND p = hPanels[4];
    mkLabel(p, L"TIM KIEM THEO BIEN SO:", 10, 8, 200, 20, true);
    mkEdit(p, IDC_EDIT_PLATE, 215, 6, 160, 24);
    mkBtn(p, IDC_BTN_SRCHPLATE, L"Tim kiem", 385, 4, 90, 26);
    mkStatic(p, IDC_STATIC_MSG+2, 10, 36, 760, 30);

    mkLabel(p, L"TRANG THAI TAT CA O DO XE:", 10, 74, 300, 20, true);
    HWND hLvAll = mkListView(p, IDC_LIST_SLOTS+1, 10, 96, 760, 420);
    lvAddCol(hLvAll, 0, L"O #",       60);
    lvAddCol(hLvAll, 1, L"Trang thai",110);
    lvAddCol(hLvAll, 2, L"Bien so",   160);
    lvAddCol(hLvAll, 3, L"Ma so",     120);
    // Luu lai de refresh
    SetWindowLongPtrW(hLvAll, GWLP_USERDATA, (LONG_PTR)hLvAll);
}

static void createPanelHistory(HWND parent) {
    HWND p = hPanels[5];
    mkLabel(p, L"LICH SU VAO/RA (moi nhat truoc):", 10, 8, 400, 22, true);
    hLvHist = mkListView(p, IDC_LIST_HIST, 10, 34, 760, 480);
    lvAddCol(hLvHist, 0, L"Thoi gian",     160);
    lvAddCol(hLvHist, 1, L"Hanh dong",      90);
    lvAddCol(hLvHist, 2, L"Ho ten",         200);
    lvAddCol(hLvHist, 3, L"Bien so",        130);
    lvAddCol(hLvHist, 4, L"O #",             60);
}

static void createPanelRevenue(HWND parent) {
    HWND p = hPanels[6];
    mkLabel(p, L"DOANH THU & QUAN TRI", 10, 8, 300, 22, true);
    hStatRev = mkStatic(p, IDC_STATIC_REV, 10, 34, 500, 130);

    mkLabel(p, L"UNDO - Hoan tac thao tac quan tri:", 10, 174, 300, 20, true);
    mkBtn(p, IDC_BTN_UNDO, L"Hoan tac (Undo)", 320, 170, 150, 26);
    mkStatic(p, IDC_STATIC_MSG+3, 10, 202, 760, 40);

    mkLabel(p, L"Lich su thao tac quan tri:", 10, 250, 250, 20);
    hLvUndo = mkListView(p, IDC_LIST_UNDO, 10, 272, 760, 240);
    lvAddCol(hLvUndo, 0, L"#",   40);
    lvAddCol(hLvUndo, 1, L"Mo ta", 600);
}

// ── WM_COMMAND handler ────────────────────────────────────
static HWND hGateMsg, hSlotMsg, hUndoMsg;

static void onCommand(HWND hwnd, int id) {
    switch (id) {
    case IDC_BTN_LOADMAP: {
        HWND hEd = GetDlgItem(hPanels[1], IDC_EDIT_MAPFILE);
        std::string fn = getEditText(hEd);
        bool ok = g_sys->loadMap(fn);
        if (!ok) ok = g_sys->loadMap("../app/" + fn);
        if (!ok) ok = g_sys->loadMap("../../app/" + fn);
        refreshAll();
        MessageBoxW(hwnd, ok ? L"Da tai ban do thanh cong!" : L"Khong tai duoc file ban do!",
                    L"Thong bao", ok ? MB_ICONINFORMATION : MB_ICONERROR);
        break;
    }
    case IDC_BTN_ADDUSER: {
        std::string id    = getEditText(GetDlgItem(hPanels[2], IDC_EDIT_UID));
        std::string name  = getEditText(GetDlgItem(hPanels[2], IDC_EDIT_UNAME));
        std::string plate = getEditText(GetDlgItem(hPanels[2], IDC_EDIT_UPLATE));
        int role = (int)SendMessageW(GetDlgItem(hPanels[2], IDC_COMBO_ROLE), CB_GETCURSEL, 0, 0);
        if (id.size() != 8) {
            setStatic(hStatMsg, "[LOI] Ma so phai la 8 chu so!");
            break;
        }
        bool ok = g_sys->addUser(id, name, plate, static_cast<UserRole>(role));
        setStatic(hStatMsg, ok ? "[OK] Da them: " + name + " (" + id + ")"
                               : "[LOI] Ma so '" + id + "' da ton tai!");
        if (ok) {
            SetWindowTextW(GetDlgItem(hPanels[2], IDC_EDIT_UID),    L"");
            SetWindowTextW(GetDlgItem(hPanels[2], IDC_EDIT_UNAME),  L"");
            SetWindowTextW(GetDlgItem(hPanels[2], IDC_EDIT_UPLATE), L"");
            refreshUsers(); refreshDashboard();
        }
        break;
    }
    case IDC_BTN_SEARCH: {
        std::string id = getEditText(GetDlgItem(hPanels[2], IDC_EDIT_SRCHID));
        const User* u = g_sys->getUser(id);
        if (!u) { setStatic(hStatMsg, "[LOI] Khong tim thay ma so '" + id + "'"); break; }
        std::string info = "Ten: " + u->name + "  |  Ma so: " + u->id
                         + "  |  Bien so: " + u->licensePlate
                         + "  |  Loai: " + roleToString(u->role);
        setStatic(hStatMsg, info);
        break;
    }
    case IDC_BTN_DELUSER: {
        std::string id = getEditText(GetDlgItem(hPanels[2], IDC_EDIT_SRCHID));
        if (MessageBoxW(hwnd, (L"Xac nhan xoa ma so: " + toW(id)).c_str(),
                        L"Xac nhan", MB_YESNO|MB_ICONQUESTION) != IDYES) break;
        bool ok = g_sys->removeUser(id);
        setStatic(hStatMsg, ok ? "[OK] Da xoa ma so: " + id : "[LOI] Khong tim thay!");
        if (ok) { refreshUsers(); refreshDashboard(); refreshUndo(); }
        break;
    }
    case IDC_BTN_ENTRY: {
        std::string id = getEditText(GetDlgItem(hPanels[3], IDC_EDIT_ENTRYID));
        auto [ok, msg] = g_sys->vehicleEntry(id);
        HWND hMsg = GetDlgItem(hPanels[3], IDC_STATIC_MSG+1);
        setStatic(hMsg, msg);
        if (ok) { SetWindowTextW(GetDlgItem(hPanels[3], IDC_EDIT_ENTRYID), L""); }
        refreshSlots(); refreshHistory(); refreshDashboard(); refreshMap();
        break;
    }
    case IDC_BTN_EXIT: {
        std::string id = getEditText(GetDlgItem(hPanels[3], IDC_EDIT_EXITID));
        auto [ok, msg] = g_sys->vehicleExit(id);
        HWND hMsg = GetDlgItem(hPanels[3], IDC_STATIC_MSG+1);
        setStatic(hMsg, msg);
        if (ok) { SetWindowTextW(GetDlgItem(hPanels[3], IDC_EDIT_EXITID), L""); }
        refreshSlots(); refreshHistory(); refreshDashboard(); refreshRevenue(); refreshMap();
        break;
    }
    case IDC_BTN_SRCHPLATE: {
        std::string plate = getEditText(GetDlgItem(hPanels[4], IDC_EDIT_PLATE));
        std::string res = g_sys->searchByPlate(plate);
        HWND hMsg = GetDlgItem(hPanels[4], IDC_STATIC_MSG+2);
        setStatic(hMsg, res);
        break;
    }
    case IDC_BTN_UNDO: {
        std::string msg = g_sys->doUndo();
        HWND hMsg = GetDlgItem(hPanels[6], IDC_STATIC_MSG+3);
        setStatic(hMsg, msg);
        refreshUsers(); refreshDashboard(); refreshUndo();
        break;
    }
    }
}

// ── WndProc cua panel (chuyen WM_COMMAND len main) ────────
static LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND)
        SendMessageW(hMain, WM_COMMAND, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Tab change ────────────────────────────────────────────
static void onTabChange(int idx) {
    for (int i = 0; i < 7; i++)
        ShowWindow(hPanels[i], i == idx ? SW_SHOW : SW_HIDE);
    if (idx == 0) refreshDashboard();
    if (idx == 4) {
        // Refresh bang trang thai tat ca o
        HWND hLvAll = GetDlgItem(hPanels[4], IDC_LIST_SLOTS+1);
        if (hLvAll) {
            lvClear(hLvAll);
            for (const auto& r : g_sys->getSlotList()) {
                bool occ = (r[1] == "1");
                lvAddRow(hLvAll, {toW(r[0]), occ ? L"Co xe" : L"Trong", toW(r[2]), toW(r[3])});
            }
        }
    }
    if (idx == 5) refreshHistory();
    if (idx == 6) { refreshRevenue(); refreshUndo(); }
}

// ── Main WndProc ──────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_NOTIFY: {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->hwndFrom == hTab && nm->code == TCN_SELCHANGE)
            onTabChange(TabCtrl_GetCurSel(hTab));
        break;
    }
    case WM_COMMAND:
        onCommand(hwnd, LOWORD(wp));
        break;
    case WM_SIZE: {
        int W = LOWORD(lp), H = HIWORD(lp);
        SetWindowPos(hTab, 0, 0, 0, W, H, SWP_NOZORDER|SWP_NOMOVE);
        RECT rc; GetClientRect(hTab, &rc);
        TabCtrl_AdjustRect(hTab, FALSE, &rc);
        for (int i = 0; i < 7; i++)
            SetWindowPos(hPanels[i], 0, rc.left, rc.top,
                         rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── WinMain ───────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    hInst = hInstance;
    SetConsoleOutputCP(65001);

    // Khoi tao Common Controls
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES|ICC_TAB_CLASSES };
    InitCommonControlsEx(&icc);

    // Tao font
    hFontUI   = CreateFontW(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    hFontBold = CreateFontW(16,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    hFontMono = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Consolas");

    // Dang ky window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszClassName = L"ParkingGUI";
    wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Dang ky class cho panel
    WNDCLASSEXW pc = {};
    pc.cbSize        = sizeof(pc);
    pc.lpfnWndProc   = PanelProc;
    pc.hInstance     = hInstance;
    pc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    pc.lpszClassName = L"ParkingPanel";
    RegisterClassExW(&pc);

    // Tao cua so chinh
    hMain = CreateWindowExW(0, L"ParkingGUI",
        L"He thong Quan ly Nha xe Thong minh  |  CSC10004",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        900, 640, 0, 0, hInstance, 0);

    // Tao Tab control
    hTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD|WS_VISIBLE|TCS_HOTTRACK,
        0, 0, 900, 600, hMain, (HMENU)IDC_TAB, hInstance, 0);
    SendMessageW(hTab, WM_SETFONT, (WPARAM)hFontUI, TRUE);

    // Them cac tab
    const wchar_t* tabNames[] = {
        L"Dashboard", L"Ban do", L"Nguoi dung",
        L"Cong vao/ra", L"Bai xe", L"Lich su", L"Doanh thu"
    };
    for (int i = 0; i < 7; i++) {
        TCITEMW ti = {}; ti.mask = TCIF_TEXT; ti.pszText = (LPWSTR)tabNames[i];
        TabCtrl_InsertItem(hTab, i, &ti);
    }

    // Lay vung noi dung cua tab
    RECT rc; GetClientRect(hTab, &rc);
    TabCtrl_AdjustRect(hTab, FALSE, &rc);

    // Tao 7 panel
    for (int i = 0; i < 7; i++) {
        hPanels[i] = CreateWindowExW(0, L"ParkingPanel", L"",
            WS_CHILD | (i==0 ? WS_VISIBLE : 0),
            rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
            hTab, 0, hInstance, 0);
    }

    // Khoi tao ParkingSystem
    g_sys = new ParkingSystem(20, "DanhSachSV.txt");
    // Thu tai ban do
    if (!g_sys->loadMap("map.txt"))
        if (!g_sys->loadMap("../app/map.txt"))
            g_sys->loadMap("../../app/map.txt");

    // Tao noi dung cac panel
    createPanelDashboard(hMain);
    createPanelMap(hMain);
    createPanelUsers(hMain);
    createPanelGate(hMain);
    createPanelSlots(hMain);
    createPanelHistory(hMain);
    createPanelRevenue(hMain);

    // Load du lieu ban dau
    refreshAll();

    ShowWindow(hMain, nCmdShow);
    UpdateWindow(hMain);

    MSG message;
    while (GetMessageW(&message, 0, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    delete g_sys;
    DeleteObject(hFontUI);
    DeleteObject(hFontBold);
    DeleteObject(hFontMono);
    return (int)message.wParam;
}
