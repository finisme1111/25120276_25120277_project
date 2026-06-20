#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <cstddef>
#include <iostream>
#include <string>

#ifdef _WIN32
    #define NOMINMAX
    #include <conio.h>
    #include <windows.h>
#else
    #include <cerrno>
    #include <cstdio>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

namespace tui {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Escape,
    Backspace,
    DeleteKey,
    Home,
    End,
    PageUp,
    PageDown,
    Tab,
    Character,
    Unknown
};

struct KeyEvent {
    Key key;
    char character;

    KeyEvent(Key keyValue = Key::Unknown, char characterValue = '\0')
        : key(keyValue), character(characterValue) {}
};

struct TerminalSize {
    int rows;
    int columns;

    TerminalSize(int rowCount = 25, int columnCount = 80)
        : rows(rowCount), columns(columnCount) {}
};

enum class Color {
    Default = 39,
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
    BrightBlack = 90,
    BrightRed = 91,
    BrightGreen = 92,
    BrightYellow = 93,
    BrightBlue = 94,
    BrightMagenta = 95,
    BrightCyan = 96,
    BrightWhite = 97
};

namespace detail {

#ifdef _WIN32

inline void enableAnsiSupport() {
    static bool initialized = false;

    if (initialized) {
        return;
    }

    initialized = true;

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE || output == nullptr) {
        return;
    }

    DWORD mode = 0;

    if (!GetConsoleMode(output, &mode)) {
        return;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(output, mode);
}

#else

class RawModeGuard {
private:
    termios oldSettings;
    bool active;

public:
    RawModeGuard()
        : oldSettings{}, active(false) {
        if (!isatty(STDIN_FILENO)) {
            return;
        }

        if (tcgetattr(STDIN_FILENO, &oldSettings) != 0) {
            return;
        }

        termios rawSettings = oldSettings;

        rawSettings.c_lflag &= static_cast<tcflag_t>(
            ~(ICANON | ECHO)
        );

        rawSettings.c_iflag &= static_cast<tcflag_t>(
            ~(IXON | ICRNL)
        );

        rawSettings.c_cc[VMIN] = 1;
        rawSettings.c_cc[VTIME] = 0;

        if (tcsetattr(
                STDIN_FILENO,
                TCSAFLUSH,
                &rawSettings
            ) == 0) {
            active = true;
        }
    }

    ~RawModeGuard() {
        if (active) {
            tcsetattr(
                STDIN_FILENO,
                TCSAFLUSH,
                &oldSettings
            );
        }
    }

    RawModeGuard(const RawModeGuard&) = delete;
    RawModeGuard& operator=(const RawModeGuard&) = delete;
};

inline int readByte() {
    unsigned char value = 0;

    while (true) {
        const ssize_t result =
            ::read(STDIN_FILENO, &value, 1);

        if (result == 1) {
            return static_cast<int>(value);
        }

        if (result < 0 && errno == EINTR) {
            continue;
        }

        return EOF;
    }
}

inline bool byteAvailable(int timeoutMilliseconds) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeval timeout{};
    timeout.tv_sec = timeoutMilliseconds / 1000;
    timeout.tv_usec =
        (timeoutMilliseconds % 1000) * 1000;

    const int result = select(
        STDIN_FILENO + 1,
        &set,
        nullptr,
        nullptr,
        &timeout
    );

    return result > 0 &&
           FD_ISSET(STDIN_FILENO, &set);
}

#endif

} // namespace detail

inline KeyEvent readKey() {
#ifdef _WIN32
    const int first = _getch();

    if (first == 0 || first == 224) {
        const int second = _getch();

        switch (second) {
            case 72:
                return KeyEvent(Key::Up);
            case 80:
                return KeyEvent(Key::Down);
            case 75:
                return KeyEvent(Key::Left);
            case 77:
                return KeyEvent(Key::Right);
            case 71:
                return KeyEvent(Key::Home);
            case 79:
                return KeyEvent(Key::End);
            case 73:
                return KeyEvent(Key::PageUp);
            case 81:
                return KeyEvent(Key::PageDown);
            case 83:
                return KeyEvent(Key::DeleteKey);
            default:
                return KeyEvent(Key::Unknown);
        }
    }

    switch (first) {
        case 13:
            return KeyEvent(Key::Enter);
        case 27:
            return KeyEvent(Key::Escape);
        case 8:
            return KeyEvent(Key::Backspace);
        case 9:
            return KeyEvent(Key::Tab);
        default:
            if (first >= 32 && first <= 126) {
                return KeyEvent(
                    Key::Character,
                    static_cast<char>(first)
                );
            }

            return KeyEvent(Key::Unknown);
    }
#else
    detail::RawModeGuard rawMode;

    const int first = detail::readByte();

    if (first == EOF) {
        return KeyEvent(Key::Unknown);
    }

    if (first == '\n' || first == '\r') {
        return KeyEvent(Key::Enter);
    }

    if (first == 127 || first == 8) {
        return KeyEvent(Key::Backspace);
    }

    if (first == '\t') {
        return KeyEvent(Key::Tab);
    }

    if (first != 27) {
        if (first >= 32 && first <= 126) {
            return KeyEvent(
                Key::Character,
                static_cast<char>(first)
            );
        }

        return KeyEvent(Key::Unknown);
    }

    /*
     * ESC đứng riêng không có byte tiếp theo.
     * Chuỗi phím mũi tên thường là ESC [ A/B/C/D.
     */
    if (!detail::byteAvailable(40)) {
        return KeyEvent(Key::Escape);
    }

    const int second = detail::readByte();

    if (second != '[' && second != 'O') {
        return KeyEvent(Key::Escape);
    }

    if (!detail::byteAvailable(40)) {
        return KeyEvent(Key::Unknown);
    }

    const int third = detail::readByte();

    switch (third) {
        case 'A':
            return KeyEvent(Key::Up);
        case 'B':
            return KeyEvent(Key::Down);
        case 'C':
            return KeyEvent(Key::Right);
        case 'D':
            return KeyEvent(Key::Left);
        case 'H':
            return KeyEvent(Key::Home);
        case 'F':
            return KeyEvent(Key::End);
        case '1':
        case '3':
        case '4':
        case '5':
        case '6': {
            if (!detail::byteAvailable(40)) {
                return KeyEvent(Key::Unknown);
            }

            const int fourth = detail::readByte();

            if (fourth != '~') {
                return KeyEvent(Key::Unknown);
            }

            switch (third) {
                case '1':
                    return KeyEvent(Key::Home);
                case '3':
                    return KeyEvent(Key::DeleteKey);
                case '4':
                    return KeyEvent(Key::End);
                case '5':
                    return KeyEvent(Key::PageUp);
                case '6':
                    return KeyEvent(Key::PageDown);
                default:
                    return KeyEvent(Key::Unknown);
            }
        }
        default:
            return KeyEvent(Key::Unknown);
    }
#endif
}

inline void clearScreen() {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

inline void clearLine() {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << "\033[2K\r";
    std::cout.flush();
}

inline void moveCursor(int row, int column) {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    if (row < 1) {
        row = 1;
    }

    if (column < 1) {
        column = 1;
    }

    std::cout
        << "\033["
        << row
        << ';'
        << column
        << 'H';

    std::cout.flush();
}

inline void hideCursor() {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << "\033[?25l";
    std::cout.flush();
}

inline void showCursor() {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << "\033[?25h";
    std::cout.flush();
}

inline void setForeground(Color color) {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout
        << "\033["
        << static_cast<int>(color)
        << 'm';
}

inline void setBackground(Color color) {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    int code = static_cast<int>(color);

    if (code >= 30 && code <= 37) {
        code += 10;
    } else if (code >= 90 && code <= 97) {
        code += 10;
    } else {
        code = 49;
    }

    std::cout << "\033[" << code << 'm';
}

inline void resetStyle() {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << "\033[0m";
}

inline void setBold(bool enabled = true) {
#ifdef _WIN32
    detail::enableAnsiSupport();
#endif

    std::cout << (enabled ? "\033[1m" : "\033[22m");
}

inline TerminalSize getTerminalSize() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info{};
    const HANDLE output =
        GetStdHandle(STD_OUTPUT_HANDLE);

    if (
        output != INVALID_HANDLE_VALUE &&
        output != nullptr &&
        GetConsoleScreenBufferInfo(output, &info)
    ) {
        const int columns =
            info.srWindow.Right -
            info.srWindow.Left + 1;

        const int rows =
            info.srWindow.Bottom -
            info.srWindow.Top + 1;

        return TerminalSize(rows, columns);
    }
#else
    winsize size{};

    if (
        ioctl(
            STDOUT_FILENO,
            TIOCGWINSZ,
            &size
        ) == 0 &&
        size.ws_row > 0 &&
        size.ws_col > 0
    ) {
        return TerminalSize(
            static_cast<int>(size.ws_row),
            static_cast<int>(size.ws_col)
        );
    }
#endif

    return TerminalSize();
}

inline std::string readLine(
    const std::string& prompt = ""
) {
    showCursor();

    if (!prompt.empty()) {
        std::cout << prompt;
        std::cout.flush();
    }

    std::string input;
    std::getline(std::cin, input);

    return input;
}

inline void waitForEnter(
    const std::string& message =
        "Nhan Enter de tiep tuc..."
) {
    std::cout << message;
    std::cout.flush();

    while (readKey().key != Key::Enter) {
    }
}

class CursorGuard {
public:
    CursorGuard() {
        hideCursor();
    }

    ~CursorGuard() {
        resetStyle();
        showCursor();
    }

    CursorGuard(const CursorGuard&) = delete;
    CursorGuard& operator=(const CursorGuard&) = delete;
};

} // namespace tui

#endif // TERMINAL_HPP
