#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <climits>
#include <utility>
#include "../lib/Queue.hpp"
#include "../lib/Algorithm.hpp"
using namespace std;

enum class CellType { WALL, ROAD, SLOT, TSLOT, ENTRY };
enum class SlotType  { STUDENT, TEACHER };

struct MapSlot {
    int row, col, slotId, distFromEntry;
    bool occupied;
    string licensePlate;
    SlotType type;
    MapSlot() : row(0), col(0), slotId(0), distFromEntry(INT_MAX),
                occupied(false), type(SlotType::STUDENT) {}
    MapSlot(int r, int c, int id, int dist, SlotType t = SlotType::STUDENT)
        : row(r), col(c), slotId(id), distFromEntry(dist),
          occupied(false), type(t) {}
};

class ParkingMap {
private:
    int _rows = 0, _cols = 0;
    vector<vector<CellType>> _grid;
    vector<vector<int>>      _slotId;
    vector<vector<int>>      _dist;
    vector<MapSlot>          _slots;
    int _entryRow = -1, _entryCol = -1;
    bool _loaded = false;

    void _bfs() {
        _dist.assign(_rows, vector<int>(_cols, INT_MAX));
        if (_entryRow < 0) return;
        lib::Queue<pair<int,int>> q;
        _dist[_entryRow][_entryCol] = 0;
        q.enqueue({_entryRow, _entryCol});
        int dr[] = {-1,1,0,0}, dc[] = {0,0,-1,1};
        while (!q.empty()) {
            auto [r,c] = q.front(); q.dequeue();
            for (int d = 0; d < 4; d++) {
                int nr = r+dr[d], nc = c+dc[d];
                if (nr<0||nr>=_rows||nc<0||nc>=_cols) continue;
                if (_grid[nr][nc]==CellType::WALL) continue;
                if (_dist[nr][nc]!=INT_MAX) continue;
                _dist[nr][nc] = _dist[r][c]+1;
                q.enqueue({nr,nc});
            }
        }
    }

public:
    bool loadFromFile(const string& fn) {
        ifstream fin(fn);
        if (!fin.is_open()) return false;
        fin >> _rows >> _cols;
        string line; getline(fin, line);
        _grid.assign(_rows, vector<CellType>(_cols, CellType::WALL));
        _slotId.assign(_rows, vector<int>(_cols, -1));
        _entryRow = _entryCol = -1;
        int cnt = 0;
        for (int r = 0; r < _rows; r++) {
            if (!getline(fin, line)) break;
            for (int c = 0; c < _cols && c < (int)line.size(); c++) {
                switch (line[c]) {
                    case '.': _grid[r][c]=CellType::ROAD; break;
                    case 'P': _grid[r][c]=CellType::SLOT;  _slotId[r][c]=++cnt; break;
                    case 'T': _grid[r][c]=CellType::TSLOT; _slotId[r][c]=++cnt; break;
                    case 'E': _grid[r][c]=CellType::ENTRY; _entryRow=r; _entryCol=c; break;
                    default:  _grid[r][c]=CellType::WALL; break;
                }
            }
        }
        _bfs();
        _slots.clear();
        for (int r=0;r<_rows;r++) for (int c=0;c<_cols;c++) {
            if (_grid[r][c]==CellType::SLOT)
                _slots.push_back(MapSlot(r,c,_slotId[r][c],_dist[r][c],SlotType::STUDENT));
            else if (_grid[r][c]==CellType::TSLOT)
                _slots.push_back(MapSlot(r,c,_slotId[r][c],_dist[r][c],SlotType::TEACHER));
        }
        lib::merge_sort(_slots.data(), 0, _slots.size()-1,
            [](const MapSlot& a, const MapSlot& b){ return a.distFromEntry < b.distFromEntry; });
        _loaded = true;
        return true;
    }

    bool isLoaded() const { return _loaded; }
    int totalSlots() const { return (int)_slots.size(); }
    int getSlotId(int i) const { return _slots[i].slotId; }
    int rows() const { return _rows; }
    int cols() const { return _cols; }

    int findBestEmptySlot() const {
        for (const auto& s : _slots) if (!s.occupied && s.type==SlotType::STUDENT) return s.slotId;
        return -1;
    }
    int findBestEmptyTeacherSlot() const {
        for (const auto& s : _slots) if (!s.occupied && s.type==SlotType::TEACHER) return s.slotId;
        return -1;
    }
    bool occupySlot(int id, const string& plate) {
        for (auto& s : _slots) if (s.slotId==id) { s.occupied=true; s.licensePlate=plate; return true; }
        return false;
    }
    bool freeSlot(int id) {
        for (auto& s : _slots) if (s.slotId==id) { s.occupied=false; s.licensePlate=""; return true; }
        return false;
    }
    const MapSlot* getSlotInfo(int id) const {
        for (const auto& s : _slots) if (s.slotId==id) return &s;
        return nullptr;
    }

    // Tra ve ban do dang vector<string> de ve len GUI
    vector<string> getMapLines() const {
        vector<string> lines;
        if (!_loaded) return lines;
        for (int r=0;r<_rows;r++) {
            string line;
            for (int c=0;c<_cols;c++) {
                switch (_grid[r][c]) {
                    case CellType::WALL:  line+='#'; break;
                    case CellType::ROAD:  line+='.'; break;
                    case CellType::ENTRY: line+='E'; break;
                    case CellType::SLOT: {
                        int id=_slotId[r][c]; bool occ=false;
                        for (const auto& s:_slots) if (s.slotId==id){occ=s.occupied;break;}
                        line+=(occ?'X':'P'); break;
                    }
                    case CellType::TSLOT: {
                        int id=_slotId[r][c]; bool occ=false;
                        for (const auto& s:_slots) if (s.slotId==id){occ=s.occupied;break;}
                        line+=(occ?'G':'T'); break;
                    }
                }
            }
            lines.push_back(line);
        }
        return lines;
    }
};
