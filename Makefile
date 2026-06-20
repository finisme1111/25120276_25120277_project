# ============================================================
# Cross-platform Makefile
# Linux:  TUI + backend tests
# Windows/MSYS2 UCRT64: TUI + Win32 GUI + backend tests
#
# Common commands:
#   make
#   make tui
#   make gui
#   make run-tui
#   make run-gui
#   make test
#   make clean
#   make debug
#
# Override compiler when needed:
#   make CXX=C:/msys64/ucrt64/bin/g++.exe
# ============================================================

CXX ?= g++
CPPFLAGS ?= -I./app -I./lib
WARNFLAGS ?= -Wall -Wextra -Wpedantic
COMMON_CXXFLAGS ?= -std=c++17 -O2 $(WARNFLAGS) \
                   -finput-charset=UTF-8 -fexec-charset=UTF-8
DEBUG_CXXFLAGS := -std=c++17 -O0 -g3 $(WARNFLAGS) \
                  -finput-charset=UTF-8 -fexec-charset=UTF-8
EXTRA_CXXFLAGS ?=
EXTRA_LDFLAGS ?=

TUI_SRC := app/main_tui.cpp
GUI_SRC := app/main.cpp
TEST_SRC := tests/backend_tests.cpp
HEADERS := $(wildcard app/*.hpp) $(wildcard lib/*.hpp)

ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    EXEEXT := .exe
    GUI_LIBS := -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -mwindows
    ALL_TARGETS := parking_tui$(EXEEXT) parking_gui$(EXEEXT)
else
    PLATFORM := linux
    EXEEXT :=
    GUI_LIBS :=
    ALL_TARGETS := parking_tui
endif

TUI_TARGET := parking_tui$(EXEEXT)
GUI_TARGET := parking_gui$(EXEEXT)
TEST_TARGET := backend_tests$(EXEEXT)

.PHONY: all tui gui run run-tui run-gui test clean debug rebuild info

all: $(ALL_TARGETS)
	@echo "[OK] Build completed for $(PLATFORM)."

tui: $(TUI_TARGET)

$(TUI_TARGET): $(TUI_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(COMMON_CXXFLAGS) $(EXTRA_CXXFLAGS) \
		$(TUI_SRC) -o $@ $(EXTRA_LDFLAGS)
	@echo "[OK] Built TUI: $@"

ifeq ($(OS),Windows_NT)

gui: $(GUI_TARGET)

$(GUI_TARGET): $(GUI_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(COMMON_CXXFLAGS) -DUNICODE -D_UNICODE \
		$(EXTRA_CXXFLAGS) $(GUI_SRC) -o $@ \
		$(GUI_LIBS) $(EXTRA_LDFLAGS)
	@echo "[OK] Built GUI: $@"

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)

else

gui:
	@echo "[INFO] Win32 GUI can only be built on Windows/MSYS2."

run-gui:
	@echo "[INFO] Win32 GUI can only run on Windows."

endif

$(TEST_TARGET): $(TEST_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(COMMON_CXXFLAGS) $(EXTRA_CXXFLAGS) \
		$(TEST_SRC) -o $@ $(EXTRA_LDFLAGS)
	@echo "[OK] Built backend tests: $@"

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: run-tui

run-tui: $(TUI_TARGET)
	./$(TUI_TARGET)

debug: COMMON_CXXFLAGS = $(DEBUG_CXXFLAGS)
debug: clean all

rebuild: clean all

info:
	@echo "Platform : $(PLATFORM)"
	@echo "Compiler : $(CXX)"
	@echo "TUI      : $(TUI_TARGET)"
ifeq ($(OS),Windows_NT)
	@echo "GUI      : $(GUI_TARGET)"
else
	@echo "GUI      : unavailable on Linux"
endif
	@echo "Tests    : $(TEST_TARGET)"

ifeq ($(OS),Windows_NT)

clean:
	@cmd /C "if exist $(TUI_TARGET) del /Q $(TUI_TARGET)"
	@cmd /C "if exist $(GUI_TARGET) del /Q $(GUI_TARGET)"
	@cmd /C "if exist $(TEST_TARGET) del /Q $(TEST_TARGET)"
	@echo "[OK] Removed generated executables."

else

clean:
	rm -f $(TUI_TARGET) $(GUI_TARGET) $(TEST_TARGET)
	@echo "[OK] Removed generated executables."

endif
