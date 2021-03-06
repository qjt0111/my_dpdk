// Cuckoo Hash
// Author: Nicholas Sullivan

#include "cuckoo.h"

#define ACT_GET 0
#define ACT_EXIST 1
#define ACT_REMOVE 2
#define RESEED 1
#define NO_RESEED 0
#define MAP_F_HASH(hash, cap) (hash % (cap / 2))
#define MAP_S_HASH(hash, cap) ((hash % (cap / 2)) + (cap / 2))
#define F_IDX() MAP_F_HASH(                       \
    hash((void *)key, strlen(key), cuck->f_seed), \
    cuck->cap)
#define S_IDX() MAP_S_HASH(                       \
    hash((void *)key, strlen(key), cuck->s_seed), \
    cuck->cap)
#define MEM_ERR "cuckoo hash cannot allocate memory"
#define force_inline static __attribute__((always_inline))

force_inline void
gen_seeds(cuckoo *cuck)
{
    srand(time(NULL));
    do
    {
        cuck->f_seed = rand();
        cuck->s_seed = rand();
        cuck->f_seed = cuck->f_seed << (rand() % 63);
        cuck->s_seed = cuck->s_seed << (rand() % 63);
    } while (cuck->f_seed == cuck->s_seed);
}

force_inline void *
help(cuckoo *cuck, char *key, uint8_t act)
{
    uint32_t f_idx = F_IDX();
    uint32_t s_idx = S_IDX();
    uint32_t idx = -1;
    if (cuck->nodes[f_idx].taken && strcmp(key, cuck->nodes[f_idx].key) == 0)
    {
        idx = f_idx;
    }
    else if (cuck->nodes[s_idx].taken && strcmp(key, cuck->nodes[s_idx].key) == 0)
    {
        idx = s_idx;
    }
    if (idx != -1)
    {
        switch (act)
        {
        case ACT_EXIST:
            return (void *)1;
        case ACT_REMOVE:
            cuck->size--;
            cuck->nodes[idx].taken = 0;
            free(cuck->nodes[idx].key);
        case ACT_GET:
            return cuck->nodes[idx].val;
        }
    }
    return NULL;
}

force_inline void
grow(cuckoo *cuck, uint8_t reseed)
{
    uint32_t o_cap = cuck->cap;
    if (reseed == RESEED)
    {
        gen_seeds(cuck);
    }
    cuck->cap = cuck->cap * CUCKOO_RESIZE_MULT;
    cuck->size_lim = cuck->cap * CUCKOO_RESIZE_LIM / 100;
    cuck->nodes = realloc(cuck->nodes, sizeof(cuckoo_node) * cuck->cap);
    if (!cuck->nodes)
    {
        fprintf(stderr, MEM_ERR);
        exit(1);
    }
    for (int i = o_cap; i < cuck->cap; i++)
    {
        cuck->nodes[i].taken = 0;
    }
    for (int i = 0; i < o_cap; i++)
    {
        if (cuck->nodes[i].taken)
        {
            char *key = cuck->nodes[i].key;
            void *val = cuck->nodes[i].val;
            uint32_t f_idx = F_IDX();
            uint32_t s_idx = S_IDX();
            if (f_idx != i && s_idx != i)
            {
                cuck->nodes[i].taken = 0;
                cuck->size--;
                cuckoo_insert(cuck, key, val);
                free(key);
            }
        }
    }
}

static uint8_t
rehash(cuckoo *cuck, uint32_t idx, uint32_t dep)
{
    char *key = cuck->nodes[idx].key;
    uint32_t f_idx = F_IDX();
    uint32_t s_idx = S_IDX();
    uint32_t n_idx = s_idx;
    if (idx == s_idx)
    {
        n_idx = f_idx;
    }
    if (cuck->nodes[n_idx].taken)
    {
        if (dep == 0)
        {
            grow(cuck, RESEED);
            return 1;
        }
        else
        {
            if (rehash(cuck, n_idx, dep - 1))
            {
                return 1;
            }
        }
    }
    memcpy(&(cuck->nodes[n_idx]), &(cuck->nodes[idx]), sizeof(cuckoo_node));
    cuck->nodes[idx].taken = 0;
    return 0;
}

cuckoo *
cuckoo_init(uint32_t init_cap)
{
    cuckoo *ncuck = malloc(sizeof(cuckoo));
    if (!ncuck)
    {
        fprintf(stderr, MEM_ERR);
        exit(1);
    }
    if (init_cap % 2 != 0)
    {
        init_cap++;
    }
    ncuck->cap = init_cap;
    ncuck->size = 0;
    ncuck->size_lim = ncuck->cap * CUCKOO_RESIZE_LIM / 100;
    gen_seeds(ncuck);
    ncuck->nodes = calloc(init_cap, sizeof(cuckoo_node));
    if (!ncuck->nodes)
    {
        fprintf(stderr, MEM_ERR);
        exit(1);
    }
    return ncuck;
}

//
uint8_t
cuckoo_insert(cuckoo *cuck, char *key, void *val)
{
    if (cuckoo_exists(cuck, key))
    {
        return 1;
    }

    if (cuck->size > cuck->size_lim)
    {
        grow(cuck, NO_RESEED);
    }
    for (;;)
    {
        uint32_t f_idx = F_IDX();
        uint32_t s_idx = S_IDX();
        if (!cuck->nodes[f_idx].taken)
        {
            cuck->nodes[f_idx].key = malloc(strlen(key) + 1);
            strcpy(cuck->nodes[f_idx].key, key);
            cuck->nodes[f_idx].val = val;
            cuck->nodes[f_idx].taken = 1;
            cuck->size++;
            break;
        }
        else if (!cuck->nodes[s_idx].taken)
        {
            cuck->nodes[s_idx].key = malloc(strlen(key) + 1);
            strcpy(cuck->nodes[s_idx].key, key);
            cuck->nodes[s_idx].val = val;
            cuck->nodes[s_idx].taken = 1;
            cuck->size++;
            break;
        }
        rehash(cuck, f_idx, CUCKOO_MAX_DEP);
    }
    return 0;
}

uint8_t
cuckoo_exists(cuckoo *cuck, char *key)
{
    if (!help(cuck, key, ACT_EXIST))
    {
        return 0;
    }
    return 1;
}

void *
cuckoo_get(cuckoo *cuck, char *key)
{
    return help(cuck, key, ACT_GET);
}

void *
cuckoo_remove(cuckoo *cuck, char *key)
{
    return help(cuck, key, ACT_REMOVE);
}

void
cuckoo_destroy(cuckoo *cuck, uint8_t deep)
{
    for (int i = 0; i < cuck->cap; i++)
    {
        if (cuck->nodes[i].taken)
        {
            free(cuck->nodes[i].key);
            if (deep)
            {
                free(cuck->nodes[i].val);
            }
        }
    }
    free(cuck->nodes);
    free(cuck);
}

// int
// main()
// {
//     cuckoo *c = cuckoo_init(100);
//     cuckoo_insert(c, "123", (void *)123);
//     char **vals = malloc(sizeof(char *) * 1000000);
//     uint64_t *ins = malloc(sizeof(uint64_t) * 1000000);
//     char *keys = malloc(10 * 1000000);
//     uint32_t inserted = 0;
//     uint32_t not_inserted = 0;
//     for (int i = 0; i < 1000000; i++)
//     {
//         char *key = keys + (10 * i);
//         for (int i = 0; i < 9; i++)
//         {
//             char k = (rand() % 40) + 49;
//             key[i] = k;
//         }
//         key[9] = '\0';
//         vals[i] = malloc(10);
//         if (!cuckoo_insert(c, key, (vals[i])))
//         {
//             inserted++;
//             ins[i] = 1;
//         }
//         else
//         {
//             ins[i] = 0;
//             not_inserted++;
//         }
//     }
//     for (int i = 0; i < 1000000; i++)
//     {
//         char *key = keys + (10 * i);
//         void *val = cuckoo_get(c, key);
//         if (val != vals[i] && ins[i])
//         {
//             printf("%d :", i);
//             puts("VALS DO NOT MATCH");
//             break;
//         }
//     }
//     printf("Inserted: %u\n", inserted);
//     printf("Not-inserted: %u\n", not_inserted);
//     printf("Size: %u\n", c->size);
//     printf("Cap: %u\n", c->cap);
//     if ((uint64_t)cuckoo_get(c, "123") == 123)
//     {
//         puts("123 passed");
//     }
//     else
//     {
//         puts("123 failed");
//     }
//     if ((uint64_t)cuckoo_remove(c, "123") == 123)//移除
//     {
//         puts("remove test 1 pass");
//         if (!cuckoo_exists(c, "123"))//不存在了
//         {
//             puts("remove test 2 pass");
//             if (cuckoo_get(c, "123") == NULL)//检查
//             {
//                 puts("remove test 3 pass");
//                 if (c->size == inserted)//检查
//                 {
//                     puts("remove test 4 pass");
//                 }
//             }
//         }
//     }
//     free(keys);
//     free(ins);
//     free(vals);
//     cuckoo_destroy(c, 1);
//     return 0;
// }
