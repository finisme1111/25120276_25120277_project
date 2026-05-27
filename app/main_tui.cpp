#include <iostream>
#include <iomanip>
#include <limits>
#include <string>
#include "ParkingSystem.hpp"

using namespace std;

void clearInput() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void pauseScreen() {
    cout << "\nNhan Enter de tiep tuc...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string inputLine(const string& label) {
    cout << label;
    string s;
    getline(cin, s);
    return s;
}

int inputInt(const string& label) {
    cout << label;
    int x;
    cin >> x;
    clearInput();
    return x;
}

UserRole inputRole() {
    cout << "Loai nguoi dung:\n";
    cout << "0. Sinh vien\n";
    cout << "1. Giang vien\n";
    cout << "2. Khach\n";

    int r = inputInt("Chon: ");

    if (r == 1) return UserRole::LECTURER;
    if (r == 2) return UserRole::GUEST;
    return UserRole::STUDENT;
}

void showDashboard(ParkingSystem& system) {
    auto st = system.getStats();

    cout << "\n===== DASHBOARD =====\n";
    cout << "Tong so o       : " << st.total << "\n";
    cout << "Dang co xe      : " << st.occ << "\n";
    cout << "Con trong       : " << st.total - st.occ << "\n";
    cout << "So nguoi dung   : " << st.users << "\n";
    cout << "So lich su      : " << st.hist << "\n";
    cout << "Doanh thu       : " << st.revenue << " VND\n";
    cout << "Ban do          : " << (st.mapLoaded ? "Da load" : "Chua load") << "\n";
}

void loadMap(ParkingSystem& system) {
    string file = inputLine("Nhap file ban do, mac dinh map.txt: ");
    if (file.empty()) file = "map.txt";

    if (system.loadMap(file)) {
        cout << "Load ban do thanh cong.\n";
    } else {
        cout << "Khong load duoc file ban do.\n";
    }
}

void showMap(ParkingSystem& system) {
    cout << "\n===== BAN DO BAI XE =====\n";

    auto lines = system.getMapLines();

    if (lines.empty()) {
        cout << "Chua co ban do. Hay load map.txt truoc.\n";
        return;
    }

    for (const auto& line : lines) {
        cout << line << "\n";
    }

    cout << "\nKy hieu:\n";
    cout << "#: tuong\n";
    cout << ".: duong\n";
    cout << "E: cong vao\n";
    cout << "P: o sinh vien trong\n";
    cout << "T: o giang vien trong\n";
    cout << "X/G: o da co xe\n";
}

void addUser(ParkingSystem& system) {
    cout << "\n===== THEM NGUOI DUNG =====\n";

    string id = inputLine("Ma so: ");
    string name = inputLine("Ho ten: ");
    string plate = inputLine("Bien so xe: ");
    UserRole role = inputRole();

    if (system.addUser(id, name, plate, role)) {
        cout << "Them nguoi dung thanh cong.\n";
    } else {
        cout << "Ma so da ton tai.\n";
    }
}

void removeUser(ParkingSystem& system) {
    cout << "\n===== XOA NGUOI DUNG =====\n";

    string id = inputLine("Nhap ma so can xoa: ");

    if (system.removeUser(id)) {
        cout << "Xoa thanh cong.\n";
    } else {
        cout << "Khong tim thay nguoi dung.\n";
    }
}

void findUser(ParkingSystem& system) {
    cout << "\n===== TIM NGUOI DUNG =====\n";

    string id = inputLine("Nhap ma so: ");
    const User* u = system.getUser(id);

    if (!u) {
        cout << "Khong tim thay.\n";
        return;
    }

    cout << "Ma so       : " << u->id << "\n";
    cout << "Ho ten      : " << u->name << "\n";
    cout << "Bien so     : " << u->licensePlate << "\n";
    cout << "Loai        : " << roleToString(u->role) << "\n";
}

void listUsers(ParkingSystem& system) {
    cout << "\n===== DANH SACH NGUOI DUNG =====\n";

    auto users = system.getUserList();

    cout << left << setw(15) << "Ma so"
         << setw(25) << "Ho ten"
         << setw(15) << "Bien so"
         << setw(15) << "Loai" << "\n";

    cout << string(70, '-') << "\n";

    for (const auto& u : users) {
        cout << left << setw(15) << u[0]
             << setw(25) << u[1]
             << setw(15) << u[2]
             << setw(15) << u[3] << "\n";
    }
}

void vehicleEntry(ParkingSystem& system) {
    cout << "\n===== XE VAO =====\n";

    string id = inputLine("Nhap ma so: ");
    auto res = system.vehicleEntry(id);

    cout << res.second << "\n";
}

void vehicleExit(ParkingSystem& system) {
    cout << "\n===== XE RA =====\n";

    string id = inputLine("Nhap ma so: ");
    auto res = system.vehicleExit(id);

    cout << res.second << "\n";
}

void listSlots(ParkingSystem& system) {
    cout << "\n===== TRANG THAI O DO =====\n";

    auto slots = system.getSlotList();

    cout << left << setw(10) << "O"
         << setw(15) << "Trang thai"
         << setw(15) << "Bien so"
         << setw(15) << "Ma so" << "\n";

    cout << string(55, '-') << "\n";

    for (const auto& s : slots) {
        string status = (s[1] == "0") ? "Trong" : "Co xe";

        cout << left << setw(10) << s[0]
             << setw(15) << status
             << setw(15) << s[2]
             << setw(15) << s[3] << "\n";
    }
}

void searchByPlate(ParkingSystem& system) {
    cout << "\n===== TIM THEO BIEN SO =====\n";

    string plate = inputLine("Nhap bien so: ");
    cout << system.searchByPlate(plate) << "\n";
}

void showHistory(ParkingSystem& system) {
    cout << "\n===== LICH SU VAO / RA =====\n";

    int n = inputInt("Nhap so dong lich su muon xem: ");
    auto hist = system.getHistoryList(n);

    cout << left << setw(22) << "Thoi gian"
         << setw(8) << "Loai"
         << setw(25) << "Ten"
         << setw(15) << "Bien so"
         << setw(8) << "O" << "\n";

    cout << string(80, '-') << "\n";

    for (const auto& h : hist) {
        cout << left << setw(22) << h[0]
             << setw(8) << h[1]
             << setw(25) << h[2]
             << setw(15) << h[3]
             << setw(8) << h[4] << "\n";
    }
}

void showRevenue(ParkingSystem& system) {
    cout << "\n===== DOANH THU =====\n";
    cout << system.getRevenueText() << "\n";
}

void showUndoList(ParkingSystem& system) {
    cout << "\n===== UNDO LIST =====\n";

    auto undo = system.getUndoList();

    if (undo.empty()) {
        cout << "Khong co thao tac nao de undo.\n";
        return;
    }

    for (int i = 0; i < (int)undo.size(); i++) {
        cout << i + 1 << ". " << undo[i] << "\n";
    }
}

void undoAction(ParkingSystem& system) {
    cout << "\n===== UNDO =====\n";
    cout << system.doUndo() << '\n';
}

void menu() {
    cout << "\n========== PARKING SYSTEM TUI ==========\n";
    cout << "1. Dashboard\n";
    cout << "2. Load ban do\n";
    cout << "3. Xem ban do\n";
    cout << "4. Them nguoi dung\n";
    cout << "5. Xoa nguoi dung\n";
    cout << "6. Tim nguoi dung\n";
    cout << "7. Danh sach nguoi dung\n";
    cout << "8. Xe vao\n";
    cout << "9. Xe ra\n";
    cout << "10. Trang thai o do\n";
    cout << "11. Tim xe theo bien so\n";
    cout << "12. Xem lich su\n";
    cout << "13. Doanh thu\n";
    cout << "14. Danh sach undo\n";
    cout << "15. Undo thao tac\n";
    cout << "0. Thoat\n";
    cout << "========================================\n";
}

int main() {
    ParkingSystem system(20, "DanhSachSV.txt");

    while (true) {
        menu();

        int choice = inputInt("Chon chuc nang: ");

        switch (choice) {
            case 1: showDashboard(system); break;
            case 2: loadMap(system); break;
            case 3: showMap(system); break;
            case 4: addUser(system); break;
            case 5: removeUser(system); break;
            case 6: findUser(system); break;
            case 7: listUsers(system); break;
            case 8: vehicleEntry(system); break;
            case 9: vehicleExit(system); break;
            case 10: listSlots(system); break;
            case 11: searchByPlate(system); break;
            case 12: showHistory(system); break;
            case 13: showRevenue(system); break;
            case 14: showUndoList(system); break;
            case 15: undoAction(system); break;
            case 0:
                cout << "Tam biet.\n";
                return 0;
            default:
                cout << "Lua chon khong hop le.\n";
        }

        pauseScreen();
    }
}