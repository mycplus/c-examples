#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct task {
    char title[24];
    int priority;
    struct task *next;
};

static struct task *task_new(const char *title, int priority)
{
    struct task *t = malloc(sizeof *t);
    if (t == NULL)
        return NULL;
    snprintf(t->title, sizeof t->title, "%s", title);
    t->priority = priority;
    t->next = NULL;
    return t;
}

static void push_front(struct task **head, struct task *t)
{
    t->next = *head;
    *head = t;
}

/* `link` always points at the pointer that must change: the head or some next. */
static void push_back(struct task **head, struct task *t)
{
    struct task **link = head;
    while (*link != NULL)
        link = &(*link)->next;
    *link = t;
}

static bool remove_title(struct task **head, const char *title)
{
    for (struct task **link = head; *link != NULL; link = &(*link)->next) {
        if (strcmp((*link)->title, title) == 0) {
            struct task *gone = *link;
            *link = gone->next;
            free(gone);
            return true;
        }
    }
    return false;
}

static void reverse(struct task **head)
{
    struct task *prev = NULL;
    while (*head != NULL) {
        struct task *next = (*head)->next;
        (*head)->next = prev;
        prev = *head;
        *head = next;
    }
    *head = prev;
}

static void free_list(struct task **head)
{
    while (*head != NULL) {
        struct task *next = (*head)->next;
        free(*head);
        *head = next;
    }
}

static void print_list(const struct task *t)
{
    for (; t != NULL; t = t->next)
        printf("  [%d] %s\n", t->priority, t->title);
}

int main(void)
{
    struct task *todo = NULL;

    push_back(&todo, task_new("Write report", 2));
    push_back(&todo, task_new("Email Sam", 3));
    push_front(&todo, task_new("Pay invoice", 1));
    printf("Initial list:\n");
    print_list(todo);

    remove_title(&todo, "Email Sam");
    printf("After removing \"Email Sam\":\n");
    print_list(todo);

    reverse(&todo);
    printf("Reversed:\n");
    print_list(todo);

    free_list(&todo);
    return 0;
}
