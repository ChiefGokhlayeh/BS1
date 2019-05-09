#include "app_list.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <dirent.h>

int al_init(struct al_item **apps, const char *dir, const char *name, struct al_item *next, struct al_item *previous)
{
    int dirlen = 0;
    if (dir != NULL)
        dirlen = strlen(dir);

    *apps = malloc(sizeof(struct al_item));
    if (*apps == NULL)
        return EXIT_FAILURE;
    char *path = malloc(dirlen + 1 + strlen(name) + 1);
    if (path == NULL)
        return EXIT_FAILURE;

    char *_name = path;
    if (dir != NULL)
    {
        _name = path + dirlen + 1;
        strcat(strcat(strcpy(path, dir), "/"), name);
    }

    (*apps)->path = path;
    (*apps)->name = _name;
    (*apps)->next = next;
    (*apps)->previous = previous;
    return EXIT_SUCCESS;
}

int al_search(const char *dir_path, struct al_item **apps)
{
    int ret = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(dir_path);
    if (d)
    {
        struct al_item **current = apps;
        struct al_item *previous = NULL;
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            {
                continue;
            }
            al_init(current, dir_path, dir->d_name, NULL, previous);
            previous = *current;
            current = &(*current)->next;
            ret++;
        }
        closedir(d);
    }
    else
    {
        ret = EXIT_FAILURE;
    }
    return ret;
}

struct al_item *al_skip_pages(struct al_item *apps, int page, int itemsPerPage)
{
    struct al_item *cur = apps;
    int _page = abs(page);
    struct al_item *advance = NULL;
    for (int i = 0; i < _page && cur != NULL; i++)
    {
        int _itemsPerPage = itemsPerPage;

        do
        {
            if (page > 0)
                advance = cur->next;
            else
                advance = cur->previous;

            if (_itemsPerPage > 0)
                _itemsPerPage--;
            else if (_itemsPerPage == 0 || advance == NULL)
                break;
            cur = advance;
        } while (advance != NULL);
    }
    return cur;
}

struct al_item *al_display_page(struct al_item *apps, int itemsPerPage, int *displayed, void (*print)(struct al_item *apps, int index))
{
    struct al_item *cur = apps;
    int _displayed = 0;
    if (displayed != NULL)
        _displayed = *displayed;
    while (cur != NULL)
    {
        if (itemsPerPage > 0)
            itemsPerPage--;
        else if (itemsPerPage == 0)
            break;
        print(cur, _displayed);
        cur = cur->next;
        _displayed++;
    }
    if (displayed != NULL)
        *displayed = _displayed;
    return cur;
}

struct al_item *al_at(struct al_item *apps, int index)
{
    struct al_item *cur = apps;
    int _index = abs(index);
    for (int i = 0; i < _index && cur != NULL; i++)
        if (index > 0)
            cur = cur->next;
        else
            cur = cur->previous;
    return cur;
}
