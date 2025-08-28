// src/actor.c
#include "leo/actor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================
   Internal structures
   ========================================================== */

struct leo_ActorSystem;

typedef struct leo_Actor
{
    struct leo_ActorSystem *sys;

    /* Tree links */
    struct leo_Actor *parent;
    struct leo_Actor *first_child;
    struct leo_Actor *last_child;
    struct leo_Actor *prev_sibling;
    struct leo_Actor *next_sibling;

    /* Ordering within siblings */
    int z;
    uint64_t seq; /* insertion order for stable sort within equal z */

    /* Identity */
    leo_ActorUID uid;
    const char *name; /* borrowed */

    /* Behavior & data */
    const leo_ActorVTable *vtable;
    void *user_data;

    /* Signals */
    leo_SignalEmitter emitter;

    /* State */
    leo_ActorGroupMask groups;
    unsigned paused : 1;
    unsigned dying : 1;
} leo_Actor;

/* Group registry: up to 64 named groups mapped to bit indices. */
typedef struct leo__GroupRegistry
{
    char *names[64]; /* strdup'd names or NULL; index = bit */
    int used;        /* number of defined groups */
} leo__GroupRegistry;

typedef struct leo_ActorSystem
{
    /* Root node lives inline; not killable by user. */
    leo_Actor root;
    int root_initialized;

    /* Global state */
    unsigned paused : 1;
    uint64_t next_uid;
    uint64_t next_seq;

    /* Live registry (for UID lookup and group scans) */
    leo_Actor **live;
    int live_count;
    int live_cap;

    /* Deferred kill list (unique) */
    leo_Actor **to_kill;
    int kill_count;
    int kill_cap;

    /* Groups */
    leo__GroupRegistry groups;
} leo_ActorSystem;

/* Common signal names (optional, defined per-actor on creation) */
static const char *LEO_SIG_SPAWNED = "spawned";
static const char *LEO_SIG_KILLED = "killed";
static const char *LEO_SIG_EXITING = "exiting";

/* ==========================================================
   Small utilities
   ========================================================== */

static void *leo__xmalloc(size_t n)
{
    void *p = malloc(n);
    return p ? p : NULL;
}

static void *leo__xcalloc(size_t n, size_t sz)
{
    void *p = calloc(n, sz);
    return p ? p : NULL;
}

static int leo__ensure_cap(void ***arrp, int *count, int *cap, int need_cap)
{
    (void)count;
    if (*cap >= need_cap)
        return 1;
    int newcap = (*cap == 0) ? 8 : (*cap * 2);
    if (newcap < need_cap)
        newcap = need_cap;
    void **np = (void **)realloc(*arrp, (size_t)newcap * sizeof(void *));
    if (!np)
        return 0;
    *arrp = np;
    *cap = newcap;
    return 1;
}

static void leo__live_add(leo_ActorSystem *sys, leo_Actor *a)
{
    if (!leo__ensure_cap((void ***)&sys->live, &sys->live_count, &sys->live_cap, sys->live_count + 1))
        return; /* best-effort; API doesn't expose OOM here */
    sys->live[sys->live_count++] = a;
}

static void leo__live_remove(leo_ActorSystem *sys, const leo_Actor *a)
{
    for (int i = 0; i < sys->live_count; ++i)
    {
        if (sys->live[i] == a)
        {
            sys->live[i] = sys->live[sys->live_count - 1];
            sys->live_count--;
            return;
        }
    }
}

static int leo__kill_push_unique(leo_ActorSystem *sys, leo_Actor *a)
{
    /* Ensure uniqueness */
    for (int i = 0; i < sys->kill_count; ++i)
        if (sys->to_kill[i] == a)
            return 1;
    if (!leo__ensure_cap((void ***)&sys->to_kill, &sys->kill_count, &sys->kill_cap, sys->kill_count + 1))
        return 0;
    sys->to_kill[sys->kill_count++] = a;
    return 1;
}

/* Sibling ordering: (z, seq) ascending */
static int leo__comes_before(const leo_Actor *a, const leo_Actor *b)
{
    if (a->z != b->z)
        return (a->z < b->z);
    return (a->seq < b->seq);
}

static void leo__link_child_sorted(leo_Actor *parent, leo_Actor *child)
{
    child->parent = parent;
    child->prev_sibling = child->next_sibling = NULL;

    leo_Actor *it = parent->first_child;
    leo_Actor *insert_before = NULL;
    while (it)
    {
        if (leo__comes_before(child, it))
        {
            insert_before = it;
            break;
        }
        it = it->next_sibling;
    }

    if (!insert_before)
    {
        /* append at end */
        child->prev_sibling = parent->last_child;
        if (parent->last_child)
            parent->last_child->next_sibling = child;
        parent->last_child = child;
        if (!parent->first_child)
            parent->first_child = child;
    }
    else
    {
        /* insert before 'insert_before' */
        child->next_sibling = insert_before;
        child->prev_sibling = insert_before->prev_sibling;
        if (insert_before->prev_sibling)
            insert_before->prev_sibling->next_sibling = child;
        insert_before->prev_sibling = child;
        if (parent->first_child == insert_before)
            parent->first_child = child;
    }
}

static void leo__unlink_child(leo_Actor *parent, leo_Actor *child)
{
    (void)parent;
    if (child->prev_sibling)
        child->prev_sibling->next_sibling = child->next_sibling;
    if (child->next_sibling)
        child->next_sibling->prev_sibling = child->prev_sibling;
    if (child->parent)
    {
        if (child->parent->first_child == child)
            child->parent->first_child = child->next_sibling;
        if (child->parent->last_child == child)
            child->parent->last_child = child->prev_sibling;
    }
    child->parent = NULL;
    child->prev_sibling = child->next_sibling = NULL;
}

/* Effective paused = system || any ancestor || self */
static int leo__is_effectively_paused(const leo_Actor *a)
{
    if (!a || !a->sys)
        return 0;
    if (a->sys->paused)
        return 1;
    const leo_Actor *it = a;
    while (it)
    {
        if (it->paused)
            return 1;
        it = it->parent;
    }
    return 0;
}

/* ==========================================================
   Group registry helpers
   ========================================================== */

static int leo__group_find_index(const leo__GroupRegistry *reg, const char *name)
{
    if (!name)
        return -1;
    for (int i = 0; i < 64; ++i)
    {
        if (reg->names[i] && strcmp(reg->names[i], name) == 0)
            return i;
    }
    return -1;
}

static int leo__group_get_or_create(leo__GroupRegistry *reg, const char *name)
{
    if (!name || !*name)
        return -1;
    int idx = leo__group_find_index(reg, name);
    if (idx >= 0)
        return idx;
    /* create new */
    for (int i = 0; i < 64; ++i)
    {
        if (!reg->names[i])
        {
            char *dup = (char *)leo__xmalloc(strlen(name) + 1);
            if (!dup)
                return -1;
            strcpy(dup, name);
            reg->names[i] = dup;
            reg->used++;
            return i;
        }
    }
    return -1; /* full */
}

/* ==========================================================
   Destruction (post-order)
   ========================================================== */

static void leo__destroy_actor_recursive(leo_Actor *a)
{
    if (!a)
        return;

    /* Destroy children first */
    leo_Actor *ch = a->first_child;
    while (ch)
    {
        leo_Actor *next = ch->next_sibling;
        leo__destroy_actor_recursive(ch);
        ch = next;
    }

    /* Exiting signal & on_exit */
    if (!a->dying)
    {
        /* In rare cases, root/system destroy calls this for live nodes; mark to avoid double signals */
        a->dying = 1;
    }
    leo_signal_define(&a->emitter, LEO_SIG_EXITING);
    leo_signal_emit(&a->emitter, LEO_SIG_EXITING);
    if (a->vtable && a->vtable->on_exit)
    {
        a->vtable->on_exit(a);
    }

    /* unlink from parent */
    if (a->parent)
    {
        leo__unlink_child(a->parent, a);
    }

    /* remove from system live list */
    if (a->sys)
    {
        leo__live_remove(a->sys, a);
    }

    /* free signals */
    leo_signal_emitter_free(&a->emitter);

    /* finally free actor memory, but never free root (allocated inline) */
    if (a != &a->sys->root)
    {
        free(a);
    }
    else
    {
        /* reset root links (so a second destroy is harmless) */
        a->first_child = a->last_child = NULL;
    }
}

/* Drain kill list at end of update */
static void leo__drain_kill_list(leo_ActorSystem *sys)
{
    for (int i = 0; i < sys->kill_count; ++i)
    {
        leo_Actor *a = sys->to_kill[i];
        if (!a)
            continue;

        /* Run "exiting" later; but emit "killed" now if first time */
        /* Already marked dying when enqueued */
        (void)leo_signal_define(&a->emitter, LEO_SIG_EXITING);

        /* Destroy recursively (post-order) */
        leo__destroy_actor_recursive(a);
    }
    sys->kill_count = 0;
}

/* ==========================================================
   Traversals
   ========================================================== */

static void leo__update_dfs(leo_Actor *a, float dt)
{
    if (!a)
        return;

    const int paused_effective = leo__is_effectively_paused(a);

    if (!paused_effective && a->vtable && a->vtable->on_update)
    {
        a->vtable->on_update(a, dt);
    }

    /* children (snapshot next before possible mutation) */
    leo_Actor *ch = a->first_child;
    while (ch)
    {
        leo_Actor *next = ch->next_sibling;
        leo__update_dfs(ch, dt);
        ch = next;
    }
}

static void leo__render_dfs(leo_Actor *a)
{
    if (!a)
        return;

    if (a->vtable && a->vtable->on_render)
    {
        a->vtable->on_render(a);
    }

    leo_Actor *ch = a->first_child;
    while (ch)
    {
        leo_Actor *next = ch->next_sibling;
        leo__render_dfs(ch);
        ch = next;
    }
}

/* ==========================================================
   Public API
   ========================================================== */

leo_ActorSystem *leo_actor_system_create(void)
{
    leo_ActorSystem *sys = (leo_ActorSystem *)leo__xcalloc(1, sizeof(*sys));
    if (!sys)
        return NULL;

    sys->paused = 0;
    sys->next_uid = 1; /* start at 1 so 0 can mean "invalid" */
    sys->next_seq = 1;

    /* Init root node inline */
    leo_Actor *r = &sys->root;
    memset(r, 0, sizeof(*r));
    r->sys = sys;
    r->uid = sys->next_uid++;
    r->name = "root";
    r->z = 0;
    r->seq = 0;
    r->vtable = NULL;
    r->groups = 0;
    r->paused = 0;
    r->dying = 0;
    leo_signal_emitter_init(&r->emitter, r);

    sys->root_initialized = 1;

    /* Add root to live list for uniform enumeration/UID find */
    leo__live_add(sys, r);

    return sys;
}

void leo_actor_system_destroy(leo_ActorSystem *sys)
{
    if (!sys)
        return;

    /* Kill entire tree (children of root) and drain */
    leo_Actor *ch = sys->root.first_child;
    while (ch)
    {
        leo_Actor *next = ch->next_sibling;
        leo__destroy_actor_recursive(ch);
        ch = next;
    }

    /* Root signals */
    leo_signal_emitter_free(&sys->root.emitter);

    /* Free group names */
    for (int i = 0; i < 64; ++i)
    {
        free(sys->groups.names[i]);
        sys->groups.names[i] = NULL;
    }

    free(sys->live);
    free(sys->to_kill);
    free(sys);
}

void leo_actor_system_update(leo_ActorSystem *sys, float dt)
{
    if (!sys)
        return;

    /* Traverse from root */
    leo__update_dfs(&sys->root, dt);

    /* Destroy killed actors post-update */
    leo__drain_kill_list(sys);
}

void leo_actor_system_render(leo_ActorSystem *sys)
{
    if (!sys)
        return;
    leo__render_dfs(&sys->root);
}

void leo_actor_system_set_paused(leo_ActorSystem *sys, bool paused)
{
    if (!sys)
        return;
    sys->paused = paused ? 1u : 0u;
}

bool leo_actor_system_is_paused(const leo_ActorSystem *sys)
{
    return sys ? (sys->paused != 0) : false;
}

leo_Actor *leo_actor_system_root(leo_ActorSystem *sys)
{
    return sys ? &sys->root : NULL;
}

leo_Actor *leo_actor_spawn(leo_Actor *parent, const leo_ActorDesc *desc)
{
    if (!parent || !parent->sys)
        return NULL;
    leo_ActorSystem *sys = parent->sys;

    leo_Actor *a = (leo_Actor *)leo__xcalloc(1, sizeof(*a));
    if (!a)
        return NULL;

    a->sys = sys;
    a->uid = sys->next_uid++;
    a->name = desc ? desc->name : NULL;
    a->vtable = desc ? desc->vtable : NULL;
    a->user_data = desc ? desc->user_data : NULL;
    a->groups = desc ? desc->groups : 0;
    a->paused = (desc && desc->start_paused) ? 1u : 0u;
    a->dying = 0;
    a->z = 0;
    a->seq = sys->next_seq++;

    leo_signal_emitter_init(&a->emitter, a);
    /* Predefine common signal names for convenience */
    leo_signal_define(&a->emitter, LEO_SIG_SPAWNED);
    leo_signal_define(&a->emitter, LEO_SIG_KILLED);
    leo_signal_define(&a->emitter, LEO_SIG_EXITING);

    /* Insert into tree with ordering */
    leo__link_child_sorted(parent, a);

    /* Add to live registry */
    leo__live_add(sys, a);

    /* Call on_init (if fails, kill immediately and return NULL) */
    if (a->vtable && a->vtable->on_init)
    {
        if (!a->vtable->on_init(a))
        {
            a->dying = 1;
            leo_signal_emit(&a->emitter, LEO_SIG_KILLED);
            (void)leo__kill_push_unique(sys, a);
            return NULL;
        }
    }

    /* Emit spawned after successful init */
    leo_signal_emit(&a->emitter, LEO_SIG_SPAWNED);
    return a;
}

void leo_actor_kill(leo_Actor *actor)
{
    if (!actor || !actor->sys)
        return;
    if (actor == &actor->sys->root)
        return; /* never kill root */

    if (!actor->dying)
    {
        actor->dying = 1;
        leo_signal_emit(&actor->emitter, LEO_SIG_KILLED);
        (void)leo__kill_push_unique(actor->sys, actor);
    }
}

/* ---------------- Identity & hierarchy ---------------- */

leo_ActorUID leo_actor_uid(const leo_Actor *a)
{
    return a ? a->uid : 0;
}
const char *leo_actor_name(const leo_Actor *a)
{
    return a ? a->name : NULL;
}
leo_Actor *leo_actor_parent(const leo_Actor *a)
{
    return a ? a->parent : NULL;
}

leo_Actor *leo_actor_find_child_by_name(const leo_Actor *parent, const char *name)
{
    if (!parent || !name)
        return NULL;
    leo_Actor *ch = parent->first_child;
    while (ch)
    {
        if (ch->name && strcmp(ch->name, name) == 0)
            return ch;
        ch = ch->next_sibling;
    }
    return NULL;
}

static leo_Actor *leo__find_recursive_by_name(leo_Actor *node, const char *name)
{
    if (!node)
        return NULL;
    for (leo_Actor *ch = node->first_child; ch; ch = ch->next_sibling)
    {
        if (ch->name && strcmp(ch->name, name) == 0)
            return ch;
        leo_Actor *r = leo__find_recursive_by_name(ch, name);
        if (r)
            return r;
    }
    return NULL;
}

leo_Actor *leo_actor_find_recursive_by_name(const leo_Actor *parent, const char *name)
{
    if (!parent || !name)
        return NULL;
    return leo__find_recursive_by_name((leo_Actor *)parent, name);
}

leo_Actor *leo_actor_find_by_uid(leo_ActorSystem *sys, leo_ActorUID uid)
{
    if (!sys || uid == 0)
        return NULL;
    for (int i = 0; i < sys->live_count; ++i)
    {
        if (sys->live[i] && sys->live[i]->uid == uid)
            return sys->live[i];
    }
    return NULL;
}

/* ---------------- Groups ---------------- */

int leo_actor_group_get_or_create(leo_ActorSystem *sys, const char *name)
{
    if (!sys)
        return -1;
    return leo__group_get_or_create(&sys->groups, name);
}

int leo_actor_group_find(const leo_ActorSystem *sys, const char *name)
{
    if (!sys)
        return -1;
    return leo__group_find_index(&sys->groups, name);
}

void leo_actor_add_to_group(leo_Actor *a, int group_bit)
{
    if (!a || group_bit < 0 || group_bit >= 64)
        return;
    a->groups |= (1ULL << (uint64_t)group_bit);
}

void leo_actor_remove_from_group(leo_Actor *a, int group_bit)
{
    if (!a || group_bit < 0 || group_bit >= 64)
        return;
    a->groups &= ~(1ULL << (uint64_t)group_bit);
}

bool leo_actor_in_group(const leo_Actor *a, int group_bit)
{
    if (!a || group_bit < 0 || group_bit >= 64)
        return false;
    return (a->groups & (1ULL << (uint64_t)group_bit)) != 0;
}

leo_ActorGroupMask leo_actor_groups(const leo_Actor *a)
{
    return a ? a->groups : 0;
}

void leo_actor_for_each_in_group(leo_ActorSystem *sys, int group_bit, leo_ActorEnumFn fn, void *user)
{
    if (!sys || !fn || group_bit < 0 || group_bit >= 64)
        return;
    uint64_t mask = (1ULL << (uint64_t)group_bit);
    for (int i = 0; i < sys->live_count; ++i)
    {
        leo_Actor *a = sys->live[i];
        if (!a)
            continue;
        if ((a->groups & mask) != 0 && !a->dying)
        {
            fn(a, user);
        }
    }
}

/* ---------------- Pause ---------------- */

void leo_actor_set_paused(leo_Actor *a, bool paused)
{
    if (!a)
        return;
    a->paused = paused ? 1u : 0u;
}

bool leo_actor_is_paused(const leo_Actor *a)
{
    return a ? (a->paused != 0) : false;
}

bool leo_actor_is_effectively_paused(const leo_Actor *a)
{
    return leo__is_effectively_paused(a) ? true : false;
}

/* ---------------- Signals ---------------- */

leo_SignalEmitter *leo_actor_emitter(leo_Actor *a)
{
    return a ? &a->emitter : NULL;
}
const leo_SignalEmitter *leo_actor_emitter_const(const leo_Actor *a)
{
    return a ? &a->emitter : NULL;
}

/* ---------------- User data ---------------- */

void *leo_actor_userdata(leo_Actor *a)
{
    return a ? a->user_data : NULL;
}
void leo_actor_set_userdata(leo_Actor *a, void *p)
{
    if (a)
        a->user_data = p;
}

/* ---------------- Utilities ---------------- */

void leo_actor_for_each_child(leo_Actor *parent, leo_ActorChildFn fn, void *user)
{
    if (!parent || !fn)
        return;
    leo_Actor *ch = parent->first_child;
    while (ch)
    {
        leo_Actor *next = ch->next_sibling;
        fn(ch, user);
        ch = next;
    }
}

void leo_actor_set_z(leo_Actor *a, int z)
{
    if (!a || !a->parent)
    {
        if (a)
            a->z = z;
        return;
    }
    if (a->z == z)
        return;
    a->z = z;
    /* Reorder within parent's list */
    leo_Actor *parent = a->parent;
    leo__unlink_child(parent, a);
    /* Keep seq as original to preserve stable tie-breaker */
    leo__link_child_sorted(parent, a);
}

int leo_actor_get_z(const leo_Actor *a)
{
    return a ? a->z : 0;
}
