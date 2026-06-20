#include "ParkingSystem.hpp"

#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

const char* kMap =
    "6 16\n"
    "################\n"
    "#E.............#\n"
    "#..PPP...TT....#\n"
    "#..............#\n"
    "#..PPP...TT....#\n"
    "################\n";

std::time_t makeTime(int year, int month, int day, int hour = 0) {
    std::tm value{};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_isdst = -1;
    return std::mktime(&value);
}

void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    assert(output && "cannot create test file");
    output << content;
}

fs::path makeCaseDir(const fs::path& root, const std::string& name) {
    fs::path dir = root / name;
    fs::remove_all(dir);
    fs::create_directories(dir);
    writeFile(dir / "map.txt", kMap);
    writeFile(dir / "history.txt", "");
    writeFile(dir / "active_sessions.txt", "");
    writeFile(dir / "monthly_usage.txt", "");
    writeFile(dir / "monthly_bills.txt", "");
    writeFile(dir / "revenue.txt", "");
    return dir;
}

void testUsers(const fs::path& root) {
    fs::path dir = makeCaseDir(root, "users");
    writeFile(
        dir / "DanhSachSV.txt",
        "OLD1|Nguyen Van A|AA-001|0\n"
        "NEW1|Tran Thi B|BB-001|1|7\n"
        "BAD1|Role Loi|CC-001|9|3\n"
    );

    ParkingSystem system(dir.string());
    std::string error;
    assert(system.initialize(&error));
    assert(system.findUser("OLD1") != nullptr);
    assert(system.findUser("OLD1")->totalParkingUnits == 0);
    assert(system.findUser("NEW1") != nullptr);
    assert(system.findUser("NEW1")->totalParkingUnits == 7);
    assert(system.findUser("BAD1") == nullptr);

    assert(!system.registerUser("OLD1", "Duplicate", "DD-001", UserRole::STUDENT, error));
    assert(!system.registerUser("NEW2", "Duplicate plate", "AA-001", UserRole::STUDENT, error));
    assert(system.registerUser("NEW2", "Le Van C", "DD-001", UserRole::GUEST, error));
}

void testEntryExitAndFees(const fs::path& root) {
    fs::path dir = makeCaseDir(root, "entry_exit");
    writeFile(
        dir / "DanhSachSV.txt",
        "S1|Sinh Vien|SV-001|0|0\n"
        "T1|Giang Vien|GV-001|1|0\n"
        "G1|Khach|KH-001|2|0\n"
    );

    ParkingSystem system(dir.string());
    std::string error;
    assert(system.initialize(&error));

    const std::time_t start = makeTime(2026, 6, 10, 8);
    ActiveSession s1;
    ActiveSession t1;
    ActiveSession g1;
    assert(system.vehicleEntry("S1", s1, error, start));
    assert(system.vehicleEntry("T1", t1, error, start));
    assert(system.vehicleEntry("G1", g1, error, start));

    ParkingSlot slot;
    assert(system.map().getSlot(s1.slotId, slot));
    assert(slot.baseType == 'P');
    assert(system.map().getSlot(t1.slotId, slot));
    assert(slot.baseType == 'T');
    assert(system.map().getSlot(g1.slotId, slot));
    assert(slot.baseType == 'P');

    ActiveSession duplicate;
    assert(!system.vehicleEntry("S1", duplicate, error, start));

    ParkingRecord record;
    assert(!system.vehicleExit("S1", "WRONG", record, error, start + 1));
    assert(system.vehicleExit("S1", "S1", record, error, start + 36000));
    assert(record.chargedUnits == 1);
    assert(record.amount == 3000);
    assert(system.map().getSlot(record.slotId, slot));
    assert(slot.status == SlotStatus::EMPTY);

    assert(system.vehicleExit("G1", "G1", record, error, start + 36001));
    assert(record.chargedUnits == 2);
    assert(record.amount == 10000);

    assert(system.vehicleExit("T1", "T1", record, error, start + 72001));
    assert(record.chargedUnits == 3);
    assert(record.amount == 15000);
}


void testNoSuitableSlot(const fs::path& root) {
    fs::path dir = root / "no_slot";
    fs::remove_all(dir);
    fs::create_directories(dir);
    writeFile(
        dir / "map.txt",
        "3 5\n"
        "#####\n"
        "#EP.#\n"
        "#####\n"
    );
    writeFile(
        dir / "DanhSachSV.txt",
        "S1|Mot O|S-001|0|0\n"
        "S2|Het O|S-002|0|0\n"
        "T1|Khong Co T|T-001|1|0\n"
    );
    writeFile(dir / "history.txt", "");
    writeFile(dir / "active_sessions.txt", "");
    writeFile(dir / "monthly_usage.txt", "");
    writeFile(dir / "monthly_bills.txt", "");
    writeFile(dir / "revenue.txt", "");

    ParkingSystem system(dir.string());
    std::string error;
    assert(system.initialize(&error));

    ActiveSession session;
    assert(system.vehicleEntry("S1", session, error, makeTime(2026, 6, 10, 8)));
    assert(!system.vehicleEntry("S2", session, error, makeTime(2026, 6, 10, 9)));
    assert(!system.vehicleEntry("T1", session, error, makeTime(2026, 6, 10, 9)));
}

void testBlacklistBillsAndPayment(const fs::path& root) {
    fs::path dir = makeCaseDir(root, "billing");
    const std::time_t now = makeTime(2026, 7, 10, 8);
    writeFile(
        dir / "DanhSachSV.txt",
        "B1|Bi No|BN-001|0|0\n"
        "B2|Dang Do|DD-001|0|0\n"
    );
    writeFile(
        dir / "monthly_usage.txt",
        "B1|2026|5|2|6000\n"
        "B2|2026|5|1|3000\n"
    );
    writeFile(
        dir / "active_sessions.txt",
        "LEGACY-B2|B2|DD-001|1|A101|1781830800|2026-06-19T08:00:00\n"
    );

    ParkingSystem system(dir.string());
    std::string error;
    assert(system.initialize(&error));
    assert(system.processMonthlyBilling(now));
    assert(system.isBlacklisted("B1", now));
    assert(system.isBlacklisted("B2", now));

    ActiveSession session;
    assert(!system.vehicleEntry("B1", session, error, now));

    ParkingRecord record;
    assert(system.vehicleExit("B2", "B2", record, error, now));

    long long paid = 0;
    assert(system.payBill("BILL-B1-2026-05", paid, error, now));
    assert(paid > 6000);
    assert(!system.isBlacklisted("B1", now));
}

void testUndo(const fs::path& root) {
    fs::path dir = makeCaseDir(root, "undo");
    writeFile(
        dir / "DanhSachSV.txt",
        "U1|Undo Mot|U-001|0|0\n"
        "U2|Undo Hai|U-002|0|0\n"
    );

    ParkingSystem system(dir.string());
    std::string error;
    assert(system.initialize(&error));

    const std::time_t start = makeTime(2026, 6, 12, 8);
    ActiveSession session;
    assert(system.vehicleEntry("U1", session, error, start));
    assert(system.undoLast(error));
    assert(system.findActiveSession("U1") == nullptr);

    assert(system.vehicleEntry("U1", session, error, start));
    ParkingRecord record;
    assert(system.vehicleExit("U1", "U1", record, error, start + 100));
    assert(system.undoLast(error));
    assert(system.findActiveSession("U1") != nullptr);

    assert(system.vehicleExit("U1", "U1", record, error, start + 200));
    assert(system.map().occupySlot(record.slotId, "U2", "U-002"));
    assert(!system.undoLast(error));

    fs::path emptyDir = makeCaseDir(root, "undo_empty");
    writeFile(emptyDir / "DanhSachSV.txt", "E1|Empty|E-001|0|0\n");
    ParkingSystem empty(emptyDir.string());
    assert(empty.initialize(&error));
    assert(!empty.undoLast(error));
}

void testPersistence(const fs::path& root) {
    fs::path dir = makeCaseDir(root, "persistence");
    writeFile(dir / "DanhSachSV.txt", "P1|Persist|P-001|0|0\n");

    std::string error;
    {
        ParkingSystem system(dir.string());
        assert(system.initialize(&error));
        ActiveSession session;
        assert(system.vehicleEntry("P1", session, error, makeTime(2026, 6, 13, 8)));
        assert(system.saveAll());
    }

    ParkingSystem reloaded(dir.string());
    assert(reloaded.initialize(&error));
    assert(reloaded.findActiveSession("P1") != nullptr);
}

} // namespace

int main() {
    fs::path root = fs::temp_directory_path() / "parking_backend_tests";
    fs::remove_all(root);
    fs::create_directories(root);

    testUsers(root);
    testEntryExitAndFees(root);
    testNoSuitableSlot(root);
    testBlacklistBillsAndPayment(root);
    testUndo(root);
    testPersistence(root);

    std::cout << "backend tests passed\n";
    return 0;
}
