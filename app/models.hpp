#pragma once
#include <string>

enum class UserRole { STUDENT = 0, LECTURER = 1, GUEST = 2 };

inline std::string roleToString(UserRole r) {
    switch (r) {
        case UserRole::STUDENT:  return "Sinh vien";
        case UserRole::LECTURER: return "Giang vien";
        case UserRole::GUEST:    return "Khach";
        default:                 return "?";
    }
}

struct User {
    std::string id, name, licensePlate;
    UserRole role;
    User() : id(""), name(""), licensePlate(""), role(UserRole::STUDENT) {}
    User(const std::string& id, const std::string& name,
         const std::string& plate, UserRole role)
        : id(id), name(name), licensePlate(plate), role(role) {}
};

enum class SlotStatus { EMPTY = 0, OCCUPIED = 1 };

struct ParkingSlot {
    int slotId;
    SlotStatus status;
    std::string licensePlate, userId;
    ParkingSlot() : slotId(0), status(SlotStatus::EMPTY) {}
    ParkingSlot(int id) : slotId(id), status(SlotStatus::EMPTY) {}
    bool operator<(const ParkingSlot& o) const { return slotId < o.slotId; }
    bool operator>(const ParkingSlot& o) const { return slotId > o.slotId; }
    bool operator==(const ParkingSlot& o) const { return slotId == o.slotId; }
};

struct ParkingRecord {
    std::string userId, licensePlate, name, action, timestamp;
    int slotId;
    ParkingRecord() : slotId(-1) {}
    ParkingRecord(const std::string& uid, const std::string& plate,
                  const std::string& nm, int slot,
                  const std::string& act, const std::string& ts)
        : userId(uid), licensePlate(plate), name(nm),
          slotId(slot), action(act), timestamp(ts) {}
};

enum class AdminActionType { ADD_USER, REMOVE_USER };

struct AdminAction {
    AdminActionType type;
    std::string description;
    User userData;
    AdminAction() : type(AdminActionType::ADD_USER) {}
};
