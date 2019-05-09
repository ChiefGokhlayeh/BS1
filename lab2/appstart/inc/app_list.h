#ifndef APP_LIST_H_
#define APP_LIST_H_

struct al_item
{
    char *path;
    char *name;
    struct al_item *next;
    struct al_item *previous;
};

/**
 * @brief Initialize an al_item with the given next and previous pointer.
 *
 * @param[out] apps
 * Pointer to an al_item start pointer. Will be allocated to a proper al_item on
 * successful completion.
 *
 * @param[in] dir
 * C-string of the base-dir where the app is located in. May be NULL for apps
 * that shall be looked up via PATH variable.
 *
 * @param[in] name
 * C-string of the app name. Must not be NULL.
 *
 * @param[in] next
 * Pointer to the next element to be set. May be NULL.
 *
 * @param[in] previous
 * Pointer to the previous element to be set. May be NULL.
 *
 * @return EXIT_SUCCESS on success, an error-code otherwise.
 */
int al_init(struct al_item **apps, const char *dir, const char *name, struct al_item *next, struct al_item *previous);

/**
 * @brief Initialize an al_item list by scanning through the contents of a
 * directory.
 *
 * @param[in] dir_path
 * Path to a directory to search through.
 *
 * @param[out] apps
 * Pointer to an al_item start pointer. Will be allocated to a proper al_item on
 * successful completion.
 *
 * @retval >= 0
 * Number of al_items created.
 *
 * @retval < 0
 * On error.
 */
int al_search(const char *dir_path, struct al_item **apps);

/**
 * @brief Follow the al_item list and return the al_item advanced by the given
 * page count.
 *
 * @param[in] apps
 * Item to start from.
 *
 * @param[in] page
 * Number of pages to advance. If page is a positive integer the list will be
 * advanced forward via the next pointer. If page is a negative integer list
 * will be advanced backwards via the previous pointer.
 *
 * @param[in] itemsPerPage
 * Number of items that fit on one page.
 *
 * @return Pointer to the element advanced by the requested number of pages or
 * the last element in the list.
 */
struct al_item *al_skip_pages(struct al_item *apps, int page, int itemsPerPage);

/**
 * @brief Display one page of al_item objects.
 *
 * @param[in] apps
 * Item to start from.
 *
 * @param[in] itemsPerPage
 * Number of items that fit on one page.
 *
 * @param[out] displayed
 * Will be set to the number of items that were actually displayed. The initial
 * value will be used to start of the item index.
 *
 * @param[in] print
 * Callback to be called for printing.
 *
 * @return Pointer to the first element of the following page or NULL if
 * end-of-list is reached.
 */
struct al_item *al_display_page(struct al_item *apps, int itemsPerPage, int *displayed, void (*print)(struct al_item *apps, int index));

/**
 * @brief Return the item a the given index or NULL if end-of-list is reached.
 *
 * @param[in] apps
 * Item to start from.
 *
 * @param[in] index
 * Index of item to return. If positive the next attribute will be used. If
 * negative the previous attribute will be used.
 *
 * @return Item at index or NULL.
 */
struct al_item *al_at(struct al_item *apps, int index);

#endif
