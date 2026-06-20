#ifndef MODELS_TUI_HPP
#define MODELS_TUI_HPP

#include <cstddef>
#include <ctime>
#include <string>

// ============================================================
// USER
// ============================================================

enum class UserRole {
    STUDENT = 0,
    LECTURER = 1,
    GUEST = 2
};

inline bool isValidUserRole(int value) {
    return value >= 0 && value <= 2;
}

inline std::string roleToString(UserRole role) {
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

struct User {
    std::string id;
    std::string name;
    std::string licensePlate;
    UserRole role;

    // Tổng số đơn vị gửi xe đã hoàn thành.
    // Mỗi tối đa 10 giờ được tính là một đơn vị gửi xe.
    long long totalParkingUnits;

    User()
        : role(UserRole::STUDENT),
          totalParkingUnits(0) {}

    User(
        const std::string& userId,
        const std::string& fullName,
        const std::string& plate,
        UserRole userRole,
        long long parkingUnits = 0
    )
        : id(userId),
          name(fullName),
          licensePlate(plate),
          role(userRole),
          totalParkingUnits(parkingUnits) {}

    bool operator==(const User& other) const {
        return id == other.id;
    }
};

// ============================================================
// POSITION
// ============================================================

struct Position {
    int row;
    int col;

    Position()
        : row(-1), col(-1) {}

    Position(int r, int c)
        : row(r), col(c) {}

    bool valid() const {
        return row >= 0 && col >= 0;
    }

    bool operator==(const Position& other) const {
        return row == other.row &&
               col == other.col;
    }
};

// ============================================================
// PARKING SLOT
// ============================================================

enum class SlotStatus {
    EMPTY = 0,
    OCCUPIED = 1
};

struct ParkingSlot {
    // ID số dùng cho AVL và tương thích code cũ.
    int slotId;

    // Mã hiển thị như A103, E511, I713.
    std::string slotCode;

    // Vị trí trên map.
    int row;
    int col;

    // Khoảng cách từ cổng.
    int distanceFromEntry;

    /*
     * Loại ô ban đầu:
     * P: sinh viên hoặc khách.
     * T: giảng viên.
     *
     * Khi có xe, map hiển thị X nhưng baseType vẫn giữ P/T.
     */
    char baseType;

    SlotStatus status;
    std::string licensePlate;
    std::string userId;

    ParkingSlot()
        : slotId(0),
          row(-1),
          col(-1),
          distanceFromEntry(-1),
          baseType('P'),
          status(SlotStatus::EMPTY) {}

    // Giữ tương thích với ParkingSlot(slotId) trong code cũ.
    explicit ParkingSlot(int id)
        : slotId(id),
          row(-1),
          col(-1),
          distanceFromEntry(-1),
          baseType('P'),
          status(SlotStatus::EMPTY) {}

    ParkingSlot(
        int id,
        const std::string& code,
        int mapRow,
        int mapCol,
        int distance,
        char type
    )
        : slotId(id),
          slotCode(code),
          row(mapRow),
          col(mapCol),
          distanceFromEntry(distance),
          baseType(type),
          status(SlotStatus::EMPTY) {}

    bool empty() const {
        return status == SlotStatus::EMPTY;
    }

    bool occupied() const {
        return status == SlotStatus::OCCUPIED;
    }

    bool accepts(UserRole role) const {
        if (role == UserRole::LECTURER) {
            return baseType == 'T';
        }

        return baseType == 'P';
    }

    char displaySymbol() const {
        return occupied() ? 'X' : baseType;
    }

    bool operator<(const ParkingSlot& other) const {
        return slotId < other.slotId;
    }

    bool operator>(const ParkingSlot& other) const {
        return slotId > other.slotId;
    }

    bool operator==(const ParkingSlot& other) const {
        return slotId == other.slotId;
    }
};

// ============================================================
// ACTIVE PARKING SESSION
// ============================================================

struct ActiveSession {
    std::string sessionId;
    std::string userId;
    std::string licensePlate;

    int slotId;
    std::string slotCode;

    std::time_t entryTime;
    std::string entryTimestamp;

    ActiveSession()
        : slotId(0),
          entryTime(0) {}

    ActiveSession(
        const std::string& session,
        const std::string& user,
        const std::string& plate,
        int parkingSlotId,
        const std::string& parkingSlotCode,
        std::time_t time,
        const std::string& timestamp
    )
        : sessionId(session),
          userId(user),
          licensePlate(plate),
          slotId(parkingSlotId),
          slotCode(parkingSlotCode),
          entryTime(time),
          entryTimestamp(timestamp) {}
};

// ============================================================
// PARKING HISTORY
// ============================================================

enum class ParkingActionType {
    ENTRY,
    EXIT,
    UNDO_ENTRY,
    UNDO_EXIT,
    PAYMENT
};

inline std::string parkingActionToString(
    ParkingActionType action
) {
    switch (action) {
        case ParkingActionType::ENTRY:
            return "VAO";

        case ParkingActionType::EXIT:
            return "RA";

        case ParkingActionType::UNDO_ENTRY:
            return "HOAN_TAC_VAO";

        case ParkingActionType::UNDO_EXIT:
            return "HOAN_TAC_RA";

        case ParkingActionType::PAYMENT:
            return "THANH_TOAN";
    }

    return "KHONG_XAC_DINH";
}

struct ParkingRecord {
    std::string eventId;
    std::string sessionId;

    std::string userId;
    std::string licensePlate;
    std::string name;

    int slotId;
    std::string slotCode;

    // Giữ dạng chuỗi để tương thích code hiện tại.
    std::string action;
    std::string timestamp;

    // Thời gian Unix dùng để tính toán chính xác.
    std::time_t eventTime;

    long long durationSeconds;
    long long chargedUnits;
    long long amount;

    ParkingRecord()
        : slotId(0),
          eventTime(0),
          durationSeconds(0),
          chargedUnits(0),
          amount(0) {}

    /*
     * Constructor tương thích với ParkingSystem hiện tại:
     *
     * ParkingRecord(
     *     id, plate, name, slotId, action, timestamp
     * )
     */
    ParkingRecord(
        const std::string& user,
        const std::string& plate,
        const std::string& fullName,
        int parkingSlotId,
        const std::string& parkingAction,
        const std::string& timeText
    )
        : userId(user),
          licensePlate(plate),
          name(fullName),
          slotId(parkingSlotId),
          action(parkingAction),
          timestamp(timeText),
          eventTime(0),
          durationSeconds(0),
          chargedUnits(0),
          amount(0) {}

    ParkingRecord(
        const std::string& event,
        const std::string& session,
        const std::string& user,
        const std::string& plate,
        const std::string& fullName,
        int parkingSlotId,
        const std::string& parkingSlotCode,
        ParkingActionType parkingAction,
        std::time_t time,
        const std::string& timeText,
        long long duration = 0,
        long long units = 0,
        long long parkingAmount = 0
    )
        : eventId(event),
          sessionId(session),
          userId(user),
          licensePlate(plate),
          name(fullName),
          slotId(parkingSlotId),
          slotCode(parkingSlotCode),
          action(parkingActionToString(parkingAction)),
          timestamp(timeText),
          eventTime(time),
          durationSeconds(duration),
          chargedUnits(units),
          amount(parkingAmount) {}
};

// ============================================================
// MONTHLY USAGE
// ============================================================

struct MonthlyUsage {
    std::string userId;
    int year;
    int month;

    long long parkingUnits;
    long long amount;

    MonthlyUsage()
        : year(0),
          month(0),
          parkingUnits(0),
          amount(0) {}

    MonthlyUsage(
        const std::string& user,
        int usageYear,
        int usageMonth
    )
        : userId(user),
          year(usageYear),
          month(usageMonth),
          parkingUnits(0),
          amount(0) {}
};

// ============================================================
// MONTHLY BILL
// ============================================================

struct MonthlyBill {
    std::string billId;
    std::string userId;

    int year;
    int month;

    long long parkingUnits;
    long long baseAmount;

    std::time_t createdAt;
    std::time_t dueDate;

    bool paid;
    std::time_t paidAt;

    MonthlyBill()
        : year(0),
          month(0),
          parkingUnits(0),
          baseAmount(0),
          createdAt(0),
          dueDate(0),
          paid(false),
          paidAt(0) {}

    MonthlyBill(
        const std::string& bill,
        const std::string& user,
        int billYear,
        int billMonth,
        long long units,
        long long amount,
        std::time_t created,
        std::time_t due
    )
        : billId(bill),
          userId(user),
          year(billYear),
          month(billMonth),
          parkingUnits(units),
          baseAmount(amount),
          createdAt(created),
          dueDate(due),
          paid(false),
          paidAt(0) {}

    bool isOverdue(std::time_t now) const {
        return !paid &&
               dueDate > 0 &&
               now > dueDate;
    }

    long long overdueDays(std::time_t now) const {
        if (!isOverdue(now)) {
            return 0;
        }

        const long long seconds =
            static_cast<long long>(now - dueDate);

        /*
         * Quá hạn dưới 24 giờ vẫn được xem là trễ 1 ngày.
         */
        return (seconds + 86400 - 1) / 86400;
    }

    long long penalty(
        std::time_t now,
        long long penaltyPerDay = 10000
    ) const {
        return overdueDays(now) * penaltyPerDay;
    }

    long long totalDue(
        std::time_t now,
        long long penaltyPerDay = 10000
    ) const {
        if (paid) {
            return 0;
        }

        return baseAmount +
               penalty(now, penaltyPerDay);
    }
};

// ============================================================
// UNDO
// ============================================================

enum class AdminActionType {
    ADD_USER,
    REMOVE_USER,
    VEHICLE_ENTRY,
    VEHICLE_EXIT,
    PAYMENT
};

struct AdminAction {
    AdminActionType type;
    std::string description;

    User userData;
    ActiveSession sessionData;
    ParkingRecord recordData;
    MonthlyBill billData;

    long long previousParkingUnits;
    long long previousAmount;
    bool previousPaidStatus;

    AdminAction()
        : type(AdminActionType::ADD_USER),
          previousParkingUnits(0),
          previousAmount(0),
          previousPaidStatus(false) {}
};

using UndoAction = AdminAction;

// ============================================================
// COMMON CALCULATIONS
// ============================================================

inline long long calculateParkingUnits(
    long long durationSeconds
) {
    static constexpr long long BLOCK_SECONDS =
        10LL * 60LL * 60LL;

    /*
     * Một phiên gửi xe luôn tính ít nhất một lần.
     *
     * 0–10 giờ      -> 1 lần
     * trên 10–20 giờ -> 2 lần
     * trên 20–30 giờ -> 3 lần
     */
    if (durationSeconds <= 0) {
        return 1;
    }

    return (
        durationSeconds +
        BLOCK_SECONDS -
        1
    ) / BLOCK_SECONDS;
}

inline long long calculateParkingAmount(
    UserRole role,
    long long parkingUnits,
    long long studentFee = 3000,
    long long lecturerFee = 5000,
    long long guestFee = 5000
) {
    if (parkingUnits <= 0) {
        return 0;
    }

    switch (role) {
        case UserRole::STUDENT:
            return parkingUnits * studentFee;

        case UserRole::LECTURER:
            return parkingUnits * lecturerFee;

        case UserRole::GUEST:
            return parkingUnits * guestFee;
    }

    return 0;
}

#endif // MODELS_TUI_HPP