/* Tests for src/linked_list.c, included with main() renamed. */
#define main linked_list_demo_main
#include "../src/linked_list.c"
#undef main

static int failures = 0;
static void check(bool ok, const char *expr, int line)
{
    if (!ok) {
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, line, expr);
        failures++;
    }
}
#define CHECK(cond) check((cond), #cond, __LINE__)

#define MAX 64

/* Plain-array model of the list: priorities only. */
static int model[MAX];
static int model_len;

static bool matches_model(const struct task *t)
{
    for (int i = 0; i < model_len; i++, t = t->next)
        if (t == NULL || t->priority != model[i])
            return false;
    return t == NULL;
}

static unsigned long long rng_state = 88172645463325252ULL;
static unsigned long long next_rand(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void test_edges(void)
{
    struct task *head = NULL;
    CHECK(!remove_title(&head, "nothing"));
    reverse(&head);
    CHECK(head == NULL);

    push_back(&head, task_new("only", 1));
    CHECK(remove_title(&head, "only") && head == NULL);

    push_back(&head, task_new("a", 1));
    push_back(&head, task_new("b", 2));
    push_back(&head, task_new("c", 3));
    CHECK(remove_title(&head, "c"));              /* tail */
    CHECK(remove_title(&head, "a"));              /* head */
    CHECK(head != NULL && head->priority == 2 && head->next == NULL);
    free_list(&head);
    CHECK(head == NULL);

    struct task *t = task_new("this title is much longer than the buffer", 9);
    CHECK(strlen(t->title) == sizeof t->title - 1);   /* truncated, still terminated */
    free(t);
}

static void test_random_operations(void)
{
    struct task *head = NULL;
    char title[24];
    for (int step = 0; step < 20000; step++) {
        int prio = (int)(next_rand() % 1000);
        snprintf(title, sizeof title, "t%d", prio);
        switch (next_rand() % 4) {
        case 0:
            if (model_len < MAX) {
                push_front(&head, task_new(title, prio));
                for (int i = model_len; i > 0; i--) model[i] = model[i - 1];
                model[0] = prio;
                model_len++;
            }
            break;
        case 1:
            if (model_len < MAX) {
                push_back(&head, task_new(title, prio));
                model[model_len++] = prio;
            }
            break;
        case 2: {
            bool removed = remove_title(&head, title);
            int at = -1;
            for (int i = 0; i < model_len; i++)
                if (model[i] == prio) { at = i; break; }
            CHECK(removed == (at >= 0));
            if (at >= 0) {
                for (int i = at; i + 1 < model_len; i++) model[i] = model[i + 1];
                model_len--;
            }
            break;
        }
        default:
            reverse(&head);
            for (int i = 0; i < model_len / 2; i++) {
                int tmp = model[i];
                model[i] = model[model_len - 1 - i];
                model[model_len - 1 - i] = tmp;
            }
            break;
        }
        CHECK(matches_model(head));
    }
    free_list(&head);
}

int main(void)
{
    test_edges();
    test_random_operations();
    if (failures == 0)
        printf("test_linked_list: all checks passed\n");
    return failures != 0;
}
