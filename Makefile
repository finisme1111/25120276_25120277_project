ifeq ($(OS),Windows_NT)

CXX = C:/msys64/ucrt64/bin/g++.exe
TARGET = parking_gui.exe
SRC = app/main.cpp
LDFLAGS = -lgdi32 -lcomctl32 -lcomdlg32 -mwindows

else

CXX = g++
TARGET = parking_tui
SRC = app/main_tui.cpp
LDFLAGS =

endif

CXXFLAGS = -std=c++17 -O2 -Wall -I./app -I./lib

all:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)