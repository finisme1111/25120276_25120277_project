# Hệ thống Quản lý Nhà xe

## 1. Giới thiệu

Project C++ mô phỏng hệ thống quản lý nhà xe với hai lớp giao diện:

- TUI chạy trên Linux terminal, Windows Terminal, MSYS2 và console Windows khi hỗ trợ UTF-8.
- GUI Win32 chạy trên Windows bằng Win32 API Unicode.

TUI và GUI dùng chung backend nghiệp vụ, chung model dữ liệu, chung loader/saver và chung định dạng file.

## 2. Thành viên

- 25120276
- 25120277

## 3. Kiến trúc

Kiến trúc sau khi chuẩn hóa:

```text
Shared models              app/models.hpp -> app/models_tui.hpp
Shared ParkingMap backend  app/ParkingMap.hpp -> app/ParkingMap_tui.hpp
Shared ParkingSystem       app/ParkingSystem.hpp -> app/ParkingSystem_tui.hpp
TUI presentation           app/main_tui.cpp, app/Terminal.hpp
Win32 GUI presentation     app/main.cpp
Data structures            lib/*.hpp
```

Các file `_tui.hpp` hiện là implementation tương thích lịch sử. Header gốc (`models.hpp`, `ParkingMap.hpp`, `ParkingSystem.hpp`) là điểm include chung cho GUI/TUI để tránh hai backend độc lập.

## 4. Cấu trúc thư mục

```text
app/
  main.cpp                 GUI Win32
  main_tui.cpp             TUI
  models.hpp               wrapper model chung
  models_tui.hpp           định nghĩa model chung
  ParkingMap.hpp           wrapper map chung
  ParkingMap_tui.hpp       map backend + BFS
  ParkingSystem.hpp        wrapper business backend chung
  ParkingSystem_tui.hpp    business backend
  Terminal.hpp             xử lý phím/terminal cho TUI
lib/
  Algorithm.hpp            thuật toán tìm kiếm/sắp xếp
  AVL.hpp                  cây AVL
  BST.hpp                  cây BST tối giản
  HashTable.hpp            hash table separate chaining bằng AVL
  LinkedList.hpp           danh sách liên kết đôi
  PriorityQueue.hpp        priority queue bằng AVL, hỗ trợ trùng giá trị
  Queue.hpp                queue
  Stack.hpp                stack
tests/
  backend_tests.cpp        test backend trên dữ liệu tạm
test_data/
  README.md                ghi chú dữ liệu test riêng
DanhSachSV.txt             dữ liệu người dùng
map.txt                    bản đồ gốc
active_sessions.txt        xe đang gửi
history.txt                lịch sử sự kiện
monthly_usage.txt          thống kê tháng
monthly_bills.txt          hóa đơn tháng
revenue.txt                snapshot doanh thu legacy, không là source of truth
Makefile
build.bat
```

## 5. Tính năng

- Tổng quan hệ thống.
- Xe vào bằng ID, tự tìm ô hợp lệ gần cổng.
- Xe ra có xem thông tin và xác nhận ID lần hai.
- Đăng ký người dùng, kiểm tra trùng ID và biển số.
- Tra cứu và danh sách người dùng.
- Danh sách xe đang gửi, tìm vị trí theo biển số.
- Xem bản đồ trạng thái.
- Lịch sử gửi xe cá nhân.
- Hóa đơn, thanh toán, blacklist.
- Thống kê tháng theo người dùng.
- Undo xe vào/xe ra trong phiên chạy hiện tại.
- Persistence chung giữa TUI và GUI.

## 6. Quy tắc map

Ký tự trong `map.txt`:

| Ký tự | Ý nghĩa |
|---|---|
| `#` | Tường |
| `.` | Lối đi |
| `E` | Cổng |
| `P` | Ô sinh viên hoặc khách |
| `T` | Ô giảng viên |
| `X` | Ô đang có xe khi hiển thị |

`map.txt` giữ map gốc, không cần ghi `X`. Trạng thái đang đỗ được khôi phục từ `active_sessions.txt`.

## 7. Quy tắc mã ô

Có 10 khu lớn: `A, B, C, D, E, F, G, H, I, K`. Khu `A` gần cổng nhất. Mỗi khu có 8 khu nhỏ, mỗi khu nhỏ có 14 ô `01` đến `14`.

Ví dụ mã ô: `A103`, `E511`, `I713`.

Backend duyệt BFS từ cổng qua lối đi, tính khoảng cách đến ô đỗ qua ô đường kề bên. Nếu nhiều ô cùng khoảng cách, chọn mã ô nhỏ hơn.

## 8. Phân loại người dùng

| Role | Ý nghĩa | Loại ô |
|---|---|---|
| `0` | Sinh viên | `P` |
| `1` | Giảng viên | `T` |
| `2` | Khách ngoài trường | `P` |

Mỗi người dùng có: `ID`, họ tên, biển số, role, tổng số lượt/đơn vị gửi xe đã hoàn thành.

## 9. Quy tắc tính phí

Một đơn vị/lượt tương ứng tối đa 10 giờ:

```cpp
units = max(1, ceil(durationSeconds / 36000.0));
```

Trong code dùng công thức số nguyên tương đương.

| Role | Phí mặc định |
|---|---:|
| Sinh viên | 3000 VND/lượt |
| Giảng viên | 5000 VND/lượt |
| Khách ngoài trường | 5000 VND/lượt |

Các mức phí khai báo tập trung trong `ParkingSystem`.

## 10. Hóa đơn, blacklist và phạt

Cuối tháng, hệ thống tạo hóa đơn từ `monthly_usage.txt` cho các tháng đã qua. Hóa đơn gồm bill ID, user ID, năm, tháng, tổng lượt, tiền gốc, ngày tạo, hạn thanh toán, trạng thái thanh toán và thời gian thanh toán.

Người dùng có hóa đơn quá hạn chưa trả sẽ bị blacklist và không được gửi xe mới. Xe đang ở trong bãi vẫn được lấy ra.

Mức phạt:

```text
10000 VND mỗi ngày quá hạn
Tổng phải trả = baseAmount + overdueDays * 10000
```

Sau khi thanh toán hết hóa đơn quá hạn, người dùng tự hết blacklist.

## 11. Undo

Undo dùng stack trong bộ nhớ của phiên chạy hiện tại.

- Undo xe vào: xóa active session, giải phóng ô, ghi sự kiện `HOAN_TAC_VAO`.
- Undo xe ra: khôi phục active session, khôi phục ô `X`, trừ tổng lượt và thống kê tháng, ghi `HOAN_TAC_RA`.
- Nếu ô cũ đã bị chiếm bởi xe khác, undo xe ra bị từ chối.
- Không xóa lịch sử cũ khi undo; hệ thống append thêm sự kiện undo.

## 12. Cấu trúc dữ liệu

| Cấu trúc | Vai trò |
|---|---|
| `LinkedList` | Lưu lịch sử trong bộ nhớ, hỗ trợ duyệt tuần tự. |
| `Stack` | Lưu undo action xe vào/ra. |
| `Queue` | Dùng trong BFS tìm khoảng cách map. |
| `PriorityQueue` | Hàng đợi ưu tiên bằng AVL, có thứ tự chèn để hỗ trợ phần tử trùng. |
| `BST` | Cây tìm kiếm nhị phân tối giản, vẫn build và quản lý bộ nhớ đúng. |
| `AVL` | Cây cân bằng, dùng cho slot và bucket hash. |
| `HashTable` | Separate chaining bằng AVL chứa `pair<key,value>`. |
| `Algorithm` | Linear/binary search, selection/insertion/bubble/heap/merge/quick sort. |

## 13. Định dạng dữ liệu

Tất cả file text dùng UTF-8, phân tách bằng `|`, xử lý được LF/CRLF và bỏ qua dòng lỗi hợp lý.

### `DanhSachSV.txt`

Chuẩn mới:

```text
id|fullName|licensePlate|role|totalParkingUnits
```

Loader vẫn đọc được format cũ 4 trường:

```text
id|fullName|licensePlate|role
```

Khi gặp dòng cũ, `totalParkingUnits = 0`.

### `active_sessions.txt`

Chuẩn mới:

```text
sessionId|userId|licensePlate|slotId|slotCode|entryTimeEpoch|entryTimestamp
```

Loader đọc thêm format cũ:

```text
userId|slotId|licensePlate|entryTimestamp
```

### `history.txt`

Chuẩn mới:

```text
eventId|sessionId|userId|licensePlate|name|slotId|slotCode|action|eventTimeEpoch|timestamp|durationSeconds|chargedUnits|amount
```

`action` gồm: `VAO`, `RA`, `HOAN_TAC_VAO`, `HOAN_TAC_RA`, `THANH_TOAN`.

Loader đọc thêm format cũ:

```text
userId|licensePlate|name|slotId|action|timestamp
```

### `monthly_usage.txt`

```text
userId|year|month|parkingUnits|amount
```

### `monthly_bills.txt`

```text
billId|userId|year|month|parkingUnits|baseAmount|createdAtEpoch|dueDateEpoch|paid|paidAtEpoch
```

`paid`: `0` hoặc `1`.

### `revenue.txt`

```text
day|month|year|amount|count
```

File này được giữ để tương thích dữ liệu cũ/snapshot thủ công. Backend hiện không dùng `revenue.txt` làm nguồn sự thật để tránh đếm tiền hai lần. Doanh thu hiển thị được suy ra từ `monthly_usage.txt` và hóa đơn từ `monthly_bills.txt`.

## 14. Build Linux TUI

```bash
make clean
make tui
./parking_tui
```

Hoặc build debug:

```bash
make debug
```

## 15. Build Windows TUI

Trong MSYS2 UCRT64, PowerShell hoặc Command Prompt có `g++`:

```bat
build.bat tui
build.bat run-tui
```

Hoặc dùng Makefile trong MSYS2:

```bash
make tui
make run-tui
```

## 16. Build Windows GUI

GUI dùng Win32 API, chỉ build trên Windows/MSYS2:

```bat
build.bat gui
build.bat run-gui
```

Hoặc:

```bash
make gui
make run-gui
```

Có thể chỉ định compiler:

```bash
make CXX=C:/msys64/ucrt64/bin/g++.exe gui
```

## 17. Makefile

Target chính:

```bash
make
make tui
make gui
make run-tui
make run-gui
make test
make clean
make debug
make info
```

Trên Linux, `make` build `parking_tui`. Target `gui` chỉ báo rằng Win32 GUI cần Windows.

## 18. build.bat

`build.bat` tự chuyển về thư mục project bằng `%~dp0`, bật UTF-8 bằng `chcp 65001`, ưu tiên compiler `C:\msys64\ucrt64\bin\g++.exe`, sau đó tìm `g++` trong PATH.

Lệnh hỗ trợ:

```bat
build.bat
build.bat all
build.bat tui
build.bat gui
build.bat clean
build.bat run-tui
build.bat run-gui
build.bat debug
build.bat test
```

## 19. Chạy với dữ liệu mặc định

Đặt các file dữ liệu `.txt` cùng thư mục chạy, sau đó:

```bash
./parking_tui
```

Windows GUI mặc định đọc thư mục hiện tại. Có thể truyền thư mục dữ liệu làm tham số đầu tiên:

```bat
parking_gui.exe path\to\data
```

## 20. Chạy với thư mục test data

TUI nhận thư mục dữ liệu ở tham số đầu tiên:

```bash
./parking_tui /duong/dan/toi/test-data
```

Test tự động tạo dữ liệu tạm trong `/tmp` và không ghi đè dữ liệu thật:

```bash
make test
```

## 21. UTF-8 và font terminal

Source và dữ liệu dùng UTF-8.

Linux nên chạy trong locale UTF-8:

```bash
locale
```

Windows TUI gọi `SetConsoleOutputCP(CP_UTF8)` và `SetConsoleCP(CP_UTF8)`. Nên dùng Windows Terminal với font hỗ trợ tiếng Việt như Cascadia Mono, Consolas hoặc Segoe UI Mono.

GUI Win32 dùng API Unicode (`CreateWindowExW`, `MessageBoxW`, `SetWindowTextW`) và chỉ chuyển UTF-8 sang UTF-16 tại ranh giới Win32.

Các chuỗi tiếng Việt kiểm tra: `HỆ THỐNG QUẢN LÝ NHÀ XE`, `Đăng ký người dùng`, `Lịch sử gửi xe cá nhân`, `Hóa đơn và thanh toán`, `Giảng viên`, `Khách ngoài trường`.

## 22. Test case chính

`tests/backend_tests.cpp` bao phủ:

- Đọc user 4 trường và 5 trường.
- Đăng ký mới, trùng ID, trùng biển số, role file không hợp lệ.
- Xe vào sinh viên/giảng viên/khách nhận đúng loại ô.
- ID không tồn tại, xe đã ở trong bãi, blacklist, hết ô phù hợp ở nhánh map giới hạn.
- Xe ra xác nhận sai ID, đúng 10 giờ, trên 10 giờ, trên 20 giờ, giải phóng ô.
- Hóa đơn quá hạn, phạt nhiều ngày, thanh toán, gỡ blacklist.
- Người blacklist vẫn lấy được xe đang gửi.
- Undo entry, undo exit, undo exit khi ô cũ bị chiếm, không có undo.
- Persistence xe vào và reload active session.

Chạy:

```bash
make test
```

## 23. Hạn chế còn lại

- Undo chỉ tồn tại trong phiên chạy hiện tại, không khôi phục stack undo sau khi tắt app.
- GUI Win32 cần môi trường Windows/MSYS2 để build và chạy thực tế; trên Linux chỉ kiểm tra tĩnh được mã nguồn.
- `revenue.txt` là legacy snapshot, không còn là nguồn tính doanh thu chính.
- Layout GUI Win32 dùng tọa độ cố định, phù hợp cửa sổ desktop mặc định hơn là màn hình rất nhỏ.

## 24. Ghi chú xác minh

Trên Linux, đã build/chạy TUI và test backend bằng dữ liệu tạm. Windows GUI cần được build/chạy trên Windows hoặc MSYS2 UCRT64 để xác minh runtime thực tế.
