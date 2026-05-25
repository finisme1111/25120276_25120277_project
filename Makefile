CXX      = C:/msys64/ucrt64/bin/g++.exe
CXXFLAGS = -std=c++17 -O2 -Wall -DUNICODE -D_UNICODE \
           -I./app -I./lib
LDFLAGS  = -lgdi32 -lcomctl32 -lcomdlg32 -mwindows -static-libgcc -static-libstdc++
TARGET   = parking_gui.exe
SRC      = app/main.cpp

all: $(TARGET)
	@echo.
	@echo [OK] Build thanh cong: $(TARGET)
	@echo [OK] Chay: $(TARGET)

$(TARGET): $(SRC) app/ParkingSystem.hpp app/ParkingMap.hpp app/models.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

clean:
	@if exist $(TARGET) del /Q $(TARGET)
	@echo [OK] Da xoa $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
