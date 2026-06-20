#ifndef PARKINGMAP_TUI_HPP
#define PARKINGMAP_TUI_HPP

#include "models_tui.hpp"
#include "../lib/Queue.hpp"

#include <climits>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class CellType {
    WALL,
    ROAD,
    SLOT,
    TSLOT,
    ENTRY
};

enum class SlotType {
    STUDENT,
    TEACHER
};

struct MapSlot {
    int row;
    int col;
    int slotId;
    int distFromEntry;
    bool occupied;
    std::string licensePlate;
    std::string userId;
    std::string slotCode;
    SlotType type;

    MapSlot()
        : row(-1),
          col(-1),
          slotId(0),
          distFromEntry(INT_MAX),
          occupied(false),
          type(SlotType::STUDENT) {}

    MapSlot(
        int mapRow,
        int mapCol,
        int id,
        int distance,
        SlotType slotType,
        const std::string& code
    )
        : row(mapRow),
          col(mapCol),
          slotId(id),
          distFromEntry(distance),
          occupied(false),
          slotCode(code),
          type(slotType) {}
};

class ParkingMap {
private:
    int _rows;
    int _cols;
    std::vector<std::vector<CellType>> _grid;
    std::vector<std::vector<int>> _slotId;
    std::vector<std::vector<int>> _dist;
    std::vector<MapSlot> _slots;
    int _entryRow;
    int _entryCol;
    bool _loaded;

    static bool isParkingCell(CellType type) {
        return type == CellType::SLOT ||
               type == CellType::TSLOT;
    }

    bool isInside(int row, int col) const {
        return row >= 0 && row < _rows &&
               col >= 0 && col < _cols;
    }

    bool isWalkable(int row, int col) const {
        if (!isInside(row, col)) {
            return false;
        }

        return _grid[row][col] == CellType::ROAD ||
               _grid[row][col] == CellType::ENTRY;
    }

    void runBfs() {
        _dist.assign(
            _rows,
            std::vector<int>(_cols, INT_MAX)
        );

        if (!isInside(_entryRow, _entryCol)) {
            return;
        }

        lib::Queue<std::pair<int, int>> queue;
        _dist[_entryRow][_entryCol] = 0;
        queue.enqueue({_entryRow, _entryCol});

        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};

        while (!queue.empty()) {
            const std::pair<int, int> current = queue.front();
            queue.dequeue();

            const int row = current.first;
            const int col = current.second;

            for (int direction = 0; direction < 4; ++direction) {
                const int nextRow = row + dr[direction];
                const int nextCol = col + dc[direction];

                if (!isWalkable(nextRow, nextCol)) {
                    continue;
                }

                if (_dist[nextRow][nextCol] != INT_MAX) {
                    continue;
                }

                _dist[nextRow][nextCol] =
                    _dist[row][col] + 1;

                queue.enqueue({nextRow, nextCol});
            }
        }
    }

    int distanceToSlot(int row, int col) const {
        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};
        int best = INT_MAX;

        for (int direction = 0; direction < 4; ++direction) {
            const int roadRow = row + dr[direction];
            const int roadCol = col + dc[direction];

            if (!isWalkable(roadRow, roadCol)) {
                continue;
            }

            if (_dist[roadRow][roadCol] == INT_MAX) {
                continue;
            }

            const int candidate =
                _dist[roadRow][roadCol] + 1;

            if (candidate < best) {
                best = candidate;
            }
        }

        return best;
    }

    static char areaLetter(int areaIndex) {
        static const char letters[10] = {
            'A', 'B', 'C', 'D', 'E',
            'F', 'G', 'H', 'I', 'K'
        };

        if (areaIndex >= 0 && areaIndex < 10) {
            return letters[areaIndex];
        }

        return '?';
    }

    static std::string makeSlotCode(
        int areaIndex,
        int section,
        int spot
    ) {
        std::ostringstream output;
        output << areaLetter(areaIndex)
               << section
               << std::setw(2)
               << std::setfill('0')
               << spot;
        return output.str();
    }

    void buildSlots() {
        _slots.clear();

        std::vector<int> parkingRows;
        for (int row = 0; row < _rows; ++row) {
            bool containsSlot = false;

            for (int col = 0; col < _cols; ++col) {
                if (isParkingCell(_grid[row][col])) {
                    containsSlot = true;
                    break;
                }
            }

            if (containsSlot) {
                parkingRows.push_back(row);
            }
        }

        for (std::size_t rowIndex = 0;
             rowIndex < parkingRows.size();
             ++rowIndex) {
            const int row = parkingRows[rowIndex];

            // Gate E is at the bottom of the supplied map, therefore
            // the two bottom parking rows form area A.
            const int reversedIndex =
                static_cast<int>(parkingRows.size() - 1 - rowIndex);
            const int areaIndex = reversedIndex / 2;

            // In each area, the row closer to the gate contains spots 01-07.
            const bool rowCloserToGate =
                (rowIndex % 2 == 1);
            const int firstSpot = rowCloserToGate ? 1 : 8;

            int section = 0;
            int col = 0;

            while (col < _cols) {
                if (!isParkingCell(_grid[row][col])) {
                    ++col;
                    continue;
                }

                const int runStart = col;
                while (col < _cols &&
                       isParkingCell(_grid[row][col])) {
                    ++col;
                }

                ++section;

                for (int slotCol = runStart;
                     slotCol < col;
                     ++slotCol) {
                    const int spot =
                        firstSpot + (slotCol - runStart);
                    const int id = _slotId[row][slotCol];
                    const SlotType type =
                        _grid[row][slotCol] == CellType::TSLOT
                            ? SlotType::TEACHER
                            : SlotType::STUDENT;

                    _slots.push_back(
                        MapSlot(
                            row,
                            slotCol,
                            id,
                            distanceToSlot(row, slotCol),
                            type,
                            makeSlotCode(
                                areaIndex,
                                section,
                                spot
                            )
                        )
                    );
                }
            }
        }
    }

    MapSlot* findMapSlot(int id) {
        for (MapSlot& slot : _slots) {
            if (slot.slotId == id) {
                return &slot;
            }
        }

        return nullptr;
    }

    const MapSlot* findMapSlot(int id) const {
        for (const MapSlot& slot : _slots) {
            if (slot.slotId == id) {
                return &slot;
            }
        }

        return nullptr;
    }

    static bool acceptsRole(
        const MapSlot& slot,
        UserRole role
    ) {
        if (role == UserRole::LECTURER) {
            return slot.type == SlotType::TEACHER;
        }

        return slot.type == SlotType::STUDENT;
    }

    static void copyToParkingSlot(
        const MapSlot& source,
        ParkingSlot& destination
    ) {
        destination.slotId = source.slotId;
        destination.slotCode = source.slotCode;
        destination.row = source.row;
        destination.col = source.col;
        destination.distanceFromEntry = source.distFromEntry;
        destination.baseType =
            source.type == SlotType::TEACHER ? 'T' : 'P';
        destination.status = source.occupied
            ? SlotStatus::OCCUPIED
            : SlotStatus::EMPTY;
        destination.licensePlate = source.licensePlate;
        destination.userId = source.userId;
    }

public:
    ParkingMap()
        : _rows(0),
          _cols(0),
          _entryRow(-1),
          _entryCol(-1),
          _loaded(false) {}

    bool loadFromFile(const std::string& fileName) {
        std::ifstream input(fileName);
        if (!input) {
            return false;
        }

        int rows = 0;
        int cols = 0;
        if (!(input >> rows >> cols) || rows <= 0 || cols <= 0) {
            return false;
        }

        std::string line;
        std::getline(input, line);

        _rows = rows;
        _cols = cols;
        _grid.assign(
            _rows,
            std::vector<CellType>(_cols, CellType::WALL)
        );
        _slotId.assign(
            _rows,
            std::vector<int>(_cols, -1)
        );
        _entryRow = -1;
        _entryCol = -1;
        _loaded = false;

        int nextSlotId = 0;

        for (int row = 0; row < _rows; ++row) {
            if (!std::getline(input, line)) {
                return false;
            }

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (static_cast<int>(line.size()) < _cols) {
                return false;
            }

            for (int col = 0; col < _cols; ++col) {
                switch (line[col]) {
                    case '.':
                        _grid[row][col] = CellType::ROAD;
                        break;

                    case 'P':
                        _grid[row][col] = CellType::SLOT;
                        _slotId[row][col] = ++nextSlotId;
                        break;

                    case 'T':
                        _grid[row][col] = CellType::TSLOT;
                        _slotId[row][col] = ++nextSlotId;
                        break;

                    case 'E':
                        _grid[row][col] = CellType::ENTRY;
                        _entryRow = row;
                        _entryCol = col;
                        break;

                    default:
                        _grid[row][col] = CellType::WALL;
                        break;
                }
            }
        }

        if (_entryRow < 0 || _entryCol < 0) {
            return false;
        }

        runBfs();
        buildSlots();
        _loaded = true;
        return true;
    }

    bool isLoaded() const {
        return _loaded;
    }

    int totalSlots() const {
        return static_cast<int>(_slots.size());
    }

    int getSlotId(int index) const {
        if (index < 0 ||
            index >= static_cast<int>(_slots.size())) {
            return -1;
        }

        return _slots[static_cast<std::size_t>(index)].slotId;
    }

    int rows() const {
        return _rows;
    }

    int cols() const {
        return _cols;
    }

    bool findNearestAvailableSlot(
        UserRole role,
        ParkingSlot& result
    ) const {
        const MapSlot* best = nullptr;

        for (const MapSlot& slot : _slots) {
            if (slot.occupied || !acceptsRole(slot, role)) {
                continue;
            }

            if (slot.distFromEntry == INT_MAX) {
                continue;
            }

            if (!best ||
                slot.distFromEntry < best->distFromEntry ||
                (slot.distFromEntry == best->distFromEntry &&
                 slot.slotCode < best->slotCode)) {
                best = &slot;
            }
        }

        if (!best) {
            return false;
        }

        copyToParkingSlot(*best, result);
        return true;
    }

    int findBestEmptySlot() const {
        ParkingSlot result;
        if (!findNearestAvailableSlot(
                UserRole::STUDENT,
                result
            )) {
            return -1;
        }

        return result.slotId;
    }

    int findBestEmptyTeacherSlot() const {
        ParkingSlot result;
        if (!findNearestAvailableSlot(
                UserRole::LECTURER,
                result
            )) {
            return -1;
        }

        return result.slotId;
    }

    bool occupySlot(
        int id,
        const std::string& userId,
        const std::string& licensePlate
    ) {
        MapSlot* slot = findMapSlot(id);
        if (!slot || slot->occupied) {
            return false;
        }

        slot->occupied = true;
        slot->userId = userId;
        slot->licensePlate = licensePlate;
        return true;
    }

    // Compatibility with the old ParkingSystem implementation.
    bool occupySlot(
        int id,
        const std::string& licensePlate
    ) {
        return occupySlot(id, "", licensePlate);
    }

    bool releaseSlot(int id) {
        MapSlot* slot = findMapSlot(id);
        if (!slot) {
            return false;
        }

        slot->occupied = false;
        slot->userId.clear();
        slot->licensePlate.clear();
        return true;
    }

    // Compatibility with the old ParkingMap API.
    bool freeSlot(int id) {
        return releaseSlot(id);
    }

    bool isSlotAvailable(int id) const {
        const MapSlot* slot = findMapSlot(id);
        return slot && !slot->occupied;
    }

    const MapSlot* getSlotInfo(int id) const {
        return findMapSlot(id);
    }

    bool getSlot(int id, ParkingSlot& result) const {
        const MapSlot* slot = findMapSlot(id);
        if (!slot) {
            return false;
        }

        copyToParkingSlot(*slot, result);
        return true;
    }

    void forEachSlot(
        const std::function<void(const ParkingSlot&)>& function
    ) const {
        for (const MapSlot& source : _slots) {
            ParkingSlot slot;
            copyToParkingSlot(source, slot);
            function(slot);
        }
    }

    std::vector<ParkingSlot> getSlots() const {
        std::vector<ParkingSlot> result;
        result.reserve(_slots.size());

        forEachSlot(
            [&](const ParkingSlot& slot) {
                result.push_back(slot);
            }
        );

        return result;
    }

    std::vector<std::string> getMapLines() const {
        std::vector<std::string> lines;
        if (!_loaded) {
            return lines;
        }

        lines.reserve(static_cast<std::size_t>(_rows));

        for (int row = 0; row < _rows; ++row) {
            std::string line;
            line.reserve(static_cast<std::size_t>(_cols));

            for (int col = 0; col < _cols; ++col) {
                switch (_grid[row][col]) {
                    case CellType::WALL:
                        line += '#';
                        break;

                    case CellType::ROAD:
                        line += '.';
                        break;

                    case CellType::ENTRY:
                        line += 'E';
                        break;

                    case CellType::SLOT:
                    case CellType::TSLOT: {
                        const MapSlot* slot = findMapSlot(
                            _slotId[row][col]
                        );

                        if (slot && slot->occupied) {
                            line += 'X';
                        } else if (
                            _grid[row][col] == CellType::TSLOT
                        ) {
                            line += 'T';
                        } else {
                            line += 'P';
                        }
                        break;
                    }
                }
            }

            lines.push_back(line);
        }

        return lines;
    }
};

#endif // PARKINGMAP_TUI_HPP
