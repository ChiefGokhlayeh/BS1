#include "app_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <unistd.h>

#include <ncurses.h>

#define STDIN_BUFFER_SIZE (1024)
#define ITEMS_PER_PAGE (10)

WINDOW *win = NULL;

static struct al_item *root;
static struct al_item *cur = NULL;
static struct al_item *next = NULL;
static int page = 0;
static int max_pages = 0;

static void quit()
{
    endwin();
}

static void print_item(struct al_item *item, int index)
{
    mvprintw(index, 0, "[%02d] %s\n", index + 1, item->name);
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
        printw("No next page.\n");
        return EXIT_FAILURE;
    }
}

static int previous_page(void)
{
    struct al_item *tmp = al_skip_pages(cur, -1, ITEMS_PER_PAGE);
    if (tmp != NULL)
    {
        cur = tmp;
        page--;
        return EXIT_SUCCESS;
    }
    else
    {
        printw("No previous page.\n");
        return EXIT_FAILURE;
    }
}

static int page_at(int at)
{
    struct al_item *tmp = al_skip_pages(root, at, ITEMS_PER_PAGE);
    if (tmp != NULL)
    {
        cur = tmp;
        page = at;
        return EXIT_SUCCESS;
    }
    else
    {
        printw("Page does not exist.\n");
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
        printw("pid: %d, stdout: %d, stderr: %d ", instance->pid, instance->stdout, instance->stderr);
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
                printw("Invalid page index!\n");
                return EXIT_FAILURE;
            }
        case 'q':
            quit();
            exit(0);
            return EXIT_SUCCESS;
        case 'c':
            if (1 <= (parsed = sscanf(buffer + i + 1, "%d %d", &index, &pid) && index > 0))
            {
                struct al_item *sel_item = al_at(cur, index - 1);
                if (parsed >= 2)
                {
                    if (pid > 1 && close_instance(sel_item, pid) == EXIT_SUCCESS)
                    {
                        printw("[%u] %s closed!\n", index, sel_item->name);
                    }
                    else
                    {
                        printw("Invalid pid!\n");
                        return EXIT_FAILURE;
                    }
                }
                else
                {
                    close_instances(sel_item);
                }
            }
            else
            {
                printw("Invalid item!\n");
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        case ' ':
        case '\t':
        case '\n':
            continue;
        default:
            printw("Invalid command!\n");
            return EXIT_FAILURE;
        }
    }
    printw("No command provided!\n");
    return EXIT_FAILURE;
}

int main(void)
{
    win = initscr();
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

    char buffer[STDIN_BUFFER_SIZE];

    int count = al_search("/usr/bin", &root);
    cur = root;
    max_pages = count / ITEMS_PER_PAGE;

    while (1)
    {
        refresh();
        int displayed = 0;
        next = al_display_page(cur, ITEMS_PER_PAGE, &displayed, print_item);
        attron(COLOR_PAIR(3));
        printw("Page: %d/%d\n", page + 1, max_pages);
        printw("Commands: [item num], (n)ext page, (p)rev page, #[page num], c[item num] [pid], (q)uit\n");
        attroff(COLOR_PAIR(3));

        attron(COLOR_PAIR(4));
        addstr("> ");
        clrtoeol();
        if (ERR != getnstr(buffer, STDIN_BUFFER_SIZE))
        {
            attroff(COLOR_PAIR(4));
            unsigned int selection = 0;
            if (1 == sscanf(buffer, "%u", &selection) && selection <= ITEMS_PER_PAGE)
            {
                struct al_item *sel_item = al_at(cur, selection - 1);
                status("Starting: [%u] %s ... ", selection, sel_item->name);
                if (EXIT_SUCCESS == start_instance(sel_item))
                {
                    printw("done!\n");
                }
            }
            else
            {
                parse_command(buffer, STDIN_BUFFER_SIZE);
            }
        }
        else
        {
            attroff(COLOR_PAIR(4));
            printw("Error while reading input!\n");
        }
        clrtobot();
    }

    return EXIT_SUCCESS;
}
