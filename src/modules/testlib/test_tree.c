#include "string.h"
#include "stdio.h"
#include "test.h"
#include "collections.h"
#include <stdlib.h>
#include <string.h>

struct tree {
    struct tree_item titem;
    uint64_t index;
};


static struct tree* get(uint64_t i) {
    struct tree* o = calloc(1, sizeof(*o));
    o->index = i;
    return o;
}


/**
 * Gibt das Objekt zu einem tree_item zurueck
 */
static inline void* to_node(tree_t* tree, struct tree_item* item) {
    return ((char*) item - tree->tree_item_offset);
}

/**
 * Gibt das tree_item zu einem Objekt zurueck
 */
static inline struct tree_item* to_tree_item(tree_t* tree, void* node) {
    return (struct tree_item*) ((char*) node + tree->tree_item_offset);
}

/**
 * Gibt den Schluessel eines Objekts zurueck
 */
static inline uint64_t get_key(tree_t* tree, struct tree_item* item) {
    return *(uint64_t*)((char*) to_node(tree, item) + tree->sort_key_offset);
}

static void do_check2(tree_t* tree, struct tree_item* item, struct tree_item* parent, int* depth)
{
    int left, right;

    if (item->parent != parent) {
        printf("Oops, elternlink  stimmt nicht: %lld\n", (unsigned long long)
            get_key(tree, item));
        failed();
    }

    if (item->left) {
        do_check2(tree, item->left, item, &left);
    } else {
        left = 0;
    }

    if (item->right) {
        do_check2(tree, item->right, item, &right);
    } else {
        right = 0;
    }

    if ((right - left) != item->balance) {
        printf("Oops, Balance stimmt nicht: %lld [%d != %d]\n",
            (unsigned long long) get_key(tree, item), right - left,
            item->balance);
        failed();
    }

    *depth = 1 + (left > right ? left : right);
}

static void do_check(tree_t* tree, struct tree_item* item, struct tree_item* parent)
{
    int i;
    static int n = 1;

    if (item == NULL) {
        return;
    }

    printf("[%d] ", n++);
    do_check2(tree, item, parent, &i);
}

/**
 * Gibt ein Element und alle Kindelemente aus
 */
static void do_dump(tree_t* tree, struct tree_item* current)
{
    if (!current) {
        printf("-");
    } else if (!current->left && !current->right) {
        printf("%ld", get_key(tree, current));
    } else {
        printf("[%d] %ld (", current->balance, get_key(tree, current));
        do_dump(tree, current->left),
        printf(" ");
        do_dump(tree, current->right);
        printf(")");
    }
}

/**
 * Gibt einen Dump eines Baums auf stdout aus
 */
void tree_dump(tree_t* tree)
{
    do_dump(tree, tree->root);
    puts("");
}


struct node {
    uint64_t key;
    struct tree_item item;
};

void statistical_test(void)
{
    tree_t* t;
    struct node nodes[32];
    int i;
    int j;
    int n;
    FILE* old_stdout = stdout;

    stdout = NULL;
    for (n = 1; n <= 15; n++) {
        for (i = 1; i <= n; i++) {
            t = tree_create(struct node, item, key);
            for (j = 1; j <= n; j++) {
                struct node* node = nodes + j - 1;
                node->key = j;
                printf("insert %d\n", j);
                tree_insert(t, node);
            }
            printf("remove %d\n", nodes[i - 1].key);
            tree_remove(t, nodes + i - 1);

            do_check(t, t->root, NULL);
            tree_dump(t);
            tree_destroy(t);
            puts("\n\n");
        }
    }

    stdout = old_stdout;
}


void test_avl(void)
{
    tree_t* tree = tree_create(struct tree, titem, index);

    // Einfuegen in leeren Baum
    // Einfuegen:
    // - links/rechts ohne Rotation
    // - mit einfacher Rotation links/rechts (Balance 0/1)
    // - mit doppelter Rotation links/rechts
    // Loeschen:
    // - links/rechts ohne Rotation
    // - mit einfacher Rotation links/rechts (Balance 0/1)
    // - mit doppelter Rotation links/rechts

    printf("\n\nAVL-Baeume: ");

    tree_insert(tree, get(10));
    do_check(tree, tree->root, NULL);

    // Keine Rotation
    printf("\nOhne Rotation: ");
    tree_insert(tree, get(5));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(15));
    do_check(tree, tree->root, NULL);

    // Einfach links (Balance 1)
    printf("\nRotation links: ");
    tree_insert(tree, get(20));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(25));
    do_check(tree, tree->root, NULL);

    // Einfach links (Balance 0)
    // Durch Einfuegen nicht erreichbar

    // Einfach rechts (Balance -1)
    printf("\nRotation rechts: ");
    tree_insert(tree, get(2));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(1));
    do_check(tree, tree->root, NULL);

    // Einfach rechts (Balance 0)
    // Durch Einfuegen nicht erreichbar

    // Doppelt links-rechts
    printf("\nRotation LR: ");
    tree_insert(tree, get(14));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(17));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(18));
    do_check(tree, tree->root, NULL);

    // Doppelt rechts-links
    printf("\nRotation RL: ");
    tree_insert(tree, get(7));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(4));
    do_check(tree, tree->root, NULL);
    tree_insert(tree, get(3));
    do_check(tree, tree->root, NULL);


    // Einfach rechts (Balance 0)
    // Erst den oberen Knoten rausnehmen (nur rechtes Kind), dann das Blatt
    printf("\nEinfach rechts, Balance 0: ");
    tree_remove(tree, tree_search(tree, 5));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 7));
    do_check(tree, tree->root, NULL);

    // Einfach links (Balance 0)
    // Erst den oberen Knoten rausnehmen (nur linkes Kind), dann das Blatt
    printf("\nEinfach links, Balance 0: ");
    tree_remove(tree, tree_search(tree, 15));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 14));
    do_check(tree, tree->root, NULL);

    // Wurzel ersetzen
    // Hat links und rechts Kinder
    printf("\nKompliziert Loeschen: ");
    tree_remove(tree, tree_search(tree, 10));
    do_check(tree, tree->root, NULL);

    // Loeschen mit links und rechts Kinder und old_left = NULL
    tree_remove(tree, tree_search(tree, 20));
    do_check(tree, tree->root, NULL);

    // Loeschen mit links und rechts Kinder und item->left = subst
    tree_remove(tree, tree_search(tree, 18));
    do_check(tree, tree->root, NULL);
    tree_dump(tree);

    // Phantasielos den Rest wegmachen
    printf("\nBaum leeren: ");
    tree_remove(tree, tree_search(tree, 17));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 25));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 4));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 2));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 1));
    do_check(tree, tree->root, NULL);
    tree_remove(tree, tree_search(tree, 3));
    do_check(tree, tree->root, NULL);

    printf("\nErgebnisbaum: ");
    tree_dump(tree);

    tree_destroy(tree);
    printf("\n");

    statistical_test();
    printf("\n");
}
