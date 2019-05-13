#include "app_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <unistd.h>

#include <ncurses.h>

#define STDIN_BUFFER_SIZE (1024)
#define DEFAULT_ITEMS_PER_PAGE (10)
#define STATUS_OPTION_APPEND (1 << 0)

static void quit();
static int clear_status(void);
static void print_item(struct al_item *item, int index);
static int next_page(void);
static int previous_page(void);
static int page_at(int at);
static int close_instances(struct al_item *app);
static int close_instance(struct al_item *app, pid_t pid);
static int start_instance(struct al_item *app);
static int parse_command(const char buffer[], size_t length);

WINDOW *init_win = NULL;
WINDOW *main_win = NULL;
WINDOW *status_win = NULL;

static struct al_item *root;
static struct al_item *cur = NULL;
static struct al_item *next = NULL;
static int page = 0;
static int max_pages = 0;

static void quit()
{
    struct al_item *cur = root;
    while (cur != NULL)
    {
        al_dispose(cur);
        cur = cur->next;
    }

    if (status_win != NULL)
    {
        delwin(status_win);
        status_win = NULL;
    }

    if (main_win != NULL)
    {
        delwin(main_win);
        main_win = NULL;
    }

    endwin();
}

static int clear_status(void)
{
    wclear(status_win);
    wmove(status_win, 0, 0);
}

static void status(const char *fmt, int options, ...)
{
    if (!(options & STATUS_OPTION_APPEND))
    {
        clear_status();
    }
    wattron(status_win, COLOR_PAIR(5));
    va_list args;
    va_start(args, options);
    vw_printw(status_win, fmt, args);
    va_end(args);
    wattroff(status_win, COLOR_PAIR(5));
    wrefresh(status_win);
}

static void print_item(struct al_item *item, int index)
{
    int dpos = 0;
    int x = 0;
    wattron(main_win, COLOR_PAIR(1));
    mvwprintw(main_win, index, x, "[%02d] %n", index + 1, &dpos);
    x += dpos;
    wattroff(main_win, COLOR_PAIR(1));
    mvwprintw(main_win, index, x, "%s %n", item->name, &dpos);
    x += dpos;

    wattron(main_win, COLOR_PAIR(2));
    struct al_instance *cur = item->instances;
    while (cur != NULL)
    {
        mvwprintw(main_win, index, x, "(%d)%n", cur->pid, &dpos);
        x += dpos;
        if ((cur = cur->next) != NULL)
        {
            mvwprintw(main_win, index, x, ", %n", &dpos);
            x += dpos;
        }
    }
    wattroff(main_win, COLOR_PAIR(2));
    mvwprintw(main_win, index, x, "\n");
}

static int next_page(void)
{
    if (next != NULL)
    {
        cur = next;
        page++;
        return EXIT_SUCCESS;
    }
    else
    {
        status("No next page.\n", 0);
        return EXIT_FAILURE;
    }
}

static int previous_page(void)
{
    struct al_item *tmp = al_skip_pages(cur, -1, DEFAULT_ITEMS_PER_PAGE);
    if (tmp != NULL)
    {
        cur = tmp;
        page--;
        return EXIT_SUCCESS;
    }
    else
    {
        status("No previous page.\n", 0);
        return EXIT_FAILURE;
    }
}

static int page_at(int at)
{
    struct al_item *tmp = al_skip_pages(root, at, DEFAULT_ITEMS_PER_PAGE);
    if (tmp != NULL)
    {
        cur = tmp;
        page = at;
        return EXIT_SUCCESS;
    }
    else
    {
        status("Page does not exist.\n", 0);
        return EXIT_FAILURE;
    }
}

static int close_instances(struct al_item *app)
{
    al_close_instances(app);
    return EXIT_SUCCESS;
}

static int close_instance(struct al_item *app, pid_t pid)
{
    struct al_instance *cur = app->instances;
    struct al_instance *found = NULL;
    while (cur != NULL)
    {
        if (cur->pid == pid)
            found = cur;
        cur = cur->next;
    }
    if (found != NULL)
    {
        al_close_instance(found);
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

static int start_instance(struct al_item *app)
{
    struct al_instance *instance = al_create_instance(app);
    if (instance != NULL)
    {
        status("pid: %d, stdout: %d, stderr: %d ", STATUS_OPTION_APPEND, instance->pid, instance->stdout, instance->stderr);
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

static int parse_command(const char buffer[], size_t length)
{
    int input_page = 0;
    int index = 0;
    int pid = 0;
    int parsed = 0;
    for (size_t i = 0; i < length; i++)
    {
        switch (buffer[i])
        {
        case 'n':
            return next_page();
        case 'p':
            return previous_page();
        case '#':
            if (1 == sscanf(buffer + i + 1, "%u", &input_page) && input_page <= max_pages && input_page > 0)
            {
                return page_at(input_page - 1);
            }
            else
            {
                status("Invalid page index!\n", 0);
                return EXIT_FAILURE;
            }
        case 'q':
            quit();
            exit(0);
            return EXIT_SUCCESS;
        case 'c':
            if (1 <= (parsed = sscanf(buffer + i + 1, "%d %d", &index, &pid)) && index > 0 && index <= DEFAULT_ITEMS_PER_PAGE)
            {
                struct al_item *sel_item = al_at(cur, index - 1);
                if (parsed >= 2)
                {
                    if (pid > 1 && close_instance(sel_item, pid) == EXIT_SUCCESS)
                    {
                        status("[%u] %s (%d) closed!\n", 0, index, sel_item->name, pid);
                        return EXIT_SUCCESS;
                    }
                    else
                    {
                        status("Invalid pid!\n", 0);
                        return EXIT_FAILURE;
                    }
                }
                else
                {
                    close_instances(sel_item);
                    status("[%u] %s closed!\n", 0, index, sel_item->name);
                    return EXIT_SUCCESS;
                }
            }
            else
            {
                status("Invalid item!\n", 0);
                return EXIT_FAILURE;
            }
        case ' ':
        case '\t':
        case '\n':
            continue;
        default:
            status("Invalid command!\n", 0);
            return EXIT_FAILURE;
        }
    }
    status("No command provided!\n", 0);
    return EXIT_FAILURE;
}

int main(void)
{
    init_win = initscr();
    atexit(quit);

    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
    }

    main_win = newwin(13, COLS, 0, 0);
    status_win = newwin(1, COLS, getmaxy(init_win) - 1, 0);

    char buffer[STDIN_BUFFER_SIZE];

    int count = al_search("/usr/bin", &root);
    cur = root;
    max_pages = count / DEFAULT_ITEMS_PER_PAGE + (count % DEFAULT_ITEMS_PER_PAGE == 0 ? 0 : 1);

    while (1)
    {
        int displayed = 0;
        next = al_display_page(cur, DEFAULT_ITEMS_PER_PAGE, &displayed, print_item);
        wattron(main_win, COLOR_PAIR(3));
        wprintw(main_win, "Page: %d/%d\n", page + 1, max_pages);
        wprintw(main_win, "Commands: [item num], (n)ext page, (p)rev page, #[page num], c[item num] [pid], (q)uit\n");
        wattroff(main_win, COLOR_PAIR(3));

        wattron(main_win, COLOR_PAIR(4));
        wprintw(main_win, "> ");
        wclrtobot(main_win);

        wrefresh(main_win);
        int err = wgetnstr(main_win, buffer, STDIN_BUFFER_SIZE);
        if (OK == err)
        {
            wattroff(main_win, COLOR_PAIR(4));
            unsigned int selection = 0;
            if (1 == sscanf(buffer, "%u", &selection) && selection <= DEFAULT_ITEMS_PER_PAGE)
            {
                struct al_item *sel_item = al_at(cur, selection - 1);
                status("Starting: [%u] %s ... ", 0, selection, sel_item->name);
                if (EXIT_SUCCESS == start_instance(sel_item))
                {
                    status("done!\n", STATUS_OPTION_APPEND);
                }
            }
            else
            {
                parse_command(buffer, STDIN_BUFFER_SIZE);
            }
        }
        else if (KEY_RESIZE)
        {
            mvwin(status_win, getmaxy(init_win) - 1, 0);
        }
        else
        {
            wattroff(main_win, COLOR_PAIR(4));
            status("Error while reading input!\n", 0);
        }
    }

    return EXIT_SUCCESS;
}
