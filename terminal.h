#include <ncurses.h>
#include <vector>
#include <string>

using namespace std;

class terminal
{
private:
    WINDOW *chatWin, *inputWin;
    vector<string> chat_history;
    int chat_scroll_offset = 0;

public:
    terminal() { initNcurses(); }
    ~terminal() { closeTerminal(); }

    void initNcurses()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

        int chat_height = LINES - 3;
        chatWin = newwin(chat_height, COLS, 0, 0);
        inputWin = newwin(3, COLS, chat_height, 0);

        keypad(chatWin, TRUE);
        keypad(inputWin, TRUE);
        scrollok(chatWin, TRUE);
        scrollok(inputWin, FALSE);

        box(inputWin, 0, 0);
        wrefresh(inputWin);
    }

    void redrawChat()
    {
        wclear(chatWin);
        box(chatWin, 0, 0);

        int chat_height, chat_width;
        getmaxyx(chatWin, chat_height, chat_width);

        int max_offset = max(0, (int)chat_history.size() - (chat_height - 2));
        chat_scroll_offset = min(chat_scroll_offset, max_offset);

        int start = max(0, (int)chat_history.size() - (chat_height - 2) - chat_scroll_offset);
        for (int i = 0; i < chat_height - 2 && start + i < (int)chat_history.size(); i++)
        {
            mvwprintw(chatWin, i + 1, 2, "%s", chat_history[start + i].c_str());
        }

        wrefresh(chatWin);
    }

    void consoleStatement(const string &msg)
    {
        chat_history.push_back(msg);
        redrawChat();
    }

    string getInput()
    {
        string input;
        int ch;
        int cursor_pos = 0;
        int max_width = COLS - 4;
        MEVENT event;

        while (true)
        {
            ch = wgetch(inputWin);

            if (ch == '\n')
            {
                break;
            }
            else if (ch == KEY_BACKSPACE || ch == 127)
            {
                if (cursor_pos > 0)
                {
                    input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                }
            }
            else if (ch == KEY_UP) // Scroll Up
            {
                if (chat_scroll_offset < (int)chat_history.size() - (LINES - 5))
                {
                    chat_scroll_offset++;
                    redrawChat();
                }
            }
            else if (ch == KEY_DOWN) // Scroll Down
            {
                if (chat_scroll_offset > 0)
                {
                    chat_scroll_offset--;
                    redrawChat();
                }
            }
            else if (ch == KEY_PPAGE) // Page Up (Go to the top)
            {
                chat_scroll_offset = (int)chat_history.size() - (LINES - 5);
                redrawChat();
            }
            else if (ch == KEY_NPAGE) // Page Down (Go to the bottom)
            {
                chat_scroll_offset = 0;
                redrawChat();
            }
            else if (ch == KEY_MOUSE)
            {
                if (getmouse(&event) == OK)
                {
                    if (event.bstate & BUTTON4_PRESSED) // Scroll Up
                    {
                        chat_scroll_offset = min(chat_scroll_offset + 2, (int)chat_history.size() - (LINES - 5));
                        redrawChat();
                    }
                    else if (event.bstate & BUTTON5_PRESSED) // Scroll Down
                    {
                        chat_scroll_offset = max(chat_scroll_offset - 2, 0);
                        redrawChat();
                    }
                }
                continue;
            }
            else if (ch == KEY_LEFT || ch == KEY_RIGHT || ch==KEY_HOME || ch==KEY_END)
            {
                // Ignore left and right arrow keys
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

    void closeTerminal()
    {
        wclear(chatWin);
        wrefresh(chatWin);
        delwin(chatWin);
        delwin(inputWin);
        endwin();
    }

};

