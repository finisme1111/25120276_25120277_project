#pragma once
#include "../lib/HashTable.hpp"
#include "../lib/AVL.hpp"
#include "../lib/Stack.hpp"
#include "../lib/LinkedList.hpp"
#include "../lib/Algorithm.hpp"
#include "models.hpp"
#include "ParkingMap.hpp"
#include <string>
#include <vector>
#include <tuple>
#include <sstream>
#include <fstream>
#include <ctime>
#include <array>
#include <cstdio>
#include <algorithm>
using namespace std;

static const int FEE_SV = 2000, FEE_GV = 0, FEE_KH = 5000;

class ParkingSystem {
private:
    lib::HashTable<string,User>   _users;
    lib::AVL<ParkingSlot>         _slots;
    lib::Stack<AdminAction>       _undo;
    lib::LinkedList<ParkingRecord> _hist;
    ParkingMap _map;
    int _total, _occ;
    bool _useMap;
    long long _revenue;
    string _userFile;

    string _now() const {
        time_t t = time(nullptr);
        char buf[32];
        strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", localtime(&t));
        return buf;
    }
    int _minBetween(const string& a, const string& b) const {
        int h1=0,m1=0,s1=0,h2=0,m2=0,s2=0;
        sscanf(a.c_str(),"%*d/%*d/%*d %d:%d:%d",&h1,&m1,&s1);
        sscanf(b.c_str(),"%*d/%*d/%*d %d:%d:%d",&h2,&m2,&s2);
        int sec=(h2*3600+m2*60+s2)-(h1*3600+m1*60+s1);
        if(sec<0) sec+=86400;
        return sec/60;
    }
    int _fee(UserRole r) const {
        if(r==UserRole::STUDENT) return FEE_SV;
        if(r==UserRole::LECTURER) return FEE_GV;
        return FEE_KH;
    }
    int _findSlot(UserRole role) const {
        if (!_useMap) {
            int f=-1;
            _slots.inorder([&](const ParkingSlot& s){
                if(f==-1 && s.status==SlotStatus::EMPTY) f=s.slotId;
            });
            return f;
        }
        if (role==UserRole::LECTURER) {
            int s=_map.findBestEmptyTeacherSlot();
            return s!=-1 ? s : _map.findBestEmptySlot();
        }
        return _map.findBestEmptySlot();
    }
    void _appendFile(const User& u) const {
        if(_userFile.empty()) return;
        ofstream f(_userFile, ios::app);
        if(f) f<<u.id<<"|"<<u.name<<"|"<<u.licensePlate<<"|"<<(int)u.role<<"\n";
    }
    void _rewriteFile() const {
        if(_userFile.empty()) return;
        ofstream f(_userFile, ios::trunc);
        if(!f) return;
        _users.forEach([&](const string&, const User& u){
            f<<u.id<<"|"<<u.name<<"|"<<u.licensePlate<<"|"<<(int)u.role<<"\n";
        });
    }
    void _loadFile() {
        if(_userFile.empty()) return;
        ifstream f(_userFile);
        if(!f) return;
        string line;
        while(getline(f,line)) {
            if(line.empty()) continue;
            stringstream ss(line);
            string id,name,plate,rs;
            if(!getline(ss,id,'|')) continue;
            if(!getline(ss,name,'|')) continue;
            if(!getline(ss,plate,'|')) continue;
            if(!getline(ss,rs)) continue;
            int ri=0; try{ri=stoi(rs);}catch(...){continue;}
            if(ri<0||ri>2) continue;
            if(!_users.contains(id))
                _users.insert(id,User(id,name,plate,static_cast<UserRole>(ri)));
        }
    }

public:
    ParkingSystem(int total=20, const string& file="DanhSachSV.txt")
        : _total(total), _occ(0), _useMap(false), _revenue(0), _userFile(file) {
        for(int i=1;i<=total;i++) _slots.insert(ParkingSlot(i));
        _loadFile();
    }

    // ── Ban do ─────────────────────────────────────────────
    bool loadMap(const string& fn) {
        if(!_map.loadFromFile(fn)) return false;
        _slots.clear(); _total=_map.totalSlots(); _occ=0;
        for(int i=0;i<_total;i++) _slots.insert(ParkingSlot(_map.getSlotId(i)));
        _useMap=true;
        return true;
    }
    bool mapLoaded() const { return _useMap; }
    vector<string> getMapLines() const { return _map.getMapLines(); }

    // ── Nguoi dung ─────────────────────────────────────────
    bool addUser(const string& id, const string& name, const string& plate, UserRole role) {
        if(_users.contains(id)) return false;
        User u(id,name,plate,role);
        _users.insert(id,u);
        _appendFile(u);
        AdminAction a; a.type=AdminActionType::ADD_USER;
        a.description="Them: "+name+" ("+id+")"; a.userData=u;
        _undo.push(a);
        return true;
    }
    bool removeUser(const string& id) {
        User* u=_users.find(id);
        if(!u) return false;
        AdminAction a; a.type=AdminActionType::REMOVE_USER;
        a.description="Xoa: "+u->name+" ("+id+")"; a.userData=*u;
        _undo.push(a);
        _users.remove(id); _rewriteFile();
        return true;
    }
    const User* getUser(const string& id) const { return _users.find(id); }

    // Danh sach nguoi dung: {id, name, plate, role}
    vector<array<string,4>> getUserList() const {
        vector<array<string,4>> v;
        _users.forEach([&](const string&, const User& u){
            v.push_back({u.id, u.name, u.licensePlate, roleToString(u.role)});
        });
        return v;
    }

    // ── Xe vao/ra ──────────────────────────────────────────
    // Tra ve {ok, message}
    pair<bool,string> vehicleEntry(const string& id) {
        User* u=_users.find(id);
        if(!u) return {false,"[LOI] Ma so '"+id+"' khong co trong he thong."};
        bool in=false;
        _slots.inorder([&](const ParkingSlot& s){ if(s.userId==id) in=true; });
        if(in) return {false,"[LOI] Xe cua '"+u->name+"' da co trong bai."};
        int sid=_findSlot(u->role);
        if(sid==-1) return {false,"[LOI] Bai xe day, khong con o trong!"};
        ParkingSlot* sl=_slots.find(ParkingSlot(sid));
        if(sl){ sl->status=SlotStatus::OCCUPIED; sl->licensePlate=u->licensePlate; sl->userId=id; }
        if(_useMap) _map.occupySlot(sid,u->licensePlate);
        _occ++;
        string ts=_now();
        _hist.insertBack(ParkingRecord(id,u->licensePlate,u->name,sid,"VAO",ts));
        string msg="[VAO] "+u->name+" | "+u->licensePlate+" | O #"+to_string(sid)+" | "+ts;
        if(_useMap){
            const MapSlot* ms=_map.getSlotInfo(sid);
            if(ms) msg+="\n      Vi tri: hang "+to_string(ms->row)+", cot "+to_string(ms->col)
                       +" | Cach cong: "+to_string(ms->distFromEntry)+" buoc";
        }
        return {true,msg};
    }

    pair<bool,string> vehicleExit(const string& id) {
        User* u=_users.find(id);
        if(!u) return {false,"[LOI] Ma so '"+id+"' khong co trong he thong."};
        int fs=-1;
        _slots.inorder([&](const ParkingSlot& s){ if(s.userId==id) fs=s.slotId; });
        if(fs==-1) return {false,"[THONG BAO] Xe cua '"+u->name+"' khong co trong bai."};
        string tin="";
        for(int i=(int)_hist.size()-1;i>=0;i--){
            const auto& r=_hist.get(i);
            if(r.userId==id && r.action=="VAO"){ tin=r.timestamp; break; }
        }
        string tout=_now();
        int mins=_minBetween(tin,tout);
        int fee=0, rate=_fee(u->role);
        if(rate>0){ int h=(mins<60)?1:(mins+59)/60; fee=h*rate; _revenue+=fee; }
        ParkingSlot* sl=_slots.find(ParkingSlot(fs));
        if(sl){ sl->status=SlotStatus::EMPTY; sl->licensePlate=""; sl->userId=""; }
        if(_useMap) _map.freeSlot(fs);
        _occ--;
        _hist.insertBack(ParkingRecord(id,u->licensePlate,u->name,fs,"RA",tout));
        string msg="[RA] "+u->name+" | "+u->licensePlate+" | O #"+to_string(fs)
                  +"\n     Vao: "+(tin.empty()?"?":tin)+" | Ra: "+tout
                  +"\n     Thoi gian: "+to_string(mins)+" phut";
        if(rate==0) msg+="\n     Phi: Mien phi ("+roleToString(u->role)+")";
        else        msg+="\n     Phi: "+to_string(fee)+" VND";
        return {true,msg};
    }

    // ── Trang thai o do xe ─────────────────────────────────
    // {slotId, status, plate, userId}
    vector<array<string,4>> getSlotList() const {
        vector<array<string,4>> v;
        _slots.inorder([&](const ParkingSlot& s){
            v.push_back({to_string(s.slotId),
                         to_string((int)s.status),
                         s.licensePlate, s.userId});
        });
        return v;
    }

    string searchByPlate(const string& plate) const {
        string res;
        _slots.inorder([&](const ParkingSlot& s){
            if(s.status==SlotStatus::OCCUPIED && s.licensePlate==plate){
                res="O #"+to_string(s.slotId)+" | Ma so: "+s.userId;
                if(_useMap){
                    const MapSlot* ms=_map.getSlotInfo(s.slotId);
                    if(ms) res+=" | Hang "+to_string(ms->row)+", cot "+to_string(ms->col);
                }
            }
        });
        return res.empty() ? "Khong tim thay bien so '"+plate+"'" : res;
    }

    // ── Lich su ────────────────────────────────────────────
    // {timestamp, action, name, plate, slotId}
    vector<array<string,5>> getHistoryList(int n=100) const {
        vector<array<string,5>> v;
        int total=(int)_hist.size(), start=max(0,total-n);
        for(int i=start;i<total;i++){
            const auto& r=_hist.get(i);
            v.push_back({r.timestamp,r.action,r.name,r.licensePlate,to_string(r.slotId)});
        }
        return v;
    }

    // ── Doanh thu ──────────────────────────────────────────
    long long getRevenue() const { return _revenue; }

    string getRevenueText() const {
        int sc=0,gc=0; long long sr=0,gr=0;
        for(int i=0;i<(int)_hist.size();i++){
            const auto& r=_hist.get(i);
            if(r.action!="RA") continue;
            const User* u=_users.find(r.userId);
            if(!u) continue;
            string tin="";
            for(int j=i-1;j>=0;j--){
                const auto& rv=_hist.get(j);
                if(rv.userId==r.userId&&rv.action=="VAO"){tin=rv.timestamp;break;}
            }
            if(tin.empty()) continue;
            int h=_minBetween(tin,r.timestamp); int hrs=(h<60)?1:(h+59)/60;
            if(u->role==UserRole::STUDENT){sc++;sr+=hrs*FEE_SV;}
            if(u->role==UserRole::GUEST){gc++;gr+=hrs*FEE_KH;}
        }
        ostringstream o;
        o<<"Sinh vien : "<<sc<<" luot | "<<sr<<" VND ("<<FEE_SV<<" VND/gio)\n";
        o<<"Khach     : "<<gc<<" luot | "<<gr<<" VND ("<<FEE_KH<<" VND/gio)\n";
        o<<"Giang vien: Mien phi\n";
        o<<"----------------------------\n";
        o<<"TONG CONG : "<<_revenue<<" VND";
        return o.str();
    }

    // ── Undo ───────────────────────────────────────────────
    string doUndo() {
        if(_undo.empty()) return "Khong co hanh dong nao de hoan tac.";
        AdminAction a=_undo.top(); _undo.pop();
        string msg="[UNDO] "+a.description;
        if(a.type==AdminActionType::ADD_USER){
            _users.remove(a.userData.id); _rewriteFile();
            msg+="\n=> Da xoa: "+a.userData.name;
        } else {
            _users.insert(a.userData.id,a.userData); _appendFile(a.userData);
            msg+="\n=> Da khoi phuc: "+a.userData.name;
        }
        return msg;
    }

    vector<string> getUndoList() const {
        vector<string> v;
        lib::Stack<AdminAction> tmp=_undo;
        while(!tmp.empty()){ v.push_back(tmp.top().description); tmp.pop(); }
        return v;
    }

    // ── Dashboard ──────────────────────────────────────────
    struct Stats { int total,occ,users,hist; long long revenue; bool mapLoaded; };
    Stats getStats() const {
        return {_total,_occ,(int)_users.size(),(int)_hist.size(),_revenue,_useMap};
    }
};
