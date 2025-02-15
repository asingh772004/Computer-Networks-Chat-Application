// Standard C++ Libraries
#include <iostream>  // For standard I/O operations
#include <vector>    // For std::vector (if needed)
#include <algorithm> // For std::find, std::remove, etc. (if needed)
#include <string>    // For std::string
#include <cstring>   // For memset(), strcpy(), etc.

// Terminal UI (ncurses)
#include <ncurses.h> // For ncurses-based UI

using namespace std;

class terminal
{
public:
    WINDOW *chatWin;
    WINDOW *inputWin;
    pthread_mutex_t consoleLock = PTHREAD_MUTEX_INITIALIZER;
    vector<string> chat_history; // Store chat messages for scrolling

    void initNcurses()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        mousemask(ALL_MOUSE_EVENTS, NULL);

        int chat_height = LINES - 3;
        chatWin = newwin(chat_height, COLS, 0, 0);
        inputWin = newwin(3, COLS, chat_height, 0);

        scrollok(chatWin, TRUE);
        scrollok(inputWin, FALSE);
        box(inputWin, 0, 0);
        wrefresh(inputWin);
    }

    void closeTerminal()
    {
        int chat_height, chat_width;
        getmaxyx(chatWin, chat_height, chat_width);
        std::vector<std::string> chat_history;
        for (int i = 0; i < chat_height; i++)
        {
            char line[chat_width + 1];
            mvwinnstr(chatWin, i, 0, line, chat_width);
            line[chat_width] = '\0';
            chat_history.push_back(std::string(line));
        }
        wclear(inputWin);
        wrefresh(inputWin);
        delwin(inputWin);
        inputWin = nullptr;
        wresize(chatWin, LINES, COLS);
        mvwin(chatWin, 0, 0);
        wrefresh(chatWin);
        refresh();
        endwin();
        for (const auto &line : chat_history)
        {
            if (!line.empty() && line.find_first_not_of(' ') != std::string::npos)
            {
                std::cout << line << std::endl;
            }
        }
        std::cout << "\033[" << chat_height << "B";
        system("stty sane");
    }

    void consoleStatement(const string &message)
    {
        pthread_mutex_lock(&consoleLock);
        chat_history.push_back(message); // Store message in history
        wprintw(chatWin, "%s\n", message.c_str());
        wrefresh(chatWin);
        pthread_mutex_unlock(&consoleLock);
    }

    string getInput()
    {
        string input;
        int ch;
        int cursor_pos = 0;
        int max_width = COLS - 4;
        MEVENT event;
        int chat_scroll_offset = 0;

        while (true)
        {
            ch = wgetch(inputWin);

            if (ch == '\n')
            {
                break;
            }
            if (ch == KEY_BACKSPACE || ch == 127)
            {
                if (cursor_pos > 0)
                {
                    input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                }
            }
            else if (ch == KEY_LEFT)
            {
                if (cursor_pos > 0)
                {
                    cursor_pos--;
                }
            }
            else if (ch == KEY_RIGHT)
            {
                if (cursor_pos < input.length())
                {
                    cursor_pos++;
                }
            }
            else if (ch == KEY_HOME)
            {
                cursor_pos = 0;
            }
            else if (ch == KEY_END)
            {
                cursor_pos = input.length();
            }
            else if (ch == KEY_UP || ch == KEY_DOWN)
            {
                continue; // Ignore arrow keys for input
            }
            else if (ch == KEY_MOUSE)
            {
                if (getmouse(&event) == OK)
                {
                    if (event.bstate & (BUTTON1_PRESSED | BUTTON2_PRESSED | BUTTON3_PRESSED))
                    {
                        continue; // Ignore mouse clicks
                    }

                    int chat_height, chat_width;
                    getmaxyx(chatWin, chat_height, chat_width);

                    if (event.bstate & BUTTON4_PRESSED) // Scroll Up
                    {
                        if (chat_scroll_offset > 0)
                        {
                            chat_scroll_offset--;
                        }
                    }
                    else if (event.bstate & BUTTON5_PRESSED) // Scroll Down
                    {
                        if (chat_scroll_offset < (int)chat_history.size() - chat_height)
                        {
                            chat_scroll_offset++;
                        }
                    }

                    // Redraw the chat window with the updated scroll offset
                    wclear(chatWin);
                    for (int i = 0; i < chat_height; i++)
                    {
                        int line_idx = i + chat_scroll_offset;
                        if (line_idx >= 0 && line_idx < (int)chat_history.size())
                        {
                            mvwprintw(chatWin, i, 0, "%s", chat_history[line_idx].c_str());
                        }
                    }
                    wrefresh(chatWin);
                }
                continue;
            }
            else if (ch != ERR && input.length() < max_width)
            {
                input.insert(cursor_pos, 1, ch);
                cursor_pos++;
            }

            wclear(inputWin);
            box(inputWin, 0, 0);
            mvwprintw(inputWin, 1, 2, "%s", input.c_str());
            wmove(inputWin, 1, cursor_pos + 2);
            wrefresh(inputWin);
        }

        wclear(inputWin);
        box(inputWin, 0, 0);
        wrefresh(inputWin);
        consoleStatement("You: " + input);
        return input;
    }
};