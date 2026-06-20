// ============================================================
// main.cpp - Win32 GUI for the shared parking backend
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
#include <shellapi.h>

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "ParkingSystem.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace {

enum PanelIndex {
    PANEL_DASHBOARD = 0,
    PANEL_GATE,
    PANEL_USERS,
    PANEL_ACTIVE,
    PANEL_MAP,
    PANEL_HISTORY,
    PANEL_BILLS,
    PANEL_BLACKLIST,
    PANEL_MONTHLY,
    PANEL_UNDO,
    PANEL_COUNT
};

enum ControlId {
    IDC_MAIN_TAB = 100,
    IDC_DASH_TEXT = 200,
    IDC_GATE_ENTRY_ID = 300,
    IDC_GATE_EXIT_ID = 301,
    IDC_GATE_CONFIRM_ID = 302,
    IDC_GATE_MESSAGE = 303,
    IDC_USER_ID = 400,
    IDC_USER_NAME = 401,
    IDC_USER_PLATE = 402,
    IDC_USER_ROLE = 403,
    IDC_USER_SEARCH_ID = 404,
    IDC_USER_MESSAGE = 405,
    IDC_ACTIVE_PLATE = 500,
    IDC_ACTIVE_MESSAGE = 501,
    IDC_MAP_TEXT = 600,
    IDC_MAP_FILE = 601,
    IDC_HISTORY_USER_ID = 700,
    IDC_BILL_USER_ID = 800,
    IDC_BILL_ID = 801,
    IDC_BILL_MESSAGE = 802,
    IDC_MONTH_YEAR = 900,
    IDC_MONTH_MONTH = 901,
    IDC_UNDO_MESSAGE = 1000,
    IDC_LIST_USERS = 2000,
    IDC_LIST_ACTIVE = 2001,
    IDC_LIST_SLOTS = 2002,
    IDC_LIST_HISTORY = 2003,
    IDC_LIST_BILLS = 2004,
    IDC_LIST_BLACKLIST = 2005,
    IDC_LIST_MONTHLY = 2006,
    IDC_BTN_REFRESH = 3000,
    IDC_BTN_ENTRY = 3001,
    IDC_BTN_PREVIEW_EXIT = 3002,
    IDC_BTN_EXIT = 3003,
    IDC_BTN_REGISTER = 3004,
    IDC_BTN_SEARCH_USER = 3005,
    IDC_BTN_REMOVE_USER = 3006,
    IDC_BTN_SEARCH_PLATE = 3007,
    IDC_BTN_LOAD_HISTORY = 3008,
    IDC_BTN_LOAD_BILLS = 3009,
    IDC_BTN_PAY_BILL = 3010,
    IDC_BTN_LOAD_MONTHLY = 3011,
    IDC_BTN_UNDO = 3012,
    IDC_BTN_LOAD_MAP = 3013
};

HINSTANCE g_instance = nullptr;
HWND g_mainWindow = nullptr;
HWND g_tab = nullptr;
HWND g_panels[PANEL_COUNT] = {};
HFONT g_fontUi = nullptr;
HFONT g_fontBold = nullptr;
HFONT g_fontMono = nullptr;
ParkingSystem* g_system = nullptr;
std::string g_dataDirectory = ".";

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    const int count = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        -1,
        nullptr,
        0
    );

    if (count <= 0) {
        return L"";
    }

    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.c_str(),
        -1,
        &result[0],
        count
    );

    if (!result.empty() && result[result.size() - 1] == L'\0') {
        result.pop_back();
    }

    return result;
}

std::string toUtf8(const std::wstring& text) {
    if (text.empty()) {
        return "";
    }

    const int count = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (count <= 0) {
        return "";
    }

    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        &result[0],
        count,
        nullptr,
        nullptr
    );

    if (!result.empty() && result[result.size() - 1] == '\0') {
        result.pop_back();
    }

    return result;
}

std::string getText(HWND handle) {
    const int length = GetWindowTextLengthW(handle);
    std::wstring buffer(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(handle, &buffer[0], length + 1);
    buffer.resize(static_cast<std::size_t>(length));
    return toUtf8(buffer);
}

void setText(HWND handle, const std::string& text) {
    SetWindowTextW(handle, toWide(text).c_str());
}

std::string money(long long amount) {
    std::ostringstream output;
    output << amount << " VND";
    return output.str();
}

std::string formatTime(std::time_t value) {
    if (value <= 0) {
        return "-";
    }

    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &value);
#else
    localtime_r(&value, &local);
#endif

    std::ostringstream output;
    output << std::put_time(&local, "%d/%m/%Y %H:%M:%S");
    return output.str();
}

void showInfo(HWND owner, const std::string& text) {
    MessageBoxW(owner, toWide(text).c_str(), L"Thông báo", MB_OK | MB_ICONINFORMATION);
}

void showError(HWND owner, const std::string& text) {
    MessageBoxW(owner, toWide(text).c_str(), L"Lỗi", MB_OK | MB_ICONERROR);
}

HWND label(HWND parent, const wchar_t* text, int x, int y, int width, int height, bool bold = false) {
    HWND control = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        g_instance,
        nullptr
    );
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(bold ? g_fontBold : g_fontUi), TRUE);
    return control;
}

HWND edit(HWND parent, int id, int x, int y, int width, int height, bool readOnly = false, bool multiline = false) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (readOnly) {
        style |= ES_READONLY;
    }
    if (multiline) {
        style &= ~ES_AUTOHSCROLL;
        style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
    }

    HWND control = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        style,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        nullptr
    );
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(multiline ? g_fontMono : g_fontUi), TRUE);
    return control;
}

HWND button(HWND parent, int id, const wchar_t* text, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        nullptr
    );
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
    return control;
}

HWND combo(HWND parent, int id, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        nullptr
    );
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
    return control;
}

HWND listView(HWND parent, int id, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        nullptr
    );
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontMono), TRUE);
    ListView_SetExtendedListViewStyle(control, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    return control;
}

void addColumn(HWND list, int index, const wchar_t* text, int width) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = const_cast<LPWSTR>(text);
    column.cx = width;
    column.iSubItem = index;
    ListView_InsertColumn(list, index, &column);
}

void addRow(HWND list, const std::vector<std::string>& cells) {
    const int row = ListView_GetItemCount(list);
    std::vector<std::wstring> wideCells;
    wideCells.reserve(cells.size());
    for (const std::string& cell : cells) {
        wideCells.push_back(toWide(cell));
    }

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = wideCells.empty() ? const_cast<LPWSTR>(L"") : &wideCells[0][0];
    ListView_InsertItem(list, &item);

    for (int index = 1; index < static_cast<int>(wideCells.size()); ++index) {
        ListView_SetItemText(list, row, index, &wideCells[static_cast<std::size_t>(index)][0]);
    }
}

void clearList(HWND list) {
    ListView_DeleteAllItems(list);
}

HWND panelControl(PanelIndex panel, int id) {
    return GetDlgItem(g_panels[panel], id);
}

void refreshDashboard() {
    g_system->processMonthlyBilling();
    const ParkingSystem::Stats stats = g_system->getStats();
    std::ostringstream output;
    output << "Thư mục dữ liệu: " << g_dataDirectory << "\r\n"
           << "Tổng ô đỗ: " << stats.total << "\r\n"
           << "Xe đang gửi: " << stats.occ << "\r\n"
           << "Người dùng: " << stats.users << "\r\n"
           << "Bản ghi lịch sử: " << stats.hist << "\r\n"
           << "Tổng lượt đã tính: " << g_system->recordedUnits() << "\r\n"
           << "Tổng tiền từ monthly_usage: " << money(stats.revenue) << "\r\n"
           << "Hóa đơn quá hạn: " << g_system->overdueBillCount() << "\r\n"
           << "Tổng nợ quá hạn: " << money(g_system->overdueAmount()) << "\r\n"
           << "Bản đồ: " << (stats.mapLoaded ? "đã tải" : "chưa tải") << "\r\n";
    setText(panelControl(PANEL_DASHBOARD, IDC_DASH_TEXT), output.str());
}

void refreshUsers() {
    HWND list = panelControl(PANEL_USERS, IDC_LIST_USERS);
    clearList(list);
    int index = 1;
    for (const auto& row : g_system->getUserList()) {
        addRow(list, {
            std::to_string(index++),
            row[0],
            row[1],
            row[2],
            row[3],
            row[4]
        });
    }
}

void refreshActive() {
    HWND active = panelControl(PANEL_ACTIVE, IDC_LIST_ACTIVE);
    clearList(active);
    for (const auto& row : g_system->getActiveSessionList()) {
        addRow(active, {row[1], row[2], row[4], row[5], row[0]});
    }

    HWND slots = panelControl(PANEL_ACTIVE, IDC_LIST_SLOTS);
    clearList(slots);
    for (const auto& row : g_system->getSlotList()) {
        addRow(slots, {
            row[1],
            row[2] == "1" ? "Đang có xe" : "Trống",
            row[5],
            row[3],
            row[4]
        });
    }
}

void refreshMap() {
    std::wstring text;
    const std::vector<std::string> lines = g_system->getMapLines();
    for (const std::string& line : lines) {
        text += toWide(line);
        text += L"\r\n";
    }
    SetWindowTextW(panelControl(PANEL_MAP, IDC_MAP_TEXT), text.c_str());
}

void refreshBlacklist() {
    HWND list = panelControl(PANEL_BLACKLIST, IDC_LIST_BLACKLIST);
    clearList(list);
    g_system->processMonthlyBilling();
    for (const auto& row : g_system->getBlacklist()) {
        addRow(list, {row[0], row[1], row[2], money(std::stoll(row[3])), money(std::stoll(row[4]))});
    }
}

void refreshAll() {
    refreshDashboard();
    refreshUsers();
    refreshActive();
    refreshMap();
    refreshBlacklist();
}

void loadPersonalHistory() {
    const std::string userId = getText(panelControl(PANEL_HISTORY, IDC_HISTORY_USER_ID));
    HWND list = panelControl(PANEL_HISTORY, IDC_LIST_HISTORY);
    clearList(list);

    if (userId.empty()) {
        return;
    }

    g_system->forEachPersonalHistory(
        userId,
        [&](const ParkingRecord& record) {
            addRow(list, {
                record.timestamp,
                record.action,
                record.slotCode.empty() ? std::to_string(record.slotId) : record.slotCode,
                std::to_string(record.durationSeconds),
                std::to_string(record.chargedUnits),
                money(record.amount)
            });
        }
    );
}

void loadBills() {
    const std::string userId = getText(panelControl(PANEL_BILLS, IDC_BILL_USER_ID));
    HWND list = panelControl(PANEL_BILLS, IDC_LIST_BILLS);
    clearList(list);

    if (userId.empty()) {
        return;
    }

    g_system->processMonthlyBilling();
    g_system->forEachBill(
        userId,
        [&](const MonthlyBill& bill, long long penalty, long long total) {
            std::ostringstream month;
            month << bill.month << '/' << bill.year;
            addRow(list, {
                bill.billId,
                month.str(),
                std::to_string(bill.parkingUnits),
                money(bill.baseAmount),
                money(penalty),
                money(total),
                bill.paid ? "Đã thanh toán" : "Chưa thanh toán",
                formatTime(bill.dueDate),
                formatTime(bill.paidAt)
            });
        }
    );
}

void loadMonthlyStats() {
    int year = 0;
    int month = 0;
    try {
        year = std::stoi(getText(panelControl(PANEL_MONTHLY, IDC_MONTH_YEAR)));
        month = std::stoi(getText(panelControl(PANEL_MONTHLY, IDC_MONTH_MONTH)));
    } catch (...) {
        showError(g_mainWindow, "Năm hoặc tháng không hợp lệ.");
        return;
    }

    HWND list = panelControl(PANEL_MONTHLY, IDC_LIST_MONTHLY);
    clearList(list);

    long long units = 0;
    long long amount = 0;
    g_system->forEachMonthlyUsage(
        year,
        month,
        [&](const User& user, const MonthlyUsage& usage) {
            units += usage.parkingUnits;
            amount += usage.amount;
            addRow(list, {
                usage.userId,
                user.name,
                roleToString(user.role),
                std::to_string(usage.parkingUnits),
                money(usage.amount)
            });
        }
    );
    addRow(list, {"TỔNG", "", "", std::to_string(units), money(amount)});
}

void createDashboardPanel() {
    HWND panel = g_panels[PANEL_DASHBOARD];
    label(panel, L"Tổng quan hệ thống", 12, 12, 240, 24, true);
    edit(panel, IDC_DASH_TEXT, 12, 44, 760, 260, true, true);
    button(panel, IDC_BTN_REFRESH, L"Làm mới", 12, 318, 110, 28);
}

void createGatePanel() {
    HWND panel = g_panels[PANEL_GATE];
    label(panel, L"Xe vào", 12, 12, 180, 24, true);
    label(panel, L"ID", 12, 44, 80, 22);
    edit(panel, IDC_GATE_ENTRY_ID, 96, 42, 180, 24);
    button(panel, IDC_BTN_ENTRY, L"Ghi xe vào", 290, 40, 120, 28);

    label(panel, L"Xe ra / tìm vị trí", 12, 92, 220, 24, true);
    label(panel, L"ID lần 1", 12, 124, 80, 22);
    edit(panel, IDC_GATE_EXIT_ID, 96, 122, 180, 24);
    button(panel, IDC_BTN_PREVIEW_EXIT, L"Xem vị trí", 290, 120, 120, 28);
    label(panel, L"ID xác nhận", 12, 158, 90, 22);
    edit(panel, IDC_GATE_CONFIRM_ID, 106, 156, 170, 24);
    button(panel, IDC_BTN_EXIT, L"Ghi xe ra", 290, 154, 120, 28);

    edit(panel, IDC_GATE_MESSAGE, 12, 204, 760, 170, true, true);
}

void createUsersPanel() {
    HWND panel = g_panels[PANEL_USERS];
    label(panel, L"Đăng ký người dùng", 12, 10, 220, 22, true);
    label(panel, L"ID", 12, 38, 70, 20);
    edit(panel, IDC_USER_ID, 86, 36, 120, 24);
    label(panel, L"Họ tên", 220, 38, 70, 20);
    edit(panel, IDC_USER_NAME, 292, 36, 190, 24);
    label(panel, L"Biển số", 496, 38, 70, 20);
    edit(panel, IDC_USER_PLATE, 568, 36, 120, 24);
    HWND role = combo(panel, IDC_USER_ROLE, 700, 36, 150, 90);
    SendMessageW(role, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sinh viên"));
    SendMessageW(role, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Giảng viên"));
    SendMessageW(role, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Khách"));
    SendMessageW(role, CB_SETCURSEL, 0, 0);
    button(panel, IDC_BTN_REGISTER, L"Đăng ký", 864, 34, 110, 28);

    label(panel, L"Tra cứu / xóa", 12, 78, 140, 22, true);
    edit(panel, IDC_USER_SEARCH_ID, 150, 76, 150, 24);
    button(panel, IDC_BTN_SEARCH_USER, L"Tra cứu", 312, 74, 95, 28);
    button(panel, IDC_BTN_REMOVE_USER, L"Xóa", 418, 74, 80, 28);
    edit(panel, IDC_USER_MESSAGE, 512, 74, 462, 56, true, true);

    HWND list = listView(panel, IDC_LIST_USERS, 12, 142, 962, 390);
    addColumn(list, 0, L"#", 44);
    addColumn(list, 1, L"ID", 110);
    addColumn(list, 2, L"Họ tên", 260);
    addColumn(list, 3, L"Biển số", 130);
    addColumn(list, 4, L"Loại", 150);
    addColumn(list, 5, L"Tổng lượt", 90);
}

void createActivePanel() {
    HWND panel = g_panels[PANEL_ACTIVE];
    label(panel, L"Xe đang gửi", 12, 10, 180, 22, true);
    label(panel, L"Tìm biển số", 600, 10, 90, 22);
    edit(panel, IDC_ACTIVE_PLATE, 694, 8, 130, 24);
    button(panel, IDC_BTN_SEARCH_PLATE, L"Tìm", 836, 6, 70, 28);
    edit(panel, IDC_ACTIVE_MESSAGE, 12, 40, 962, 34, true, false);

    HWND active = listView(panel, IDC_LIST_ACTIVE, 12, 86, 962, 180);
    addColumn(active, 0, L"ID", 110);
    addColumn(active, 1, L"Biển số", 130);
    addColumn(active, 2, L"Ô", 90);
    addColumn(active, 3, L"Thời gian vào", 180);
    addColumn(active, 4, L"Session", 260);

    label(panel, L"Trạng thái ô đỗ", 12, 278, 180, 22, true);
    HWND slots = listView(panel, IDC_LIST_SLOTS, 12, 306, 962, 226);
    addColumn(slots, 0, L"Ô", 80);
    addColumn(slots, 1, L"Trạng thái", 120);
    addColumn(slots, 2, L"Loại", 70);
    addColumn(slots, 3, L"Biển số", 130);
    addColumn(slots, 4, L"ID", 110);
}

void createMapPanel() {
    HWND panel = g_panels[PANEL_MAP];
    label(panel, L"Bản đồ trạng thái (# tường, . lối đi, E cổng, P sinh viên/khách, T giảng viên, X đang có xe)", 12, 10, 850, 22, true);
    label(panel, L"File map", 12, 40, 70, 22);
    edit(panel, IDC_MAP_FILE, 86, 38, 260, 24);
    setText(panelControl(PANEL_MAP, IDC_MAP_FILE), g_dataDirectory == "." ? "map.txt" : g_dataDirectory + "/map.txt");
    button(panel, IDC_BTN_LOAD_MAP, L"Tải map", 358, 36, 100, 28);
    edit(panel, IDC_MAP_TEXT, 12, 76, 962, 456, true, true);
}

void createHistoryPanel() {
    HWND panel = g_panels[PANEL_HISTORY];
    label(panel, L"Lịch sử gửi xe cá nhân", 12, 10, 240, 22, true);
    label(panel, L"ID", 12, 42, 60, 22);
    edit(panel, IDC_HISTORY_USER_ID, 76, 40, 160, 24);
    button(panel, IDC_BTN_LOAD_HISTORY, L"Xem lịch sử", 248, 38, 120, 28);
    HWND list = listView(panel, IDC_LIST_HISTORY, 12, 82, 962, 450);
    addColumn(list, 0, L"Thời gian", 170);
    addColumn(list, 1, L"Sự kiện", 110);
    addColumn(list, 2, L"Ô", 80);
    addColumn(list, 3, L"Giây gửi", 100);
    addColumn(list, 4, L"Lượt", 70);
    addColumn(list, 5, L"Số tiền", 120);
}

void createBillsPanel() {
    HWND panel = g_panels[PANEL_BILLS];
    label(panel, L"Hóa đơn và thanh toán", 12, 10, 240, 22, true);
    label(panel, L"ID", 12, 42, 50, 22);
    edit(panel, IDC_BILL_USER_ID, 66, 40, 140, 24);
    button(panel, IDC_BTN_LOAD_BILLS, L"Xem hóa đơn", 218, 38, 120, 28);
    label(panel, L"Bill ID", 360, 42, 60, 22);
    edit(panel, IDC_BILL_ID, 424, 40, 240, 24);
    button(panel, IDC_BTN_PAY_BILL, L"Thanh toán", 676, 38, 120, 28);
    edit(panel, IDC_BILL_MESSAGE, 12, 76, 962, 34, true, false);

    HWND list = listView(panel, IDC_LIST_BILLS, 12, 122, 962, 410);
    addColumn(list, 0, L"Bill ID", 210);
    addColumn(list, 1, L"Tháng", 80);
    addColumn(list, 2, L"Lượt", 70);
    addColumn(list, 3, L"Gốc", 100);
    addColumn(list, 4, L"Phạt", 100);
    addColumn(list, 5, L"Tổng", 100);
    addColumn(list, 6, L"Trạng thái", 130);
    addColumn(list, 7, L"Hạn", 150);
    addColumn(list, 8, L"Thanh toán", 150);
}

void createBlacklistPanel() {
    HWND panel = g_panels[PANEL_BLACKLIST];
    label(panel, L"Danh sách đen", 12, 10, 180, 22, true);
    button(panel, IDC_BTN_REFRESH, L"Làm mới", 200, 8, 100, 28);
    HWND list = listView(panel, IDC_LIST_BLACKLIST, 12, 48, 962, 484);
    addColumn(list, 0, L"ID", 110);
    addColumn(list, 1, L"Họ tên", 260);
    addColumn(list, 2, L"Hóa đơn quá hạn", 130);
    addColumn(list, 3, L"Tổng phạt", 120);
    addColumn(list, 4, L"Tổng phải trả", 130);
}

void createMonthlyPanel() {
    HWND panel = g_panels[PANEL_MONTHLY];
    label(panel, L"Thống kê tháng", 12, 10, 200, 22, true);
    label(panel, L"Năm", 12, 42, 50, 22);
    edit(panel, IDC_MONTH_YEAR, 66, 40, 90, 24);
    label(panel, L"Tháng", 170, 42, 60, 22);
    edit(panel, IDC_MONTH_MONTH, 234, 40, 60, 24);
    button(panel, IDC_BTN_LOAD_MONTHLY, L"Xem thống kê", 306, 38, 130, 28);
    HWND list = listView(panel, IDC_LIST_MONTHLY, 12, 82, 962, 450);
    addColumn(list, 0, L"ID", 110);
    addColumn(list, 1, L"Họ tên", 260);
    addColumn(list, 2, L"Loại", 150);
    addColumn(list, 3, L"Lượt", 80);
    addColumn(list, 4, L"Số tiền", 120);
}

void createUndoPanel() {
    HWND panel = g_panels[PANEL_UNDO];
    label(panel, L"Hoàn tác thao tác xe vào / xe ra gần nhất", 12, 12, 420, 22, true);
    button(panel, IDC_BTN_UNDO, L"Undo", 12, 44, 100, 30);
    edit(panel, IDC_UNDO_MESSAGE, 12, 90, 760, 120, true, true);
}

void createPanels() {
    createDashboardPanel();
    createGatePanel();
    createUsersPanel();
    createActivePanel();
    createMapPanel();
    createHistoryPanel();
    createBillsPanel();
    createBlacklistPanel();
    createMonthlyPanel();
    createUndoPanel();
}

void selectPanel(int index) {
    for (int i = 0; i < PANEL_COUNT; ++i) {
        ShowWindow(g_panels[i], i == index ? SW_SHOW : SW_HIDE);
    }

    if (index == PANEL_DASHBOARD) {
        refreshDashboard();
    } else if (index == PANEL_USERS) {
        refreshUsers();
    } else if (index == PANEL_ACTIVE) {
        refreshActive();
    } else if (index == PANEL_MAP) {
        refreshMap();
    } else if (index == PANEL_BLACKLIST) {
        refreshBlacklist();
    }
}

void handleRegister() {
    const std::string id = getText(panelControl(PANEL_USERS, IDC_USER_ID));
    const std::string name = getText(panelControl(PANEL_USERS, IDC_USER_NAME));
    const std::string plate = getText(panelControl(PANEL_USERS, IDC_USER_PLATE));
    const int roleIndex = static_cast<int>(SendMessageW(panelControl(PANEL_USERS, IDC_USER_ROLE), CB_GETCURSEL, 0, 0));
    UserRole role = UserRole::STUDENT;
    if (roleIndex == 1) {
        role = UserRole::LECTURER;
    } else if (roleIndex == 2) {
        role = UserRole::GUEST;
    }

    std::string error;
    if (!g_system->registerUser(id, name, plate, role, error)) {
        setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), error);
        return;
    }

    setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), "Đăng ký thành công.");
    refreshUsers();
    refreshDashboard();
}

void handleSearchUser() {
    const std::string id = getText(panelControl(PANEL_USERS, IDC_USER_SEARCH_ID));
    const User* user = g_system->findUser(id);
    if (!user) {
        setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), "Không tìm thấy người dùng.");
        return;
    }

    std::ostringstream output;
    output << "ID: " << user->id
           << " | Họ tên: " << user->name
           << " | Biển số: " << user->licensePlate
           << " | Loại: " << roleToString(user->role)
           << "\r\nTổng lượt: " << user->totalParkingUnits
           << " | Blacklist: " << (g_system->isBlacklisted(id) ? "Có" : "Không")
           << " | Chưa thanh toán: " << money(g_system->outstandingAmount(id));
    setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), output.str());
}

void handleRemoveUser() {
    const std::string id = getText(panelControl(PANEL_USERS, IDC_USER_SEARCH_ID));
    if (id.empty()) {
        return;
    }

    if (MessageBoxW(g_mainWindow, toWide("Xóa người dùng " + id + "?").c_str(), L"Xác nhận", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    std::string error;
    if (!g_system->removeUser(id, error)) {
        setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), error);
        return;
    }

    setText(panelControl(PANEL_USERS, IDC_USER_MESSAGE), "Đã xóa người dùng.");
    refreshUsers();
    refreshDashboard();
}

void handleEntry() {
    const std::string id = getText(panelControl(PANEL_GATE, IDC_GATE_ENTRY_ID));
    ActiveSession session;
    std::string error;
    if (!g_system->vehicleEntry(id, session, error)) {
        if (!g_system->findUser(id)) {
            const int answer = MessageBoxW(
                g_mainWindow,
                L"ID chưa đăng ký. Chuyển sang tab đăng ký và điền ID này?",
                L"Đăng ký người dùng",
                MB_YESNO | MB_ICONQUESTION
            );
            if (answer == IDYES) {
                TabCtrl_SetCurSel(g_tab, PANEL_USERS);
                selectPanel(PANEL_USERS);
                setText(panelControl(PANEL_USERS, IDC_USER_ID), id);
            }
        }
        setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), error);
        return;
    }

    std::ostringstream output;
    output << "Xe vào thành công\r\nID: " << session.userId
           << "\r\nBiển số: " << session.licensePlate
           << "\r\nÔ: " << session.slotCode
           << "\r\nThời gian: " << session.entryTimestamp;
    setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), output.str());
    refreshActive();
    refreshMap();
    refreshDashboard();
}

void handlePreviewExit() {
    const std::string id = getText(panelControl(PANEL_GATE, IDC_GATE_EXIT_ID));
    ActiveSession session;
    std::string error;
    if (!g_system->previewVehicleExit(id, session, error)) {
        setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), error);
        return;
    }

    const User* user = g_system->findUser(id);
    std::ostringstream output;
    if (user) {
        output << "Họ tên: " << user->name << "\r\n";
    }
    output << "Biển số: " << session.licensePlate
           << "\r\nÔ: " << session.slotCode
           << "\r\nThời gian vào: " << session.entryTimestamp
           << "\r\nNhập lại ID ở ô xác nhận rồi bấm Ghi xe ra.";
    setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), output.str());
}

void handleExit() {
    const std::string id = getText(panelControl(PANEL_GATE, IDC_GATE_EXIT_ID));
    const std::string confirmation = getText(panelControl(PANEL_GATE, IDC_GATE_CONFIRM_ID));
    ParkingRecord record;
    std::string error;
    if (!g_system->vehicleExit(id, confirmation, record, error)) {
        setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), error);
        return;
    }

    std::ostringstream output;
    output << "Xe ra thành công\r\nID: " << record.userId
           << "\r\nÔ giải phóng: " << record.slotCode
           << "\r\nThời lượng giây: " << record.durationSeconds
           << "\r\nSố lượt: " << record.chargedUnits
           << "\r\nTiền cộng vào tháng: " << money(record.amount);
    setText(panelControl(PANEL_GATE, IDC_GATE_MESSAGE), output.str());
    refreshActive();
    refreshMap();
    refreshDashboard();
}

void handleSearchPlate() {
    const std::string plate = getText(panelControl(PANEL_ACTIVE, IDC_ACTIVE_PLATE));
    setText(panelControl(PANEL_ACTIVE, IDC_ACTIVE_MESSAGE), g_system->searchByPlate(plate));
}

void handleLoadMap() {
    const std::string fileName = getText(panelControl(PANEL_MAP, IDC_MAP_FILE));
    if (!g_system->loadMap(fileName)) {
        showError(g_mainWindow, "Không tải được file map hoặc map không khớp active sessions hiện tại.");
        return;
    }

    refreshActive();
    refreshMap();
    refreshDashboard();
    showInfo(g_mainWindow, "Đã tải lại bản đồ.");
}

void handlePayBill() {
    const std::string billId = getText(panelControl(PANEL_BILLS, IDC_BILL_ID));
    long long paid = 0;
    std::string error;
    if (!g_system->payBill(billId, paid, error)) {
        setText(panelControl(PANEL_BILLS, IDC_BILL_MESSAGE), error);
        return;
    }

    setText(panelControl(PANEL_BILLS, IDC_BILL_MESSAGE), "Thanh toán thành công: " + money(paid));
    loadBills();
    refreshBlacklist();
    refreshDashboard();
}

void handleUndo() {
    std::string error;
    if (!g_system->undoLast(error)) {
        setText(panelControl(PANEL_UNDO, IDC_UNDO_MESSAGE), error);
        return;
    }

    setText(panelControl(PANEL_UNDO, IDC_UNDO_MESSAGE), "Hoàn tác thành công.");
    refreshAll();
}

void handleCommand(HWND hwnd, int id) {
    switch (id) {
        case IDC_BTN_REFRESH:
            refreshAll();
            break;
        case IDC_BTN_ENTRY:
            handleEntry();
            break;
        case IDC_BTN_PREVIEW_EXIT:
            handlePreviewExit();
            break;
        case IDC_BTN_EXIT:
            handleExit();
            break;
        case IDC_BTN_REGISTER:
            handleRegister();
            break;
        case IDC_BTN_SEARCH_USER:
            handleSearchUser();
            break;
        case IDC_BTN_REMOVE_USER:
            handleRemoveUser();
            break;
        case IDC_BTN_SEARCH_PLATE:
            handleSearchPlate();
            break;
        case IDC_BTN_LOAD_HISTORY:
            loadPersonalHistory();
            break;
        case IDC_BTN_LOAD_BILLS:
            loadBills();
            break;
        case IDC_BTN_PAY_BILL:
            handlePayBill();
            break;
        case IDC_BTN_LOAD_MONTHLY:
            loadMonthlyStats();
            break;
        case IDC_BTN_LOAD_MAP:
            handleLoadMap();
            break;
        case IDC_BTN_UNDO:
            handleUndo();
            break;
        default:
            (void)hwnd;
            break;
    }
}

LRESULT CALLBACK PanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_COMMAND || message == WM_NOTIFY) {
        return SendMessageW(g_mainWindow, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_NOTIFY: {
            NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
            if (header && header->hwndFrom == g_tab && header->code == TCN_SELCHANGE) {
                selectPanel(TabCtrl_GetCurSel(g_tab));
            }
            return 0;
        }
        case WM_COMMAND:
            handleCommand(hwnd, LOWORD(wParam));
            return 0;
        case WM_SIZE: {
            if (!g_tab) {
                break;
            }
            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);
            SetWindowPos(g_tab, nullptr, 0, 0, width, height, SWP_NOZORDER);
            RECT rect{};
            GetClientRect(g_tab, &rect);
            TabCtrl_AdjustRect(g_tab, FALSE, &rect);
            for (int index = 0; index < PANEL_COUNT; ++index) {
                SetWindowPos(
                    g_panels[index],
                    nullptr,
                    rect.left,
                    rect.top,
                    rect.right - rect.left,
                    rect.bottom - rect.top,
                    SWP_NOZORDER
                );
            }
            return 0;
        }
        case WM_DESTROY:
            if (g_system) {
                g_system->saveAll();
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

std::string commandLineDataDirectory() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::string result = ".";
    if (argv && argc >= 2) {
        result = toUtf8(argv[1]);
    }
    if (argv) {
        LocalFree(argv);
    }
    return result.empty() ? "." : result;
}

bool registerWindowClasses() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = g_instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = L"ParkingSharedBackendGUI";
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&windowClass)) {
        return false;
    }

    WNDCLASSEXW panelClass{};
    panelClass.cbSize = sizeof(panelClass);
    panelClass.lpfnWndProc = PanelProc;
    panelClass.hInstance = g_instance;
    panelClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    panelClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    panelClass.lpszClassName = L"ParkingSharedBackendPanel";

    return RegisterClassExW(&panelClass) != 0;
}

void createMainWindow(int showCommand) {
    g_mainWindow = CreateWindowExW(
        0,
        L"ParkingSharedBackendGUI",
        L"Hệ thống Quản lý Nhà xe",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1040,
        680,
        nullptr,
        nullptr,
        g_instance,
        nullptr
    );

    g_tab = CreateWindowExW(
        0,
        WC_TABCONTROLW,
        L"",
        WS_CHILD | WS_VISIBLE | TCS_HOTTRACK,
        0,
        0,
        1040,
        640,
        g_mainWindow,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MAIN_TAB)),
        g_instance,
        nullptr
    );
    SendMessageW(g_tab, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);

    const wchar_t* names[PANEL_COUNT] = {
        L"Tổng quan",
        L"Vào/Ra",
        L"Người dùng",
        L"Đang gửi",
        L"Bản đồ",
        L"Lịch sử",
        L"Hóa đơn",
        L"Blacklist",
        L"Thống kê",
        L"Undo"
    };

    for (int index = 0; index < PANEL_COUNT; ++index) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPWSTR>(names[index]);
        TabCtrl_InsertItem(g_tab, index, &item);
    }

    RECT rect{};
    GetClientRect(g_tab, &rect);
    TabCtrl_AdjustRect(g_tab, FALSE, &rect);

    for (int index = 0; index < PANEL_COUNT; ++index) {
        g_panels[index] = CreateWindowExW(
            0,
            L"ParkingSharedBackendPanel",
            L"",
            WS_CHILD | (index == 0 ? WS_VISIBLE : 0),
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            g_tab,
            nullptr,
            g_instance,
            nullptr
        );
    }

    createPanels();
    refreshAll();

    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    setText(panelControl(PANEL_MONTHLY, IDC_MONTH_YEAR), std::to_string(local.tm_year + 1900));
    setText(panelControl(PANEL_MONTHLY, IDC_MONTH_MONTH), std::to_string(local.tm_mon + 1));

    ShowWindow(g_mainWindow, showCommand);
    UpdateWindow(g_mainWindow);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    g_instance = instance;
    SetConsoleOutputCP(CP_UTF8);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&controls);

    g_fontUi = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_fontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_fontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Consolas");

    g_dataDirectory = commandLineDataDirectory();
    g_system = new ParkingSystem(g_dataDirectory);
    std::string error;
    if (!g_system->initialize(&error)) {
        showError(nullptr, "Không thể khởi tạo dữ liệu: " + error);
        delete g_system;
        g_system = nullptr;
        return 1;
    }

    if (!registerWindowClasses()) {
        showError(nullptr, "Không thể đăng ký lớp cửa sổ Win32.");
        delete g_system;
        g_system = nullptr;
        return 1;
    }

    createMainWindow(showCommand);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DeleteObject(g_fontUi);
    DeleteObject(g_fontBold);
    DeleteObject(g_fontMono);
    delete g_system;
    g_system = nullptr;

    return static_cast<int>(message.wParam);
}
