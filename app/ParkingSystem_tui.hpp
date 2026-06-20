#ifndef PARKINGSYSTEM_TUI_HPP
#define PARKINGSYSTEM_TUI_HPP

#include "models_tui.hpp"
#include "ParkingMap_tui.hpp"
#include "../lib/HashTable.hpp"
#include "../lib/LinkedList.hpp"
#include "../lib/Stack.hpp"

#include <array>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/*
 * ParkingSystem requires ParkingMap.hpp to provide:
 *
 *   bool loadFromFile(const std::string& fileName);
 *   bool findNearestAvailableSlot(UserRole role, ParkingSlot& result) const;
 *   bool occupySlot(int slotId,
 *                   const std::string& userId,
 *                   const std::string& licensePlate);
 *   bool releaseSlot(int slotId);
 *   bool isSlotAvailable(int slotId) const;
 *   ParkingSlot* findSlot(int slotId);
 *   const ParkingSlot* findSlot(int slotId) const;
 *
 * map.txt is treated as the original map. Occupied positions are restored
 * from active_sessions.txt when the application starts.
 */
class ParkingSystem {
public:
    struct StoragePaths {
        std::string usersFile;
        std::string mapFile;
        std::string activeSessionsFile;
        std::string historyFile;
        std::string monthlyUsageFile;
        std::string monthlyBillsFile;
        std::string revenueFile;

        explicit StoragePaths(const std::string& directory = ".")
            : usersFile(join(directory, "DanhSachSV.txt")),
              mapFile(join(directory, "map.txt")),
              activeSessionsFile(join(directory, "active_sessions.txt")),
              historyFile(join(directory, "history.txt")),
              monthlyUsageFile(join(directory, "monthly_usage.txt")),
              monthlyBillsFile(join(directory, "monthly_bills.txt")),
              revenueFile(join(directory, "revenue.txt")) {}

    private:
        static std::string join(
            const std::string& directory,
            const std::string& fileName
        ) {
            if (directory.empty() || directory == ".") {
                return fileName;
            }

            const char last = directory[directory.size() - 1];
            if (last == '/' || last == '\\') {
                return directory + fileName;
            }

            return directory + "/" + fileName;
        }
    };

    static constexpr long long STUDENT_FEE = 3000;
    static constexpr long long LECTURER_FEE = 5000;

    // The requirement does not define the guest fee, so it is configurable
    // here and currently uses the lecturer fee.
    static constexpr long long GUEST_FEE = 5000;

    static constexpr long long LATE_FEE_PER_DAY = 10000;

    // A bill is generated at 00:00 on the first day of the following month.
    // The user has one day to pay before the bill becomes overdue.
    static constexpr int PAYMENT_GRACE_DAYS = 1;

private:
    struct LocalDate {
        int year;
        int month;
        int day;
    };

    StoragePaths paths;
    ParkingMap parkingMap;

    lib::HashTable<std::string, User> users;
    lib::HashTable<std::string, ActiveSession> activeSessions;
    lib::HashTable<std::string, MonthlyUsage> monthlyUsage;
    lib::HashTable<std::string, MonthlyBill> monthlyBills;

    lib::LinkedList<ParkingRecord> history;
    lib::Stack<AdminAction> undoStack;

    unsigned long long eventCounter;
    unsigned long long sessionCounter;
    bool initialized;

    static void removeTrailingCarriageReturn(std::string& value) {
        if (!value.empty() && value[value.size() - 1] == '\r') {
            value.erase(value.size() - 1);
        }
    }

    static bool readField(
        std::istringstream& stream,
        std::string& value
    ) {
        if (!std::getline(stream, value, '|')) {
            return false;
        }

        removeTrailingCarriageReturn(value);
        return true;
    }

    static std::vector<std::string> splitFields(
        const std::string& line
    ) {
        std::vector<std::string> fields;
        std::string field;
        std::istringstream stream(line);

        while (std::getline(stream, field, '|')) {
            removeTrailingCarriageReturn(field);
            fields.push_back(field);
        }

        if (!line.empty() && line[line.size() - 1] == '|') {
            fields.push_back("");
        }

        return fields;
    }

    static bool parseInt(const std::string& text, int& value) {
        try {
            std::size_t used = 0;
            const int parsed = std::stoi(text, &used);
            if (used != text.size()) {
                return false;
            }
            value = parsed;
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool parseLongLong(
        const std::string& text,
        long long& value
    ) {
        try {
            std::size_t used = 0;
            const long long parsed = std::stoll(text, &used);
            if (used != text.size()) {
                return false;
            }
            value = parsed;
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool parseTime(
        const std::string& text,
        std::time_t& value
    ) {
        long long parsed = 0;
        if (!parseLongLong(text, parsed)) {
            return false;
        }

        value = static_cast<std::time_t>(parsed);
        return true;
    }

    static bool parseTimestampText(
        const std::string& text,
        std::time_t& value
    ) {
        if (text.empty()) {
            return false;
        }

        std::tm parsed{};
        std::istringstream isoStream(text);
        isoStream >> std::get_time(&parsed, "%Y-%m-%dT%H:%M:%S");

        if (!isoStream.fail()) {
            parsed.tm_isdst = -1;
            value = std::mktime(&parsed);
            return value != static_cast<std::time_t>(-1);
        }

        parsed = std::tm{};
        std::istringstream legacyStream(text);
        legacyStream >> std::get_time(&parsed, "%d/%m/%Y %H:%M:%S");

        if (!legacyStream.fail()) {
            parsed.tm_isdst = -1;
            value = std::mktime(&parsed);
            return value != static_cast<std::time_t>(-1);
        }

        return false;
    }

    static std::tm toLocalTime(std::time_t time) {
        std::tm result{};

#if defined(_WIN32)
        localtime_s(&result, &time);
#else
        localtime_r(&time, &result);
#endif

        return result;
    }

    static LocalDate getLocalDate(std::time_t time) {
        const std::tm local = toLocalTime(time);
        return LocalDate{
            local.tm_year + 1900,
            local.tm_mon + 1,
            local.tm_mday
        };
    }

    static std::time_t makeLocalTime(
        int year,
        int month,
        int day,
        int hour = 0,
        int minute = 0,
        int second = 0
    ) {
        std::tm value{};
        value.tm_year = year - 1900;
        value.tm_mon = month - 1;
        value.tm_mday = day;
        value.tm_hour = hour;
        value.tm_min = minute;
        value.tm_sec = second;
        value.tm_isdst = -1;
        return std::mktime(&value);
    }

    static std::string formatTime(std::time_t time) {
        if (time <= 0) {
            return "";
        }

        const std::tm local = toLocalTime(time);
        std::ostringstream output;
        output << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
        return output.str();
    }

    static bool isEarlierMonth(
        int year,
        int month,
        int currentYear,
        int currentMonth
    ) {
        return year < currentYear ||
               (year == currentYear && month < currentMonth);
    }

    static std::string monthlyKey(
        const std::string& userId,
        int year,
        int month
    ) {
        std::ostringstream output;
        output << userId << '|' << year << '|'
               << std::setw(2) << std::setfill('0') << month;
        return output.str();
    }

    static std::string billIdFor(
        const std::string& userId,
        int year,
        int month
    ) {
        std::ostringstream output;
        output << "BILL-" << userId << '-'
               << year << '-'
               << std::setw(2) << std::setfill('0') << month;
        return output.str();
    }

    template <typename Writer>
    bool writeFileAtomically(
        const std::string& fileName,
        Writer writer
    ) const {
        const std::string temporaryFile = fileName + ".tmp";

        {
            std::ofstream output(
                temporaryFile,
                std::ios::trunc | std::ios::binary
            );

            if (!output) {
                return false;
            }

            writer(output);

            if (!output) {
                output.close();
                std::remove(temporaryFile.c_str());
                return false;
            }
        }

#if defined(_WIN32)
        std::remove(fileName.c_str());
#endif

        if (std::rename(temporaryFile.c_str(), fileName.c_str()) != 0) {
            std::remove(temporaryFile.c_str());
            return false;
        }

        return true;
    }

    std::string makeSessionId(
        const std::string& userId,
        std::time_t now
    ) {
        std::ostringstream output;
        output << "S-" << userId << '-'
               << static_cast<long long>(now) << '-'
               << ++sessionCounter;
        return output.str();
    }

    std::string makeEventId(std::time_t now) {
        std::ostringstream output;
        output << "E-" << static_cast<long long>(now)
               << '-' << ++eventCounter;
        return output.str();
    }

    static long long feeForRole(UserRole role) {
        switch (role) {
            case UserRole::STUDENT:
                return STUDENT_FEE;

            case UserRole::LECTURER:
                return LECTURER_FEE;

            case UserRole::GUEST:
                return GUEST_FEE;
        }

        return 0;
    }

    bool licensePlateExists(
        const std::string& licensePlate,
        const std::string& ignoredUserId = ""
    ) const {
        bool found = false;

        users.forEach(
            [&](const std::string& id, const User& user) {
                if (!found &&
                    id != ignoredUserId &&
                    user.licensePlate == licensePlate) {
                    found = true;
                }
            }
        );

        return found;
    }

    bool writeUsers() const {
        return writeFileAtomically(
            paths.usersFile,
            [&](std::ofstream& output) {
                users.forEach(
                    [&](const std::string&, const User& user) {
                        output << user.id << '|'
                               << user.name << '|'
                               << user.licensePlate << '|'
                               << static_cast<int>(user.role) << '|'
                               << user.totalParkingUnits << '\n';
                    }
                );
            }
        );
    }

    bool writeActiveSessions() const {
        return writeFileAtomically(
            paths.activeSessionsFile,
            [&](std::ofstream& output) {
                activeSessions.forEach(
                    [&](const std::string&, const ActiveSession& session) {
                        output << session.sessionId << '|'
                               << session.userId << '|'
                               << session.licensePlate << '|'
                               << session.slotId << '|'
                               << session.slotCode << '|'
                               << static_cast<long long>(session.entryTime) << '|'
                               << session.entryTimestamp << '\n';
                    }
                );
            }
        );
    }

    bool writeMonthlyUsage() const {
        return writeFileAtomically(
            paths.monthlyUsageFile,
            [&](std::ofstream& output) {
                monthlyUsage.forEach(
                    [&](const std::string&, const MonthlyUsage& usage) {
                        output << usage.userId << '|'
                               << usage.year << '|'
                               << usage.month << '|'
                               << usage.parkingUnits << '|'
                               << usage.amount << '\n';
                    }
                );
            }
        );
    }

    bool writeMonthlyBills() const {
        return writeFileAtomically(
            paths.monthlyBillsFile,
            [&](std::ofstream& output) {
                monthlyBills.forEach(
                    [&](const std::string&, const MonthlyBill& bill) {
                        output << bill.billId << '|'
                               << bill.userId << '|'
                               << bill.year << '|'
                               << bill.month << '|'
                               << bill.parkingUnits << '|'
                               << bill.baseAmount << '|'
                               << static_cast<long long>(bill.createdAt) << '|'
                               << static_cast<long long>(bill.dueDate) << '|'
                               << (bill.paid ? 1 : 0) << '|'
                               << static_cast<long long>(bill.paidAt) << '\n';
                    }
                );
            }
        );
    }

    bool appendHistory(ParkingRecord record) {
        history.insertBack(record);

        std::ofstream output(paths.historyFile, std::ios::app);
        if (!output) {
            history.removeBack();
            return false;
        }

        output << record.eventId << '|'
               << record.sessionId << '|'
               << record.userId << '|'
               << record.licensePlate << '|'
               << record.name << '|'
               << record.slotId << '|'
               << record.slotCode << '|'
               << record.action << '|'
               << static_cast<long long>(record.eventTime) << '|'
               << record.timestamp << '|'
               << record.durationSeconds << '|'
               << record.chargedUnits << '|'
               << record.amount << '\n';

        if (!output) {
            history.removeBack();
            return false;
        }

        return true;
    }

    bool loadUsers() {
        std::ifstream input(paths.usersFile);
        if (!input) {
            return false;
        }

        std::string line;
        while (std::getline(input, line)) {
            removeTrailingCarriageReturn(line);
            if (line.empty()) {
                continue;
            }

            std::istringstream stream(line);
            std::string id;
            std::string name;
            std::string plate;
            std::string roleText;
            std::string unitsText;

            if (!readField(stream, id) ||
                !readField(stream, name) ||
                !readField(stream, plate) ||
                !readField(stream, roleText)) {
                continue;
            }

            int roleValue = 0;
            if (!parseInt(roleText, roleValue) ||
                !isValidUserRole(roleValue)) {
                continue;
            }

            long long units = 0;
            if (readField(stream, unitsText)) {
                if (!parseLongLong(unitsText, units) || units < 0) {
                    units = 0;
                }
            }

            if (!users.contains(id)) {
                users.insert(
                    id,
                    User(
                        id,
                        name,
                        plate,
                        static_cast<UserRole>(roleValue),
                        units
                    )
                );
            }
        }

        return true;
    }

    bool loadActiveSessions(std::string* error) {
        std::ifstream input(paths.activeSessionsFile);
        if (!input) {
            return true;
        }

        std::string line;
        while (std::getline(input, line)) {
            removeTrailingCarriageReturn(line);
            if (line.empty()) {
                continue;
            }

            const std::vector<std::string> fields = splitFields(line);
            std::string sessionId;
            std::string userId;
            std::string plate;
            std::string slotIdText;
            std::string slotCode;
            std::string entryTimeText;
            std::string entryTimestamp;

            if (fields.size() >= 7) {
                sessionId = fields[0];
                userId = fields[1];
                plate = fields[2];
                slotIdText = fields[3];
                slotCode = fields[4];
                entryTimeText = fields[5];
                entryTimestamp = fields[6];
            } else if (fields.size() >= 4) {
                // Legacy GUI/TUI format: userId|slotId|licensePlate|entryTimeText
                userId = fields[0];
                slotIdText = fields[1];
                plate = fields[2];
                entryTimestamp = fields[3];
            } else {
                continue;
            }

            int slotId = 0;
            std::time_t entryTime = 0;
            if (!parseInt(slotIdText, slotId)) {
                continue;
            }

            if (!entryTimeText.empty()) {
                if (!parseTime(entryTimeText, entryTime)) {
                    parseTimestampText(entryTimeText, entryTime);
                }
            } else {
                parseTimestampText(entryTimestamp, entryTime);
            }

            ParkingSlot slot;
            if (!parkingMap.getSlot(slotId, slot)) {
                if (error) {
                    *error = "Active session references an unknown slot: " +
                             slotIdText;
                }
                return false;
            }

            if (slotCode.empty()) {
                slotCode = slot.slotCode;
            }

            if (entryTimestamp.empty()) {
                entryTimestamp = formatTime(entryTime);
            }

            if (sessionId.empty()) {
                std::ostringstream generated;
                generated << "LEGACY-S-" << userId << '-'
                          << slotId << '-'
                          << static_cast<long long>(entryTime);
                sessionId = generated.str();
            }

            if (!users.contains(userId)) {
                if (error) {
                    *error = "Active session references an unknown user: " +
                             userId;
                }
                return false;
            }

            if (activeSessions.contains(userId)) {
                if (error) {
                    *error = "Duplicate active session for user: " + userId;
                }
                return false;
            }

            if (!parkingMap.occupySlot(slotId, userId, plate)) {
                if (error) {
                    *error = "Cannot restore occupied slot: " + slotCode;
                }
                return false;
            }

            activeSessions.insert(
                userId,
                ActiveSession(
                    sessionId,
                    userId,
                    plate,
                    slotId,
                    slotCode,
                    entryTime,
                    entryTimestamp
                )
            );
        }

        return true;
    }

    bool loadHistory() {
        std::ifstream input(paths.historyFile);
        if (!input) {
            return true;
        }

        std::string line;
        while (std::getline(input, line)) {
            removeTrailingCarriageReturn(line);
            if (line.empty()) {
                continue;
            }

            const std::vector<std::string> fields = splitFields(line);
            ParkingRecord record;

            if (fields.size() >= 13) {
                int slotId = 0;
                std::time_t eventTime = 0;
                long long duration = 0;
                long long units = 0;
                long long amount = 0;

                if (!parseInt(fields[5], slotId) ||
                    !parseTime(fields[8], eventTime) ||
                    !parseLongLong(fields[10], duration) ||
                    !parseLongLong(fields[11], units) ||
                    !parseLongLong(fields[12], amount)) {
                    continue;
                }

                record.eventId = fields[0];
                record.sessionId = fields[1];
                record.userId = fields[2];
                record.licensePlate = fields[3];
                record.name = fields[4];
                record.slotId = slotId;
                record.slotCode = fields[6];
                record.action = fields[7];
                record.eventTime = eventTime;
                record.timestamp = fields[9];
                record.durationSeconds = duration;
                record.chargedUnits = units;
                record.amount = amount;
            } else if (fields.size() >= 6) {
                // Legacy format: userId|licensePlate|name|slotId|action|timestamp
                int slotId = 0;
                if (!parseInt(fields[3], slotId)) {
                    continue;
                }

                std::time_t eventTime = 0;
                parseTimestampText(fields[5], eventTime);

                ParkingSlot slot;
                std::string slotCode;
                if (parkingMap.getSlot(slotId, slot)) {
                    slotCode = slot.slotCode;
                }

                record.eventId = makeEventId(eventTime > 0 ? eventTime : 0);
                record.sessionId = "";
                record.userId = fields[0];
                record.licensePlate = fields[1];
                record.name = fields[2];
                record.slotId = slotId;
                record.slotCode = slotCode;
                record.action = fields[4];
                record.eventTime = eventTime;
                record.timestamp = fields[5];
                record.durationSeconds = 0;
                record.chargedUnits = 0;
                record.amount = 0;
            } else {
                continue;
            }

            history.insertBack(record);
            ++eventCounter;
        }

        return true;
    }

    bool loadMonthlyUsage() {
        std::ifstream input(paths.monthlyUsageFile);
        if (!input) {
            return true;
        }

        std::string line;
        while (std::getline(input, line)) {
            removeTrailingCarriageReturn(line);
            if (line.empty()) {
                continue;
            }

            std::istringstream stream(line);
            std::string userId;
            std::string yearText;
            std::string monthText;
            std::string unitsText;
            std::string amountText;

            if (!readField(stream, userId) ||
                !readField(stream, yearText) ||
                !readField(stream, monthText) ||
                !readField(stream, unitsText) ||
                !readField(stream, amountText)) {
                continue;
            }

            int year = 0;
            int month = 0;
            long long units = 0;
            long long amount = 0;

            if (!parseInt(yearText, year) ||
                !parseInt(monthText, month) ||
                !parseLongLong(unitsText, units) ||
                !parseLongLong(amountText, amount) ||
                month < 1 || month > 12) {
                continue;
            }

            monthlyUsage.insert(
                monthlyKey(userId, year, month),
                MonthlyUsage(userId, year, month)
            );

            MonthlyUsage* usage = monthlyUsage.find(
                monthlyKey(userId, year, month)
            );

            if (usage) {
                usage->parkingUnits = units;
                usage->amount = amount;
            }
        }

        return true;
    }

    bool loadMonthlyBills() {
        std::ifstream input(paths.monthlyBillsFile);
        if (!input) {
            return true;
        }

        std::string line;
        while (std::getline(input, line)) {
            removeTrailingCarriageReturn(line);
            if (line.empty()) {
                continue;
            }

            std::istringstream stream(line);
            std::string billId;
            std::string userId;
            std::string yearText;
            std::string monthText;
            std::string unitsText;
            std::string amountText;
            std::string createdText;
            std::string dueText;
            std::string paidText;
            std::string paidAtText;

            if (!readField(stream, billId) ||
                !readField(stream, userId) ||
                !readField(stream, yearText) ||
                !readField(stream, monthText) ||
                !readField(stream, unitsText) ||
                !readField(stream, amountText) ||
                !readField(stream, createdText) ||
                !readField(stream, dueText) ||
                !readField(stream, paidText) ||
                !readField(stream, paidAtText)) {
                continue;
            }

            int year = 0;
            int month = 0;
            int paidValue = 0;
            long long units = 0;
            long long amount = 0;
            std::time_t createdAt = 0;
            std::time_t dueDate = 0;
            std::time_t paidAt = 0;

            if (!parseInt(yearText, year) ||
                !parseInt(monthText, month) ||
                !parseLongLong(unitsText, units) ||
                !parseLongLong(amountText, amount) ||
                !parseTime(createdText, createdAt) ||
                !parseTime(dueText, dueDate) ||
                !parseInt(paidText, paidValue) ||
                !parseTime(paidAtText, paidAt)) {
                continue;
            }

            MonthlyBill bill(
                billId,
                userId,
                year,
                month,
                units,
                amount,
                createdAt,
                dueDate
            );

            bill.paid = paidValue != 0;
            bill.paidAt = paidAt;
            monthlyBills.insert(billId, bill);
        }

        return true;
    }

    MonthlyUsage* getOrCreateUsage(
        const std::string& userId,
        int year,
        int month
    ) {
        const std::string key = monthlyKey(userId, year, month);
        MonthlyUsage* usage = monthlyUsage.find(key);

        if (!usage) {
            monthlyUsage.insert(
                key,
                MonthlyUsage(userId, year, month)
            );
            usage = monthlyUsage.find(key);
        }

        return usage;
    }

    bool hasGeneratedBill(
        const std::string& userId,
        int year,
        int month
    ) const {
        return monthlyBills.contains(
            billIdFor(userId, year, month)
        );
    }

    bool restoreExitState(
        const AdminAction& action,
        std::string& error
    ) {
        const ActiveSession& session = action.sessionData;
        const ParkingRecord& record = action.recordData;

        if (activeSessions.contains(session.userId)) {
            error = "Cannot undo exit: user already has an active session.";
            return false;
        }

        if (!parkingMap.isSlotAvailable(session.slotId)) {
            error = "Cannot undo exit: the old slot is no longer available.";
            return false;
        }

        User* user = users.find(session.userId);
        if (!user) {
            error = "Cannot undo exit: user no longer exists.";
            return false;
        }

        const LocalDate date = getLocalDate(record.eventTime);
        if (hasGeneratedBill(user->id, date.year, date.month)) {
            error = "Cannot undo exit after the monthly bill was generated.";
            return false;
        }

        MonthlyUsage* usage = getOrCreateUsage(
            user->id,
            date.year,
            date.month
        );

        if (!usage ||
            usage->parkingUnits < record.chargedUnits ||
            usage->amount < record.amount ||
            user->totalParkingUnits < record.chargedUnits) {
            error = "Cannot undo exit: monthly totals are inconsistent.";
            return false;
        }

        if (!parkingMap.occupySlot(
                session.slotId,
                session.userId,
                session.licensePlate
            )) {
            error = "Cannot restore the previous parking slot.";
            return false;
        }

        activeSessions.insert(session.userId, session);
        user->totalParkingUnits -= record.chargedUnits;
        usage->parkingUnits -= record.chargedUnits;
        usage->amount -= record.amount;

        if (!writeUsers() ||
            !writeActiveSessions() ||
            !writeMonthlyUsage()) {
            activeSessions.remove(session.userId);
            parkingMap.releaseSlot(session.slotId);
            user->totalParkingUnits += record.chargedUnits;
            usage->parkingUnits += record.chargedUnits;
            usage->amount += record.amount;

            writeUsers();
            writeActiveSessions();
            writeMonthlyUsage();

            error = "Cannot save data while undoing exit.";
            return false;
        }

        ParkingRecord undoRecord = record;
        undoRecord.eventId = makeEventId(std::time(nullptr));
        undoRecord.action = parkingActionToString(
            ParkingActionType::UNDO_EXIT
        );
        undoRecord.eventTime = std::time(nullptr);
        undoRecord.timestamp = formatTime(undoRecord.eventTime);

        if (!appendHistory(undoRecord)) {
            activeSessions.remove(session.userId);
            parkingMap.releaseSlot(session.slotId);
            user->totalParkingUnits += record.chargedUnits;
            usage->parkingUnits += record.chargedUnits;
            usage->amount += record.amount;

            writeUsers();
            writeActiveSessions();
            writeMonthlyUsage();

            error = "Cannot write undo history.";
            return false;
        }

        return true;
    }

public:
    explicit ParkingSystem(
        const std::string& dataDirectory = "."
    )
        : paths(dataDirectory),
          parkingMap(),
          users(2003),
          activeSessions(2003),
          monthlyUsage(4093),
          monthlyBills(4093),
          history(),
          undoStack(),
          eventCounter(0),
          sessionCounter(0),
          initialized(false) {}

    ParkingSystem(const ParkingSystem&) = delete;
    ParkingSystem& operator=(const ParkingSystem&) = delete;

    bool initialize(std::string* error = nullptr) {
        if (initialized) {
            return true;
        }

        if (!parkingMap.loadFromFile(paths.mapFile)) {
            if (error) {
                *error = "Cannot load map file: " + paths.mapFile;
            }
            return false;
        }

        if (!loadUsers()) {
            if (error) {
                *error = "Cannot load user file: " + paths.usersFile;
            }
            return false;
        }

        if (!loadHistory() ||
            !loadMonthlyUsage() ||
            !loadMonthlyBills()) {
            if (error) {
                *error = "Cannot load one or more data files.";
            }
            return false;
        }

        if (!loadActiveSessions(error)) {
            return false;
        }

        if (!processMonthlyBilling(std::time(nullptr))) {
            if (error) {
                *error = "Cannot generate monthly bills.";
            }
            return false;
        }

        initialized = true;
        return true;
    }

    bool isInitialized() const {
        return initialized;
    }

    ParkingMap& map() {
        return parkingMap;
    }

    const ParkingMap& map() const {
        return parkingMap;
    }

    User* findUser(const std::string& userId) {
        return users.find(userId);
    }

    const User* findUser(const std::string& userId) const {
        return users.find(userId);
    }

    const ActiveSession* findActiveSession(
        const std::string& userId
    ) const {
        return activeSessions.find(userId);
    }

    bool registerUser(
        const std::string& userId,
        const std::string& fullName,
        const std::string& licensePlate,
        UserRole role,
        std::string& error
    ) {
        if (userId.empty() ||
            fullName.empty() ||
            licensePlate.empty()) {
            error = "ID, full name and license plate cannot be empty.";
            return false;
        }

        if (userId.find('|') != std::string::npos ||
            fullName.find('|') != std::string::npos ||
            licensePlate.find('|') != std::string::npos) {
            error = "Input data cannot contain the '|' character.";
            return false;
        }

        if (users.contains(userId)) {
            error = "User ID already exists.";
            return false;
        }

        if (licensePlateExists(licensePlate)) {
            error = "License plate already exists.";
            return false;
        }

        users.insert(
            userId,
            User(userId, fullName, licensePlate, role, 0)
        );

        if (!writeUsers()) {
            users.remove(userId);
            error = "Cannot save the user list.";
            return false;
        }

        return true;
    }

    bool vehicleEntry(
        const std::string& userId,
        ActiveSession& result,
        std::string& error,
        std::time_t now = std::time(nullptr)
    ) {
        if (!processMonthlyBilling(now)) {
            error = "Cannot update monthly billing data.";
            return false;
        }

        User* user = users.find(userId);
        if (!user) {
            error = "User ID does not exist. Please register first.";
            return false;
        }

        if (activeSessions.contains(userId)) {
            error = "This user already has a vehicle in the parking lot.";
            return false;
        }

        if (isBlacklisted(userId, now)) {
            error = "Parking is denied because the user has an overdue bill.";
            return false;
        }

        ParkingSlot slot;
        if (!parkingMap.findNearestAvailableSlot(user->role, slot)) {
            error = "No suitable parking slot is currently available.";
            return false;
        }

        ActiveSession session(
            makeSessionId(userId, now),
            userId,
            user->licensePlate,
            slot.slotId,
            slot.slotCode,
            now,
            formatTime(now)
        );

        if (!parkingMap.occupySlot(
                slot.slotId,
                userId,
                user->licensePlate
            )) {
            error = "The selected parking slot is no longer available.";
            return false;
        }

        activeSessions.insert(userId, session);

        if (!writeActiveSessions()) {
            activeSessions.remove(userId);
            parkingMap.releaseSlot(slot.slotId);
            error = "Cannot save the active parking session.";
            return false;
        }

        ParkingRecord record(
            makeEventId(now),
            session.sessionId,
            user->id,
            user->licensePlate,
            user->name,
            slot.slotId,
            slot.slotCode,
            ParkingActionType::ENTRY,
            now,
            formatTime(now),
            0,
            0,
            0
        );

        if (!appendHistory(record)) {
            activeSessions.remove(userId);
            parkingMap.releaseSlot(slot.slotId);
            writeActiveSessions();
            error = "Cannot write entry history.";
            return false;
        }

        AdminAction action;
        action.type = AdminActionType::VEHICLE_ENTRY;
        action.description = "Vehicle entry: " + userId;
        action.sessionData = session;
        action.recordData = record;
        undoStack.push(action);

        result = session;
        return true;
    }

    bool previewVehicleExit(
        const std::string& userId,
        ActiveSession& result,
        std::string& error
    ) const {
        const ActiveSession* session = activeSessions.find(userId);
        if (!session) {
            error = "No active parking session was found for this ID.";
            return false;
        }

        result = *session;
        return true;
    }

    bool vehicleExit(
        const std::string& userId,
        const std::string& confirmationId,
        ParkingRecord& result,
        std::string& error,
        std::time_t now = std::time(nullptr)
    ) {
        if (userId != confirmationId) {
            error = "The confirmation ID does not match.";
            return false;
        }

        ActiveSession* sessionPointer = activeSessions.find(userId);
        if (!sessionPointer) {
            error = "No active parking session was found for this ID.";
            return false;
        }

        User* user = users.find(userId);
        if (!user) {
            error = "The user record is missing.";
            return false;
        }

        const ActiveSession session = *sessionPointer;
        long long duration = static_cast<long long>(
            now - session.entryTime
        );

        if (duration < 0) {
            duration = 0;
        }

        const long long units = calculateParkingUnits(duration);
        const long long amount = units * feeForRole(user->role);
        const LocalDate date = getLocalDate(now);

        MonthlyUsage* usage = getOrCreateUsage(
            userId,
            date.year,
            date.month
        );

        if (!usage) {
            error = "Cannot create monthly usage data.";
            return false;
        }

        const long long oldUserUnits = user->totalParkingUnits;
        const long long oldUsageUnits = usage->parkingUnits;
        const long long oldUsageAmount = usage->amount;

        if (!parkingMap.releaseSlot(session.slotId)) {
            error = "Cannot release the occupied parking slot.";
            return false;
        }

        activeSessions.remove(userId);
        user->totalParkingUnits += units;
        usage->parkingUnits += units;
        usage->amount += amount;

        if (!writeUsers() ||
            !writeActiveSessions() ||
            !writeMonthlyUsage()) {
            activeSessions.insert(userId, session);
            parkingMap.occupySlot(
                session.slotId,
                session.userId,
                session.licensePlate
            );
            user->totalParkingUnits = oldUserUnits;
            usage->parkingUnits = oldUsageUnits;
            usage->amount = oldUsageAmount;

            writeUsers();
            writeActiveSessions();
            writeMonthlyUsage();

            error = "Cannot save exit data.";
            return false;
        }

        ParkingRecord record(
            makeEventId(now),
            session.sessionId,
            user->id,
            user->licensePlate,
            user->name,
            session.slotId,
            session.slotCode,
            ParkingActionType::EXIT,
            now,
            formatTime(now),
            duration,
            units,
            amount
        );

        if (!appendHistory(record)) {
            activeSessions.insert(userId, session);
            parkingMap.occupySlot(
                session.slotId,
                session.userId,
                session.licensePlate
            );
            user->totalParkingUnits = oldUserUnits;
            usage->parkingUnits = oldUsageUnits;
            usage->amount = oldUsageAmount;

            writeUsers();
            writeActiveSessions();
            writeMonthlyUsage();

            error = "Cannot write exit history.";
            return false;
        }

        AdminAction action;
        action.type = AdminActionType::VEHICLE_EXIT;
        action.description = "Vehicle exit: " + userId;
        action.sessionData = session;
        action.recordData = record;
        action.previousParkingUnits = oldUserUnits;
        action.previousAmount = oldUsageAmount;
        undoStack.push(action);

        result = record;
        return true;
    }

    bool undoLast(std::string& error) {
        if (undoStack.empty()) {
            error = "There is no action to undo.";
            return false;
        }

        const AdminAction action = undoStack.top();

        if (action.type == AdminActionType::VEHICLE_ENTRY) {
            const ActiveSession* current = activeSessions.find(
                action.sessionData.userId
            );

            if (!current ||
                current->sessionId != action.sessionData.sessionId) {
                error = "Cannot undo entry because the active session changed.";
                return false;
            }

            activeSessions.remove(action.sessionData.userId);

            if (!parkingMap.releaseSlot(action.sessionData.slotId)) {
                activeSessions.insert(
                    action.sessionData.userId,
                    action.sessionData
                );
                error = "Cannot release the slot while undoing entry.";
                return false;
            }

            if (!writeActiveSessions()) {
                activeSessions.insert(
                    action.sessionData.userId,
                    action.sessionData
                );
                parkingMap.occupySlot(
                    action.sessionData.slotId,
                    action.sessionData.userId,
                    action.sessionData.licensePlate
                );
                error = "Cannot save data while undoing entry.";
                return false;
            }

            ParkingRecord undoRecord = action.recordData;
            undoRecord.eventId = makeEventId(std::time(nullptr));
            undoRecord.action = parkingActionToString(
                ParkingActionType::UNDO_ENTRY
            );
            undoRecord.eventTime = std::time(nullptr);
            undoRecord.timestamp = formatTime(undoRecord.eventTime);

            if (!appendHistory(undoRecord)) {
                activeSessions.insert(
                    action.sessionData.userId,
                    action.sessionData
                );
                parkingMap.occupySlot(
                    action.sessionData.slotId,
                    action.sessionData.userId,
                    action.sessionData.licensePlate
                );
                writeActiveSessions();
                error = "Cannot write undo history.";
                return false;
            }

            undoStack.pop();
            return true;
        }

        if (action.type == AdminActionType::VEHICLE_EXIT) {
            if (!restoreExitState(action, error)) {
                return false;
            }

            undoStack.pop();
            return true;
        }

        error = "The last action type is not supported by undo.";
        return false;
    }

    bool processMonthlyBilling(
        std::time_t now = std::time(nullptr)
    ) {
        const LocalDate current = getLocalDate(now);
        bool changed = false;

        monthlyUsage.forEach(
            [&](const std::string&, const MonthlyUsage& usage) {
                if (usage.parkingUnits <= 0 ||
                    !isEarlierMonth(
                        usage.year,
                        usage.month,
                        current.year,
                        current.month
                    )) {
                    return;
                }

                const std::string billId = billIdFor(
                    usage.userId,
                    usage.year,
                    usage.month
                );

                if (monthlyBills.contains(billId)) {
                    return;
                }

                const std::time_t createdAt = makeLocalTime(
                    usage.year,
                    usage.month + 1,
                    1
                );

                const std::time_t dueDate =
                    createdAt +
                    static_cast<std::time_t>(
                        PAYMENT_GRACE_DAYS * 86400LL
                    );

                monthlyBills.insert(
                    billId,
                    MonthlyBill(
                        billId,
                        usage.userId,
                        usage.year,
                        usage.month,
                        usage.parkingUnits,
                        usage.amount,
                        createdAt,
                        dueDate
                    )
                );

                changed = true;
            }
        );

        return !changed || writeMonthlyBills();
    }

    bool isBlacklisted(
        const std::string& userId,
        std::time_t now = std::time(nullptr)
    ) const {
        bool blacklisted = false;

        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (!blacklisted &&
                    bill.userId == userId &&
                    bill.isOverdue(now)) {
                    blacklisted = true;
                }
            }
        );

        return blacklisted;
    }

    long long outstandingAmount(
        const std::string& userId,
        std::time_t now = std::time(nullptr)
    ) const {
        long long total = 0;

        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (bill.userId == userId && !bill.paid) {
                    total += bill.totalDue(now, LATE_FEE_PER_DAY);
                }
            }
        );

        return total;
    }

    bool payBill(
        const std::string& billId,
        long long& paidAmount,
        std::string& error,
        std::time_t now = std::time(nullptr)
    ) {
        MonthlyBill* bill = monthlyBills.find(billId);
        if (!bill) {
            error = "Bill ID does not exist.";
            return false;
        }

        if (bill->paid) {
            error = "This bill has already been paid.";
            return false;
        }

        User* user = users.find(bill->userId);
        if (!user) {
            error = "The bill references an unknown user.";
            return false;
        }

        paidAmount = bill->totalDue(now, LATE_FEE_PER_DAY);
        bill->paid = true;
        bill->paidAt = now;

        if (!writeMonthlyBills()) {
            bill->paid = false;
            bill->paidAt = 0;
            error = "Cannot save payment information.";
            return false;
        }

        ParkingRecord record(
            makeEventId(now),
            "",
            user->id,
            user->licensePlate,
            user->name,
            0,
            "",
            ParkingActionType::PAYMENT,
            now,
            formatTime(now),
            0,
            0,
            paidAmount
        );

        if (!appendHistory(record)) {
            bill->paid = false;
            bill->paidAt = 0;
            writeMonthlyBills();
            error = "Cannot write payment history.";
            return false;
        }

        return true;
    }

    bool saveAll() const {
        return writeUsers() &&
               writeActiveSessions() &&
               writeMonthlyUsage() &&
               writeMonthlyBills();
    }

    std::size_t userCount() const {
        return users.size();
    }

    std::size_t activeVehicleCount() const {
        return activeSessions.size();
    }

    std::size_t historyCount() const {
        return history.size();
    }

    void forEachUser(
        const std::function<void(const User&)>& function
    ) const {
        users.forEach(
            [&](const std::string&, const User& user) {
                function(user);
            }
        );
    }

    void forEachActiveSession(
        const std::function<void(const ActiveSession&)>& function
    ) const {
        activeSessions.forEach(
            [&](const std::string&, const ActiveSession& session) {
                function(session);
            }
        );
    }

    void forEachPersonalHistory(
        const std::string& userId,
        const std::function<void(const ParkingRecord&)>& function
    ) const {
        history.forEach(
            [&](const ParkingRecord& record) {
                if (record.userId == userId) {
                    function(record);
                }
            }
        );
    }

    void forEachBill(
        const std::string& userId,
        const std::function<void(
            const MonthlyBill&,
            long long,
            long long
        )>& function,
        std::time_t now = std::time(nullptr)
    ) const {
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (bill.userId == userId) {
                    function(
                        bill,
                        bill.penalty(now, LATE_FEE_PER_DAY),
                        bill.totalDue(now, LATE_FEE_PER_DAY)
                    );
                }
            }
        );
    }

    void forEachBlacklisted(
        const std::function<void(
            const User&,
            const MonthlyBill&,
            long long,
            long long
        )>& function,
        std::time_t now = std::time(nullptr)
    ) const {
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (!bill.isOverdue(now)) {
                    return;
                }

                const User* user = users.find(bill.userId);
                if (!user) {
                    return;
                }

                function(
                    *user,
                    bill,
                    bill.penalty(now, LATE_FEE_PER_DAY),
                    bill.totalDue(now, LATE_FEE_PER_DAY)
                );
            }
        );
    }

    void forEachMonthlyUsage(
        int year,
        int month,
        const std::function<void(
            const User&,
            const MonthlyUsage&
        )>& function
    ) const {
        monthlyUsage.forEach(
            [&](const std::string&, const MonthlyUsage& usage) {
                if (usage.year != year || usage.month != month) {
                    return;
                }

                const User* user = users.find(usage.userId);
                if (user) {
                    function(*user, usage);
                }
            }
        );
    }

    bool loadMap(const std::string& fileName) {
        ParkingMap candidate;
        if (!candidate.loadFromFile(fileName)) {
            return false;
        }

        bool restored = true;
        activeSessions.forEach(
            [&](const std::string&, const ActiveSession& session) {
                if (!candidate.occupySlot(
                        session.slotId,
                        session.userId,
                        session.licensePlate
                    )) {
                    restored = false;
                }
            }
        );

        if (!restored) {
            return false;
        }

        parkingMap = candidate;
        return true;
    }

    bool mapLoaded() const {
        return parkingMap.isLoaded();
    }

    std::vector<std::string> getMapLines() const {
        return parkingMap.getMapLines();
    }

    bool addUser(
        const std::string& userId,
        const std::string& fullName,
        const std::string& licensePlate,
        UserRole role
    ) {
        std::string error;
        return registerUser(userId, fullName, licensePlate, role, error);
    }

    bool removeUser(const std::string& userId, std::string& error) {
        if (activeSessions.contains(userId)) {
            error = "Cannot remove a user who currently has a vehicle parked.";
            return false;
        }

        bool hasBill = false;
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (bill.userId == userId && !bill.paid) {
                    hasBill = true;
                }
            }
        );

        if (hasBill) {
            error = "Cannot remove a user who still has unpaid bills.";
            return false;
        }

        User* user = users.find(userId);
        if (!user) {
            error = "User ID does not exist.";
            return false;
        }

        const User oldUser = *user;
        users.remove(userId);

        if (!writeUsers()) {
            users.insert(userId, oldUser);
            error = "Cannot save the user list.";
            return false;
        }

        return true;
    }

    bool removeUser(const std::string& userId) {
        std::string error;
        return removeUser(userId, error);
    }

    const User* getUser(const std::string& userId) const {
        return findUser(userId);
    }

    std::pair<bool, std::string> vehicleEntry(
        const std::string& userId
    ) {
        ActiveSession session;
        std::string error;
        if (!vehicleEntry(userId, session, error)) {
            return {false, error};
        }

        std::ostringstream message;
        message << "Xe vào thành công: " << session.userId
                << " | " << session.licensePlate
                << " | ô " << session.slotCode
                << " | " << session.entryTimestamp;
        return {true, message.str()};
    }

    std::pair<bool, std::string> vehicleExit(
        const std::string& userId
    ) {
        ParkingRecord record;
        std::string error;
        if (!vehicleExit(userId, userId, record, error)) {
            return {false, error};
        }

        std::ostringstream message;
        message << "Xe ra thành công: " << record.userId
                << " | ô " << record.slotCode
                << " | lượt " << record.chargedUnits
                << " | tiền " << record.amount << " VND";
        return {true, message.str()};
    }

    bool undoLastAction(std::string& error) {
        return undoLast(error);
    }

    std::string doUndo() {
        std::string error;
        if (!undoLast(error)) {
            return error;
        }

        return "Hoàn tác thành công.";
    }

    std::size_t undoCount() const {
        return undoStack.size();
    }

    void forEachHistory(
        const std::function<void(const ParkingRecord&)>& function
    ) const {
        history.forEach(function);
    }

    void forEachSlot(
        const std::function<void(const ParkingSlot&)>& function
    ) const {
        parkingMap.forEachSlot(function);
    }

    void forEachAnyBill(
        const std::function<void(
            const MonthlyBill&,
            const User*,
            long long,
            long long
        )>& function,
        std::time_t now = std::time(nullptr)
    ) const {
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                function(
                    bill,
                    users.find(bill.userId),
                    bill.penalty(now, LATE_FEE_PER_DAY),
                    bill.totalDue(now, LATE_FEE_PER_DAY)
                );
            }
        );
    }

    long long recordedAmount() const {
        long long total = 0;
        monthlyUsage.forEach(
            [&](const std::string&, const MonthlyUsage& usage) {
                total += usage.amount;
            }
        );
        return total;
    }

    long long recordedUnits() const {
        long long total = 0;
        monthlyUsage.forEach(
            [&](const std::string&, const MonthlyUsage& usage) {
                total += usage.parkingUnits;
            }
        );
        return total;
    }

    long long overdueAmount(std::time_t now = std::time(nullptr)) const {
        long long total = 0;
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (bill.isOverdue(now)) {
                    total += bill.totalDue(now, LATE_FEE_PER_DAY);
                }
            }
        );
        return total;
    }

    std::size_t overdueBillCount(
        std::time_t now = std::time(nullptr)
    ) const {
        std::size_t count = 0;
        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (bill.isOverdue(now)) {
                    ++count;
                }
            }
        );
        return count;
    }

    struct Stats {
        int total;
        int occ;
        int users;
        int hist;
        long long revenue;
        bool mapLoaded;
    };

    Stats getStats() const {
        return Stats{
            static_cast<int>(parkingMap.totalSlots()),
            static_cast<int>(activeSessions.size()),
            static_cast<int>(users.size()),
            static_cast<int>(history.size()),
            recordedAmount(),
            parkingMap.isLoaded()
        };
    }

    std::vector<std::array<std::string, 5>> getUserList() const {
        std::vector<std::array<std::string, 5>> result;
        users.forEach(
            [&](const std::string&, const User& user) {
                result.push_back({
                    user.id,
                    user.name,
                    user.licensePlate,
                    roleToString(user.role),
                    std::to_string(user.totalParkingUnits)
                });
            }
        );
        return result;
    }

    std::vector<std::array<std::string, 6>> getActiveSessionList() const {
        std::vector<std::array<std::string, 6>> result;
        activeSessions.forEach(
            [&](const std::string&, const ActiveSession& session) {
                result.push_back({
                    session.sessionId,
                    session.userId,
                    session.licensePlate,
                    std::to_string(session.slotId),
                    session.slotCode,
                    session.entryTimestamp
                });
            }
        );
        return result;
    }

    std::vector<std::array<std::string, 6>> getSlotList() const {
        std::vector<std::array<std::string, 6>> result;
        parkingMap.forEachSlot(
            [&](const ParkingSlot& slot) {
                result.push_back({
                    std::to_string(slot.slotId),
                    slot.slotCode,
                    slot.status == SlotStatus::OCCUPIED ? "1" : "0",
                    slot.licensePlate,
                    slot.userId,
                    std::string(1, slot.baseType)
                });
            }
        );
        return result;
    }

    std::vector<std::array<std::string, 8>> getHistoryList(
        int limit = 200
    ) const {
        std::vector<std::array<std::string, 8>> result;
        const int total = static_cast<int>(history.size());
        int start = 0;
        if (limit > 0 && total > limit) {
            start = total - limit;
        }

        for (int index = start; index < total; ++index) {
            const ParkingRecord& record = history.get(
                static_cast<std::size_t>(index)
            );
            result.push_back({
                record.timestamp,
                record.action,
                record.userId,
                record.name,
                record.licensePlate,
                record.slotCode.empty() ? std::to_string(record.slotId) : record.slotCode,
                std::to_string(record.chargedUnits),
                std::to_string(record.amount)
            });
        }
        return result;
    }

    std::string searchByPlate(const std::string& licensePlate) const {
        std::string result;
        activeSessions.forEach(
            [&](const std::string&, const ActiveSession& session) {
                if (result.empty() && session.licensePlate == licensePlate) {
                    result = "ID: " + session.userId +
                             " | Ô: " + session.slotCode +
                             " | Vào: " + session.entryTimestamp;
                }
            }
        );

        if (result.empty()) {
            return "Không tìm thấy biển số đang gửi: " + licensePlate;
        }
        return result;
    }

    std::vector<std::array<std::string, 7>> getUserBills(
        const std::string& userId
    ) const {
        std::vector<std::array<std::string, 7>> result;
        forEachBill(
            userId,
            [&](const MonthlyBill& bill, long long penalty, long long total) {
                std::ostringstream monthYear;
                monthYear << bill.month << '/' << bill.year;
                result.push_back({
                    bill.billId,
                    monthYear.str(),
                    std::to_string(bill.parkingUnits),
                    std::to_string(bill.baseAmount),
                    std::to_string(penalty),
                    std::to_string(total),
                    bill.paid ? "Đã thanh toán" : "Chưa thanh toán"
                });
            }
        );
        return result;
    }

    std::vector<std::array<std::string, 5>> getBlacklist() const {
        std::vector<std::array<std::string, 5>> result;
        std::vector<std::string> processed;
        const std::time_t now = std::time(nullptr);

        monthlyBills.forEach(
            [&](const std::string&, const MonthlyBill& bill) {
                if (!bill.isOverdue(now)) {
                    return;
                }

                for (const std::string& userId : processed) {
                    if (userId == bill.userId) {
                        return;
                    }
                }
                processed.push_back(bill.userId);

                int billCount = 0;
                long long penalty = 0;
                long long total = 0;
                monthlyBills.forEach(
                    [&](const std::string&, const MonthlyBill& item) {
                        if (item.userId == bill.userId && item.isOverdue(now)) {
                            ++billCount;
                            penalty += item.penalty(now, LATE_FEE_PER_DAY);
                            total += item.totalDue(now, LATE_FEE_PER_DAY);
                        }
                    }
                );

                const User* user = users.find(bill.userId);
                result.push_back({
                    bill.userId,
                    user ? user->name : "",
                    std::to_string(billCount),
                    std::to_string(penalty),
                    std::to_string(total)
                });
            }
        );

        return result;
    }

    std::vector<std::array<std::string, 5>> getMonthlyUsageList(
        int year,
        int month
    ) const {
        std::vector<std::array<std::string, 5>> result;
        forEachMonthlyUsage(
            year,
            month,
            [&](const User& user, const MonthlyUsage& usage) {
                result.push_back({
                    usage.userId,
                    user.name,
                    roleToString(user.role),
                    std::to_string(usage.parkingUnits),
                    std::to_string(usage.amount)
                });
            }
        );
        return result;
    }

    std::string getRevenueText() const {
        std::ostringstream output;
        output << "Doanh thu được suy ra từ monthly_usage.txt\n"
               << "Tổng lượt đã tính: " << recordedUnits() << '\n'
               << "Tổng tiền ghi nhận: " << recordedAmount() << " VND";
        return output.str();
    }

    long long getRevenue() const {
        return recordedAmount();
    }

};

#endif // PARKINGSYSTEM_TUI_HPP
