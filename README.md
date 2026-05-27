# Hệ thống Quản lý Nhà xe

Project mô phỏng hệ thống quản lý nhà xe, gồm 2 phiên bản giao diện:

- **TUI**: giao diện dòng lệnh, chạy được trên Windows và Linux.
- **GUI Win32**: giao diện cửa sổ Windows, xây dựng bằng Win32 API.

---

# 1. Phiên bản TUI - Giao diện dòng lệnh

Phiên bản TUI dùng `cin/cout`, không phụ thuộc thư viện đồ họa ngoài.

## Biên dịch trên Windows

Mở **PowerShell**, **Command Prompt**, **MSYS2 UCRT64** hoặc terminal trong VS Code tại thư mục project.

Chạy lệnh:

```bat
g++ -std=c++17 -O2 -I./app -I./lib -o parking_tui.exe app/main_tui.cpp
```

## Chạy trên Windows

Nếu dùng **PowerShell**:

```powershell
.\parking_tui.exe
```

Nếu dùng **Command Prompt**:

```cmd
parking_tui.exe
```

Nếu dùng **Git Bash / MSYS2 terminal**:

```bash
./parking_tui.exe
```

---

## Biên dịch trên Linux

Mở terminal tại thư mục project, sau đó chạy:

```bash
g++ -std=c++17 -O2 -I./app -I./lib -o parking_tui app/main_tui.cpp
```

## Chạy trên Linux

```bash
./parking_tui
```

Nếu bị lỗi `Permission denied`, chạy thêm:

```bash
chmod +x parking_tui
./parking_tui
```

---

# 2. Hệ thống Quản lý Nhà xe - Phiên bản GUI Win32

Phiên bản hoàn thiện với giao diện cửa sổ Windows thực sự, xây dựng bằng **Win32 API** thuần túy  **g++ (MSYS2)**.

## Chạy ngay (không cần cài gì)

1. Clone repo về
2. Double-click `parking_gui.exe`

> `map.txt` và `DanhSachSV.txt` phải nằm cùng thư mục với exe (đã có sẵn trong repo).

## Build lại từ source

Yêu cầu: **MSYS2** với `ucrt64` toolchain (`C:\msys64\ucrt64\bin\g++.exe`)

```bat
build.bat
```

Hoặc thủ công:
```bat
g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -I./app -I./lib -o parking_gui.exe app/main.cpp -lgdi32 -lcomctl32 -lcomdlg32 -mwindows -static -static-libgcc -static-libstdc++
```

## Giao diện - 7 Tab

| Tab | Chức năng |
|-----|-----------|
| Dashboard | Tổng quan: số ô, doanh thu, trạng thái bản đồ |
| Bản đồ | Tải file map.txt, xem sơ đồ 2D bãi xe |
| Người dùng | Thêm / xóa / tìm kiếm người dùng |
| Cổng vào/ra | Quét mã xe vào, xe ra, tính phí tự động |
| Bãi xe | Trạng thái tất cả ô, tìm kiếm theo biển số |
| Lịch sử | Toàn bộ lịch sử vào/ra (mới nhất trước) |
| Doanh thu | Thống kê doanh thu + Undo thao tác quản trị |

## Cấu trúc dữ liệu

| Cấu trúc | Mục đích |
|----------|----------|
| `HashTable` | Tra cứu người dùng O(1) |
| `AVL Tree` | Quản lý ô đỗ xe O(log n) |
| `Stack` | Undo thao tác quản trị |
| `LinkedList` | Lịch sử vào/ra |
| `Queue` | BFS tìm đường đi tối ưu trên bản đồ |

## Phí đỗ xe

| Loại | Phí |
|------|-----|
| Sinh viên | 2.000 VND/giờ |
| Giảng viên | Miễn phí |
| Khách | 5.000 VND/giờ |

## Cấu trúc thư mục

```
25120276_25120277_project/
├── parking_gui.exe      ← Chạy trực tiếp (không cần cài gì)
├── map.txt              ← Bản đồ bãi xe
├── DanhSachSV.txt       ← Danh sách người dùng (tự động lưu)
├── build.bat            ← Script build (cần MSYS2)
├── Makefile             ← Makefile cho mingw32-make
├── README.md
├── app/
│   ├── main.cpp         ← Win32 GUI + entry point
│   ├── ParkingSystem.hpp← Logic hệ thống
│   ├── ParkingMap.hpp   ← Bản đồ 2D + BFS
│   └── models.hpp       ← Data models
└── lib/
    ├── HashTable.hpp    ← Bảng băm O(1)
    ├── AVL.hpp          ← Cây AVL O(log n)
    ├── Stack.hpp        ← Ngăn xếp (Undo)
    ├── LinkedList.hpp   ← Danh sách liên kết (Lịch sử)
    ├── Queue.hpp        ← Hàng đợi (BFS)
    ├── Algorithm.hpp    ← Các thuật toán sắp xếp/tìm kiếm
    ├── BST.hpp          ← Cây tìm kiếm nhị phân
    └── PriorityQueue.hpp← Hàng đợi ưu tiên
```
