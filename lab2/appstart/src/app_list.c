#include "app_list.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <dirent.h>

#include <sys/wait.h>
#include <signal.h>

int al_create(struct al_item **apps, const char *dir, const char *name, struct al_item *next, struct al_item *previous)
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
    (*apps)->instances = NULL;
    (*apps)->next = next;
    (*apps)->previous = previous;
    return EXIT_SUCCESS;
}

struct al_instance *al_create_instance(struct al_item *app)
{
    int stdout_link[2] = {-1, -1};
    int stderr_link[2] = {-1, -1};

    struct al_instance *prev = NULL;
    struct al_instance **cur = &app->instances;
    while (*cur != NULL)
    {
        prev = *cur;
        cur = &(*cur)->next;
    }

    *cur = malloc(sizeof(struct al_instance));
    if (*cur == NULL)
        goto err;
    (*cur)->app = app;
    (*cur)->next = NULL;
    (*cur)->previous = prev;

    if (pipe(stdout_link) != 0)
        goto err;

    if (pipe(stderr_link) != 0)
        goto err;

    (*cur)->pid = fork();
    if ((*cur)->pid == 0)
    {
        dup2(stdout_link[1], STDOUT_FILENO);
        close(stdout_link[0]);
        close(stdout_link[1]);
        dup2(stderr_link[1], STDERR_FILENO);
        close(stderr_link[0]);
        close(stderr_link[1]);

        /* execl will destroy the current process context. So it will only
         * return if something goes wrong. */
        execl(app->path, app->name, NULL);
        goto err;
    }
    else if ((*cur)->pid > 0)
    {
        close(stdout_link[1]);
        (*cur)->stdout = stdout_link[0];
        close(stderr_link[1]);
        (*cur)->stderr = stderr_link[0];

        return *cur;
    }
    else
    {
        goto err;
    }

err:
    if (*cur != NULL)
    {
        free(*cur);
        *cur = NULL;
    }

    if (stdout_link[0] > -1)
        close(stdout_link[0]);
    if (stdout_link[1] > -1)
        close(stdout_link[1]);
    if (stderr_link[0] > -1)
        close(stderr_link[0]);
    if (stderr_link[1] > -1)
        close(stderr_link[1]);
    return NULL;
}

void al_close_instance(struct al_instance *instance)
{
    kill(instance->pid, SIGTERM);
    waitpid(instance->pid, NULL, 0);

    if (instance->next != NULL)
    {
        instance->next->previous = instance->previous;
    }
    if (instance->previous != NULL)
    {
        instance->previous->next = instance->next;
    }
    else
    {
        if (instance == instance->app->instances)
        {
            instance->app->instances = instance->next;
        }
    }

    instance->next = NULL;
    instance->previous = NULL;

    if (instance->stdout > -1)
    {
        close(instance->stdout);
        instance->stdout = -1;
    }
    if (instance->stderr > -1)
    {
        close(instance->stderr);
        instance->stderr = -1;
    }

    free(instance);
}

void al_close_instances(struct al_item *app)
{
    struct al_instance *cur = app->instances;
    struct al_instance *next = NULL;
    while (cur != NULL)
    {
        next = cur->next;
        al_close_instance(cur);
        cur = next;
    }
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
            al_create(current, dir_path, dir->d_name, NULL, previous);
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
