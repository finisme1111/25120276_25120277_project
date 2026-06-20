#include "ParkingSystem.hpp"
#include "Terminal.hpp"

#include <cctype>
#include <clocale>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

const char* roleName(UserRole role) {
    switch (role) {
        case UserRole::STUDENT:
            return "Sinh viên";
        case UserRole::LECTURER:
            return "Giảng viên";
        case UserRole::GUEST:
            return "Khách ngoài trường";
    }
    return "Không xác định";
}

std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() &&
           std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }

    std::size_t last = text.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }

    return text.substr(first, last - first);
}

std::string toUpperAscii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(
            std::toupper(static_cast<unsigned char>(ch))
        );
    }
    return text;
}

int clampInt(int value, int minimum, int maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

std::string formatMoney(long long amount) {
    const bool negative = amount < 0;
    unsigned long long value = negative
        ? static_cast<unsigned long long>(-(amount + 1)) + 1ULL
        : static_cast<unsigned long long>(amount);

    std::string digits = std::to_string(value);
    std::string result;

    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) {
            result += '.';
        }
        result += digits[i];
    }

    if (negative) {
        result.insert(result.begin(), '-');
    }

    result += " đ";
    return result;
}

std::string formatDuration(long long seconds) {
    if (seconds < 0) {
        seconds = 0;
    }

    const long long days = seconds / 86400;
    seconds %= 86400;
    const long long hours = seconds / 3600;
    seconds %= 3600;
    const long long minutes = seconds / 60;
    const long long remainingSeconds = seconds % 60;

    std::ostringstream output;
    if (days > 0) {
        output << days << " ngày ";
    }
    output << hours << " giờ "
           << minutes << " phút "
           << remainingSeconds << " giây";
    return output.str();
}

std::string formatTimestamp(std::time_t time) {
    if (time <= 0) {
        return "-";
    }

    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif

    std::ostringstream output;
    output << std::put_time(&local, "%d/%m/%Y %H:%M:%S");
    return output.str();
}

void printDivider(char character = '=') {
    const tui::TerminalSize size = tui::getTerminalSize();
    int width = size.columns - 2;
    if (width < 30) {
        width = 30;
    }
    if (width > 100) {
        width = 100;
    }

    for (int i = 0; i < width; ++i) {
        std::cout << character;
    }
    std::cout << '\n';
}

void drawHeader(const std::string& title) {
    tui::clearScreen();
    tui::setBold();
    tui::setForeground(tui::Color::BrightCyan);
    std::cout << "HỆ THỐNG QUẢN LÝ NHÀ XE\n";
    tui::resetStyle();
    printDivider('=');
    tui::setBold();
    std::cout << title << '\n';
    tui::resetStyle();
    printDivider('-');
}

void printSuccess(const std::string& message) {
    tui::setForeground(tui::Color::BrightGreen);
    std::cout << "[THÀNH CÔNG] " << message << '\n';
    tui::resetStyle();
}

void printError(const std::string& message) {
    tui::setForeground(tui::Color::BrightRed);
    std::cout << "[LỖI] " << message << '\n';
    tui::resetStyle();
}

void printWarning(const std::string& message) {
    tui::setForeground(tui::Color::BrightYellow);
    std::cout << "[CẢNH BÁO] " << message << '\n';
    tui::resetStyle();
}

void pauseScreen() {
    std::cout << '\n';
    tui::waitForEnter("Nhấn Enter để tiếp tục...");
}

std::string readText(
    const std::string& prompt,
    const std::string& defaultValue = ""
) {
    tui::showCursor();

    std::cout << prompt;
    if (!defaultValue.empty()) {
        std::cout << " [" << defaultValue << "]";
    }
    std::cout << ": ";
    std::cout.flush();

    std::string value;
    std::getline(std::cin, value);
    value = trim(value);

    tui::hideCursor();

    if (value.empty()) {
        return defaultValue;
    }
    return value;
}

bool confirm(const std::string& question) {
    int selected = 0;
    const char* choices[2] = {"Có", "Không"};

    while (true) {
        std::cout << "\r\033[2K" << question << "  ";

        for (int i = 0; i < 2; ++i) {
            if (i == selected) {
                tui::setForeground(tui::Color::BrightCyan);
                tui::setBold();
                std::cout << "[" << choices[i] << "] ";
                tui::resetStyle();
            } else {
                std::cout << " " << choices[i] << "  ";
            }
        }
        std::cout.flush();

        const tui::KeyEvent event = tui::readKey();
        if (event.key == tui::Key::Left || event.key == tui::Key::Right) {
            selected = 1 - selected;
        } else if (event.key == tui::Key::Enter) {
            std::cout << '\n';
            return selected == 0;
        } else if (event.key == tui::Key::Escape) {
            std::cout << '\n';
            return false;
        } else if (event.key == tui::Key::Character) {
            const char key = static_cast<char>(
                std::tolower(static_cast<unsigned char>(event.character))
            );
            if (key == 'y' || key == 'c') {
                std::cout << '\n';
                return true;
            }
            if (key == 'n' || key == 'k') {
                std::cout << '\n';
                return false;
            }
        }
    }
}

int selectMenu(
    const std::string& title,
    const char* const items[],
    int itemCount,
    int initialSelection = 0
) {
    int selected = clampInt(initialSelection, 0, itemCount - 1);

    while (true) {
        drawHeader(title);

        for (int i = 0; i < itemCount; ++i) {
            if (i == selected) {
                tui::setForeground(tui::Color::Black);
                tui::setBackground(tui::Color::BrightCyan);
                tui::setBold();
                std::cout << " > " << items[i] << " ";
                tui::resetStyle();
                std::cout << '\n';
            } else {
                std::cout << "   " << items[i] << '\n';
            }
        }

        std::cout << '\n'
                  << "↑/↓: Di chuyển  Enter: Chọn  Esc: Quay lại\n";

        const tui::KeyEvent event = tui::readKey();
        if (event.key == tui::Key::Up) {
            selected = selected == 0 ? itemCount - 1 : selected - 1;
        } else if (event.key == tui::Key::Down) {
            selected = selected == itemCount - 1 ? 0 : selected + 1;
        } else if (event.key == tui::Key::Home) {
            selected = 0;
        } else if (event.key == tui::Key::End) {
            selected = itemCount - 1;
        } else if (event.key == tui::Key::Enter) {
            return selected;
        } else if (event.key == tui::Key::Escape) {
            return -1;
        }
    }
}

void showPagedLines(
    const std::string& title,
    const std::vector<std::string>& lines
) {
    int offset = 0;

    while (true) {
        drawHeader(title);

        const tui::TerminalSize terminalSize = tui::getTerminalSize();
        int pageSize = terminalSize.rows - 8;
        if (pageSize < 5) {
            pageSize = 5;
        }

        const int lineCount = static_cast<int>(lines.size());
        const int maxOffset = lineCount > pageSize
            ? lineCount - pageSize
            : 0;
        offset = clampInt(offset, 0, maxOffset);

        if (lines.empty()) {
            std::cout << "Không có dữ liệu.\n";
        } else {
            const int end = offset + pageSize < lineCount
                ? offset + pageSize
                : lineCount;

            for (int i = offset; i < end; ++i) {
                std::cout << lines[static_cast<std::size_t>(i)] << '\n';
            }

            std::cout << '\n'
                      << "Dòng " << offset + 1 << "-" << end
                      << "/" << lineCount << '\n';
        }

        std::cout << "↑/↓: Cuộn  PgUp/PgDn: Cuộn trang  Esc/Enter: Quay lại\n";

        const tui::KeyEvent event = tui::readKey();
        if (event.key == tui::Key::Up) {
            --offset;
        } else if (event.key == tui::Key::Down) {
            ++offset;
        } else if (event.key == tui::Key::PageUp) {
            offset -= pageSize;
        } else if (event.key == tui::Key::PageDown) {
            offset += pageSize;
        } else if (event.key == tui::Key::Home) {
            offset = 0;
        } else if (event.key == tui::Key::End) {
            offset = maxOffset;
        } else if (
            event.key == tui::Key::Escape ||
            event.key == tui::Key::Enter
        ) {
            return;
        }
    }
}

bool selectRole(UserRole& role) {
    const char* roles[] = {
        "Sinh viên",
        "Giảng viên",
        "Khách ngoài trường"
    };

    const int selected = selectMenu(
        "CHỌN PHÂN LOẠI NGƯỜI DÙNG",
        roles,
        3
    );

    if (selected < 0) {
        return false;
    }

    if (selected == 1) {
        role = UserRole::LECTURER;
    } else if (selected == 2) {
        role = UserRole::GUEST;
    } else {
        role = UserRole::STUDENT;
    }

    return true;
}

bool registerUserScreen(
    ParkingSystem& system,
    const std::string& presetId = ""
) {
    drawHeader("ĐĂNG KÝ NGƯỜI DÙNG");

    const std::string userId = readText("ID", presetId);
    if (userId.empty()) {
        printWarning("Đã hủy đăng ký.");
        pauseScreen();
        return false;
    }

    const std::string fullName = readText("Họ và tên");
    const std::string licensePlate = toUpperAscii(
        readText("Biển số xe")
    );

    if (fullName.empty() || licensePlate.empty()) {
        printError("Họ tên và biển số xe không được để trống.");
        pauseScreen();
        return false;
    }

    UserRole role = UserRole::STUDENT;
    if (!selectRole(role)) {
        drawHeader("ĐĂNG KÝ NGƯỜI DÙNG");
        printWarning("Đã hủy đăng ký.");
        pauseScreen();
        return false;
    }

    drawHeader("XÁC NHẬN ĐĂNG KÝ");
    std::cout << "ID: " << userId << '\n'
              << "Họ tên: " << fullName << '\n'
              << "Biển số: " << licensePlate << '\n'
              << "Phân loại: " << roleName(role) << '\n';

    if (!confirm("Lưu người dùng này?")) {
        printWarning("Đã hủy đăng ký.");
        pauseScreen();
        return false;
    }

    std::string error;
    if (!system.registerUser(
            userId,
            fullName,
            licensePlate,
            role,
            error
        )) {
        printError(error);
        pauseScreen();
        return false;
    }

    printSuccess("Đăng ký người dùng thành công.");
    pauseScreen();
    return true;
}

void vehicleEntryScreen(ParkingSystem& system) {
    drawHeader("XE VÀO");
    const std::string userId = readText("Nhập ID");

    if (userId.empty()) {
        return;
    }

    if (!system.findUser(userId)) {
        printWarning("ID chưa được đăng ký.");
        if (!confirm("Đăng ký ngay?")) {
            pauseScreen();
            return;
        }

        if (!registerUserScreen(system, userId)) {
            return;
        }
    }

    std::string error;
    ActiveSession session;

    if (!system.vehicleEntry(userId, session, error)) {
        drawHeader("XE VÀO");
        printError(error);

        const long long outstanding = system.outstandingAmount(userId);
        if (outstanding > 0) {
            std::cout << "Tổng tiền chưa thanh toán: "
                      << formatMoney(outstanding) << '\n';
        }

        pauseScreen();
        return;
    }

    const User* user = system.findUser(userId);

    drawHeader("XE VÀO THÀNH CÔNG");
    if (user) {
        std::cout << "Họ tên: " << user->name << '\n'
                  << "Biển số: " << user->licensePlate << '\n'
                  << "Phân loại: " << roleName(user->role) << '\n';
    }
    std::cout << "Ô được chỉ định: " << session.slotCode << '\n'
              << "Thời gian vào: " << session.entryTimestamp << '\n'
              << "Vui lòng đỗ đúng vị trí được chỉ định.\n";

    pauseScreen();
}

void vehicleExitScreen(ParkingSystem& system) {
    drawHeader("XE RA / TÌM VỊ TRÍ XE");
    const std::string userId = readText("Nhập ID lần thứ nhất");

    if (userId.empty()) {
        return;
    }

    ActiveSession session;
    std::string error;

    if (!system.previewVehicleExit(userId, session, error)) {
        printError(error);
        pauseScreen();
        return;
    }

    const User* user = system.findUser(userId);

    drawHeader("THÔNG TIN XE ĐANG GỬI");
    if (user) {
        std::cout << "Họ tên: " << user->name << '\n'
                  << "Biển số: " << user->licensePlate << '\n';
    }
    std::cout << "Vị trí xe: " << session.slotCode << '\n'
              << "Thời gian vào: " << session.entryTimestamp << '\n';
    printDivider('-');

    const std::string confirmationId = readText(
        "Nhập lại ID để xác nhận lấy xe"
    );

    if (confirmationId.empty()) {
        printWarning("Đã hủy thao tác xe ra.");
        pauseScreen();
        return;
    }

    ParkingRecord record;
    if (!system.vehicleExit(
            userId,
            confirmationId,
            record,
            error
        )) {
        printError(error);
        pauseScreen();
        return;
    }

    drawHeader("XE RA THÀNH CÔNG");
    std::cout << "Vị trí vừa giải phóng: " << record.slotCode << '\n'
              << "Thời gian gửi: "
              << formatDuration(record.durationSeconds) << '\n'
              << "Số lượt tính phí: " << record.chargedUnits << '\n'
              << "Số tiền cộng vào tháng: "
              << formatMoney(record.amount) << '\n';
    printSuccess("Đã ghi nhận một lượt gửi xe hoàn tất.");
    pauseScreen();
}

void searchUserScreen(ParkingSystem& system) {
    drawHeader("TRA CỨU NGƯỜI DÙNG");
    const std::string userId = readText("Nhập ID");

    if (userId.empty()) {
        return;
    }

    const User* user = system.findUser(userId);
    if (!user) {
        printError("Không tìm thấy người dùng.");
        pauseScreen();
        return;
    }

    drawHeader("THÔNG TIN NGƯỜI DÙNG");
    std::cout << "ID: " << user->id << '\n'
              << "Họ tên: " << user->name << '\n'
              << "Biển số: " << user->licensePlate << '\n'
              << "Phân loại: " << roleName(user->role) << '\n'
              << "Tổng lượt gửi đã tính: "
              << user->totalParkingUnits << '\n';

    const ActiveSession* session = system.findActiveSession(userId);
    if (session) {
        std::cout << "Trạng thái: Đang gửi xe\n"
                  << "Vị trí: " << session->slotCode << '\n'
                  << "Thời gian vào: " << session->entryTimestamp << '\n';
    } else {
        std::cout << "Trạng thái: Không có xe trong bãi\n";
    }

    const bool blacklisted = system.isBlacklisted(userId);
    std::cout << "Danh sách đen: "
              << (blacklisted ? "Có" : "Không") << '\n'
              << "Tổng tiền chưa thanh toán: "
              << formatMoney(system.outstandingAmount(userId)) << '\n';

    pauseScreen();
}

void listUsersScreen(ParkingSystem& system) {
    std::vector<std::string> lines;
    lines.push_back("ID | Họ tên | Biển số | Phân loại | Tổng lượt");
    lines.push_back("--------------------------------------------------------------------------");

    system.forEachUser([&](const User& user) {
        std::ostringstream line;
        line << user.id << " | "
             << user.name << " | "
             << user.licensePlate << " | "
             << roleName(user.role) << " | "
             << user.totalParkingUnits;
        lines.push_back(line.str());
    });

    showPagedLines("DANH SÁCH NGƯỜI DÙNG", lines);
}

void activeSessionsScreen(ParkingSystem& system) {
    std::vector<std::string> lines;
    lines.push_back("ID | Biển số | Vị trí | Thời gian vào");
    lines.push_back("--------------------------------------------------------------------------");

    system.forEachActiveSession([&](const ActiveSession& session) {
        std::ostringstream line;
        line << session.userId << " | "
             << session.licensePlate << " | "
             << session.slotCode << " | "
             << session.entryTimestamp;
        lines.push_back(line.str());
    });

    showPagedLines("DANH SÁCH XE ĐANG GỬI", lines);
}

void personalHistoryScreen(ParkingSystem& system) {
    drawHeader("LỊCH SỬ GỬI XE CÁ NHÂN");
    const std::string userId = readText("Nhập ID");

    if (userId.empty()) {
        return;
    }

    const User* user = system.findUser(userId);
    if (!user) {
        printError("Không tìm thấy người dùng.");
        pauseScreen();
        return;
    }

    std::vector<std::string> lines;
    lines.push_back(
        "Thời gian | Hành động | Vị trí | Thời lượng | Lượt | Số tiền"
    );
    lines.push_back(
        "------------------------------------------------------------------------------------------"
    );

    system.forEachPersonalHistory(
        userId,
        [&](const ParkingRecord& record) {
            std::ostringstream line;
            line << record.timestamp << " | "
                 << record.action << " | "
                 << (record.slotCode.empty() ? "-" : record.slotCode)
                 << " | ";

            if (record.durationSeconds > 0) {
                line << formatDuration(record.durationSeconds);
            } else {
                line << '-';
            }

            line << " | " << record.chargedUnits
                 << " | " << formatMoney(record.amount);
            lines.push_back(line.str());
        }
    );

    showPagedLines(
        "LỊCH SỬ CỦA " + user->name + " (" + userId + ")",
        lines
    );
}

void billsScreen(ParkingSystem& system) {
    drawHeader("HÓA ĐƠN VÀ THANH TOÁN");
    const std::string userId = readText("Nhập ID");

    if (userId.empty()) {
        return;
    }

    const User* user = system.findUser(userId);
    if (!user) {
        printError("Không tìm thấy người dùng.");
        pauseScreen();
        return;
    }

    system.processMonthlyBilling();

    std::vector<std::string> lines;
    bool hasBill = false;

    system.forEachBill(
        userId,
        [&](const MonthlyBill& bill, long long penalty, long long total) {
            hasBill = true;
            std::ostringstream line;
            line << bill.billId
                 << " | Tháng " << bill.month << '/' << bill.year
                 << " | Lượt: " << bill.parkingUnits
                 << " | Gốc: " << formatMoney(bill.baseAmount)
                 << " | Phạt: " << formatMoney(penalty)
                 << " | Tổng: " << formatMoney(total)
                 << " | " << (bill.paid ? "Đã thanh toán" : "Chưa thanh toán")
                 << " | Hạn: " << formatTimestamp(bill.dueDate);
            lines.push_back(line.str());
        }
    );

    drawHeader("HÓA ĐƠN CỦA " + user->name);
    if (!hasBill) {
        std::cout << "Người dùng chưa có hóa đơn tháng.\n";
        pauseScreen();
        return;
    }

    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }

    std::cout << '\n'
              << "Tổng tiền chưa thanh toán: "
              << formatMoney(system.outstandingAmount(userId)) << '\n';

    const std::string billId = readText(
        "Nhập mã hóa đơn cần thanh toán, để trống để quay lại"
    );

    if (billId.empty()) {
        return;
    }

    if (!confirm("Xác nhận thanh toán hóa đơn " + billId + "?")) {
        return;
    }

    long long paidAmount = 0;
    std::string error;
    if (!system.payBill(billId, paidAmount, error)) {
        printError(error);
        pauseScreen();
        return;
    }

    printSuccess("Thanh toán thành công: " + formatMoney(paidAmount));
    if (!system.isBlacklisted(userId)) {
        std::cout << "Người dùng hiện không còn bị chặn gửi xe.\n";
    }
    pauseScreen();
}

void blacklistScreen(ParkingSystem& system) {
    struct BlacklistRow {
        User user;
        int overdueBills;
        long long totalPenalty;
        long long totalDue;
    };

    std::vector<BlacklistRow> rows;

    system.processMonthlyBilling();
    system.forEachBlacklisted(
        [&](const User& user,
            const MonthlyBill&,
            long long penalty,
            long long total) {
            BlacklistRow* existing = nullptr;
            for (BlacklistRow& row : rows) {
                if (row.user.id == user.id) {
                    existing = &row;
                    break;
                }
            }

            if (!existing) {
                rows.push_back(BlacklistRow{user, 1, penalty, total});
            } else {
                ++existing->overdueBills;
                existing->totalPenalty += penalty;
                existing->totalDue += total;
            }
        }
    );

    std::vector<std::string> lines;
    lines.push_back(
        "ID | Họ tên | Biển số | Hóa đơn quá hạn | Tổng phạt | Tổng phải trả"
    );
    lines.push_back(
        "------------------------------------------------------------------------------------------"
    );

    for (const BlacklistRow& row : rows) {
        std::ostringstream line;
        line << row.user.id << " | "
             << row.user.name << " | "
             << row.user.licensePlate << " | "
             << row.overdueBills << " | "
             << formatMoney(row.totalPenalty) << " | "
             << formatMoney(row.totalDue);
        lines.push_back(line.str());
    }

    showPagedLines("DANH SÁCH ĐEN", lines);
}

void monthlyStatisticsScreen(ParkingSystem& system) {
    drawHeader("THỐNG KÊ THÁNG");

    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    int year = local.tm_year + 1900;
    int month = local.tm_mon + 1;

    const std::string yearText = readText(
        "Năm",
        std::to_string(year)
    );
    const std::string monthText = readText(
        "Tháng",
        std::to_string(month)
    );

    try {
        year = std::stoi(yearText);
        month = std::stoi(monthText);
    } catch (...) {
        printError("Năm hoặc tháng không hợp lệ.");
        pauseScreen();
        return;
    }

    if (year < 1970 || month < 1 || month > 12) {
        printError("Năm hoặc tháng không hợp lệ.");
        pauseScreen();
        return;
    }

    std::vector<std::string> lines;
    long long totalUnits = 0;
    long long totalAmount = 0;

    system.forEachMonthlyUsage(
        year,
        month,
        [&](const User& user, const MonthlyUsage& usage) {
            std::ostringstream line;
            line << user.id << " | "
                 << user.name << " | "
                 << roleName(user.role) << " | Lượt: "
                 << usage.parkingUnits << " | Tiền: "
                 << formatMoney(usage.amount);
            lines.push_back(line.str());
            totalUnits += usage.parkingUnits;
            totalAmount += usage.amount;
        }
    );

    std::ostringstream summary;
    summary << "TỔNG CỘNG | Lượt: " << totalUnits
            << " | Doanh thu ghi nhận: "
            << formatMoney(totalAmount);
    lines.insert(lines.begin(), summary.str());
    lines.insert(lines.begin() + 1, std::string(80, '-'));

    std::ostringstream title;
    title << "THỐNG KÊ THÁNG " << month << '/' << year;
    showPagedLines(title.str(), lines);
}

void undoScreen(ParkingSystem& system) {
    drawHeader("HOÀN TÁC THAO TÁC GẦN NHẤT");
    printWarning(
        "Chỉ hoàn tác được thao tác xe vào hoặc xe ra gần nhất trong phiên chạy hiện tại."
    );

    if (!confirm("Bạn chắc chắn muốn hoàn tác?")) {
        return;
    }

    std::string error;
    if (!system.undoLast(error)) {
        printError(error);
        pauseScreen();
        return;
    }

    printSuccess("Hoàn tác thành công.");
    pauseScreen();
}

void renderMapCharacter(char character) {
    switch (character) {
        case '#':
            tui::setForeground(tui::Color::BrightBlack);
            break;
        case 'P':
            tui::setForeground(tui::Color::BrightGreen);
            break;
        case 'T':
            tui::setForeground(tui::Color::BrightCyan);
            break;
        case 'X':
            tui::setForeground(tui::Color::BrightRed);
            tui::setBold();
            break;
        case 'E':
            tui::setForeground(tui::Color::BrightYellow);
            tui::setBold();
            break;
        default:
            tui::setForeground(tui::Color::Default);
            break;
    }

    std::cout << character;
    tui::resetStyle();
}

void mapScreen(ParkingSystem& system) {
    int top = 0;
    int left = 0;

    while (true) {
        const std::vector<std::string> lines =
            system.map().getMapLines();

        if (lines.empty()) {
            drawHeader("BẢN ĐỒ NHÀ XE");
            printError("Bản đồ chưa được tải.");
            pauseScreen();
            return;
        }

        const tui::TerminalSize terminalSize = tui::getTerminalSize();
        int viewHeight = terminalSize.rows - 8;
        int viewWidth = terminalSize.columns - 3;

        if (viewHeight < 5) {
            viewHeight = 5;
        }
        if (viewWidth < 20) {
            viewWidth = 20;
        }

        const int mapHeight = static_cast<int>(lines.size());
        const int mapWidth = static_cast<int>(lines[0].size());

        if (viewHeight > mapHeight) {
            viewHeight = mapHeight;
        }
        if (viewWidth > mapWidth) {
            viewWidth = mapWidth;
        }

        const int maxTop = mapHeight - viewHeight;
        const int maxLeft = mapWidth - viewWidth;
        top = clampInt(top, 0, maxTop);
        left = clampInt(left, 0, maxLeft);

        drawHeader("BẢN ĐỒ NHÀ XE");

        for (int row = top; row < top + viewHeight; ++row) {
            for (int col = left; col < left + viewWidth; ++col) {
                renderMapCharacter(
                    lines[static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(col)]
                );
            }
            std::cout << '\n';
        }

        std::cout << "\n# Tường  . Lối đi  ";
        tui::setForeground(tui::Color::BrightGreen);
        std::cout << "P Sinh viên/khách  ";
        tui::setForeground(tui::Color::BrightCyan);
        std::cout << "T Giảng viên  ";
        tui::setForeground(tui::Color::BrightRed);
        std::cout << "X Đã đỗ  ";
        tui::setForeground(tui::Color::BrightYellow);
        std::cout << "E Cổng";
        tui::resetStyle();
        std::cout << '\n';

        std::cout << "Góc nhìn: dòng " << top + 1
                  << ", cột " << left + 1
                  << " | Phím mũi tên: cuộn | Esc/Enter: quay lại\n";

        const tui::KeyEvent event = tui::readKey();
        if (event.key == tui::Key::Up) {
            --top;
        } else if (event.key == tui::Key::Down) {
            ++top;
        } else if (event.key == tui::Key::Left) {
            --left;
        } else if (event.key == tui::Key::Right) {
            ++left;
        } else if (event.key == tui::Key::PageUp) {
            top -= viewHeight;
        } else if (event.key == tui::Key::PageDown) {
            top += viewHeight;
        } else if (event.key == tui::Key::Home) {
            top = 0;
            left = 0;
        } else if (event.key == tui::Key::End) {
            top = maxTop;
            left = maxLeft;
        } else if (
            event.key == tui::Key::Escape ||
            event.key == tui::Key::Enter
        ) {
            return;
        }
    }
}

void dashboardScreen(ParkingSystem& system) {
    system.processMonthlyBilling();

    std::size_t overdueBillCount = 0;
    long long overdueTotal = 0;

    system.forEachBlacklisted(
        [&](const User&,
            const MonthlyBill&,
            long long,
            long long total) {
            ++overdueBillCount;
            overdueTotal += total;
        }
    );

    drawHeader("TỔNG QUAN");
    std::cout << "Tổng người dùng: " << system.userCount() << '\n'
              << "Xe đang gửi: " << system.activeVehicleCount() << '\n'
              << "Số bản ghi lịch sử: " << system.historyCount() << '\n'
              << "Hóa đơn đang quá hạn: " << overdueBillCount << '\n'
              << "Tổng nợ quá hạn hiện tại: "
              << formatMoney(overdueTotal) << '\n';

    printDivider('-');
    std::cout << "Phí sinh viên: "
              << formatMoney(ParkingSystem::STUDENT_FEE)
              << "/lượt\n"
              << "Phí giảng viên: "
              << formatMoney(ParkingSystem::LECTURER_FEE)
              << "/lượt\n"
              << "Phạt quá hạn: "
              << formatMoney(ParkingSystem::LATE_FEE_PER_DAY)
              << "/ngày\n";

    pauseScreen();
}

bool runMainMenu(ParkingSystem& system) {
    const char* items[] = {
        "Tổng quan hệ thống",
        "Xe vào",
        "Xe ra / tìm vị trí xe",
        "Đăng ký người dùng",
        "Tra cứu người dùng",
        "Danh sách người dùng",
        "Danh sách xe đang gửi",
        "Xem bản đồ nhà xe",
        "Lịch sử gửi xe cá nhân",
        "Hóa đơn và thanh toán",
        "Danh sách đen",
        "Thống kê tháng",
        "Hoàn tác thao tác gần nhất",
        "Thoát"
    };

    static int selected = 0;

    while (true) {
        system.processMonthlyBilling();

        std::ostringstream title;
        title << "MENU CHÍNH | Người dùng: " << system.userCount()
              << " | Xe đang gửi: " << system.activeVehicleCount();

        selected = selectMenu(
            title.str(),
            items,
            static_cast<int>(sizeof(items) / sizeof(items[0])),
            selected
        );

        if (selected < 0 || selected == 13) {
            return true;
        }

        switch (selected) {
            case 0:
                dashboardScreen(system);
                break;
            case 1:
                vehicleEntryScreen(system);
                break;
            case 2:
                vehicleExitScreen(system);
                break;
            case 3:
                registerUserScreen(system);
                break;
            case 4:
                searchUserScreen(system);
                break;
            case 5:
                listUsersScreen(system);
                break;
            case 6:
                activeSessionsScreen(system);
                break;
            case 7:
                mapScreen(system);
                break;
            case 8:
                personalHistoryScreen(system);
                break;
            case 9:
                billsScreen(system);
                break;
            case 10:
                blacklistScreen(system);
                break;
            case 11:
                monthlyStatisticsScreen(system);
                break;
            case 12:
                undoScreen(system);
                break;
            default:
                break;
        }
    }
}

void configureTerminal() {
    std::setlocale(LC_ALL, "");

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace

int main(int argc, char* argv[]) {
    configureTerminal();
    tui::CursorGuard cursorGuard;

    const std::string dataDirectory = argc >= 2 ? argv[1] : ".";
    ParkingSystem system(dataDirectory);
    std::string error;

    drawHeader("KHỞI ĐỘNG HỆ THỐNG");
    std::cout << "Đang đọc dữ liệu từ: " << dataDirectory << '\n';

    if (!system.initialize(&error)) {
        printError(error);
        std::cout << "\nKiểm tra các tệp DanhSachSV.txt, map.txt và quyền ghi thư mục.\n";
        pauseScreen();
        return 1;
    }

    printSuccess("Khởi tạo dữ liệu thành công.");

    runMainMenu(system);

    drawHeader("THOÁT CHƯƠNG TRÌNH");
    if (!system.saveAll()) {
        printError("Không thể lưu đầy đủ dữ liệu trước khi thoát.");
        pauseScreen();
        return 1;
    }

    printSuccess("Đã lưu dữ liệu. Tạm biệt!");
    return 0;
}
