/* ==========================================================
 * File: include/leo/actor.h
 * Lightweight actor tree for Leo (no ECS)
 * - Actors update/render each frame and can own child actors.
 * - Uses existing leo/signal.h for events.
 * - Safe lifetime: leo_actor_kill() marks for destruction and
 *   the system frees at the end of update(). No unsafe frees.
 * ========================================================== */
#pragma once

#include "leo/export.h"
#include "leo/signal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ----------------------------- */
    /* Types & constants             */
    /* ----------------------------- */

    typedef struct leo_ActorSystem leo_ActorSystem;
    typedef struct leo_Actor leo_Actor;

    typedef uint64_t leo_ActorUID;

    /* Up to 64 groups per system, addressed by a bit index [0..63].
       Names are mapped to bits at runtime (first-come first-served). */
    typedef uint64_t leo_ActorGroupMask;

    /* Actor lifecycle callbacks.
       All are optional; null means no-op. */
    typedef struct leo_ActorVTable
    {
        /* Called once after creation (parent set, name/groups applied).
           Return false to cancel creation (actor will be killed immediately). */
        bool (*on_init)(leo_Actor *self);

        /* Called every frame if not effectively paused. */
        void (*on_update)(leo_Actor *self, float dt);

        /* Called every frame (even if paused, unless you check paused yourself).
           Typical usage: draw only when not paused or draw paused differently. */
        void (*on_render)(leo_Actor *self);

        /* Called exactly once before destruction (children already killed). */
        void (*on_exit)(leo_Actor *self);
    } leo_ActorVTable;

    /* Spawn-time description (immutable after spawn). */
    typedef struct leo_ActorDesc
    {
        const char *name;              /* optional, borrowed */
        const leo_ActorVTable *vtable; /* optional (all nullptr ok) */
        void *user_data;               /* owned by caller; you manage lifetime */
        leo_ActorGroupMask groups;     /* initial membership (bitmask) */
        bool start_paused;             /* start in paused state */
    } leo_ActorDesc;

    /* ----------------------------- */
    /* System lifecycle              */
    /* ----------------------------- */

    LEO_API leo_ActorSystem *leo_actor_system_create(void);
    /* Destroys the whole actor tree (runs on_exit for remaining actors). */
    LEO_API void leo_actor_system_destroy(leo_ActorSystem *sys);

    /* Advance one frame:
       - walks the tree depth-first
       - skips on_update for effectively paused actors
       - queues destruction for actors killed during the tick
       - executes on_exit and frees killed actors at the end */
    LEO_API void leo_actor_system_update(leo_ActorSystem *sys, float dt);

    /* Render pass (tree order). You may call multiple times per frame if needed. */
    LEO_API void leo_actor_system_render(leo_ActorSystem *sys);

    /* Global pause toggles (affects effective pause). */
    LEO_API void leo_actor_system_set_paused(leo_ActorSystem *sys, bool paused);
    LEO_API bool leo_actor_system_is_paused(const leo_ActorSystem *sys);

    /* Root scene node (never null while system exists). Do not kill/destroy it. */
    LEO_API leo_Actor *leo_actor_system_root(leo_ActorSystem *sys);

    /* ----------------------------- */
    /* Spawning & lifetime           */
    /* ----------------------------- */

    /* Create a child actor under `parent`. Returns NULL if allocation fails or
       if vtable->on_init exists and returns false (in which case the actor is
       immediately scheduled for destruction and not returned). */
    LEO_API leo_Actor *leo_actor_spawn(leo_Actor *parent, const leo_ActorDesc *desc);

    /* Schedule an actor (and all its descendants) for destruction.
       Safe to call during update/render. on_exit will be invoked before free. */
    LEO_API void leo_actor_kill(leo_Actor *actor);

    /* ----------------------------- */
    /* Identity & hierarchy          */
    /* ----------------------------- */

    LEO_API leo_ActorUID leo_actor_uid(const leo_Actor *a); /* unique per-system */
    LEO_API const char *leo_actor_name(const leo_Actor *a); /* may be NULL */
    LEO_API leo_Actor *leo_actor_parent(const leo_Actor *a);

    /* Lookups by name:
       - child:      ONLY immediate children of `parent`
       - recursive:  any descendant depth-first under `parent` (includes children) */
    LEO_API leo_Actor *leo_actor_find_child_by_name(const leo_Actor *parent, const char *name);
    LEO_API leo_Actor *leo_actor_find_recursive_by_name(const leo_Actor *parent, const char *name);

    /* Lookup by UID (system-wide). */
    LEO_API leo_Actor *leo_actor_find_by_uid(leo_ActorSystem *sys, leo_ActorUID uid);

    /* ----------------------------- */
    /* Groups                        */
    /* ----------------------------- */

    /* Returns a group bit index [0..63] for `name`. Creates a new one if needed
       (fails and returns -1 if already at capacity). */
    LEO_API int leo_actor_group_get_or_create(leo_ActorSystem *sys, const char *name);

    /* Returns an existing group bit index, or -1 if unknown. */
    LEO_API int leo_actor_group_find(const leo_ActorSystem *sys, const char *name);

    /* Actor group membership operations. */
    LEO_API void leo_actor_add_to_group(leo_Actor *a, int group_bit);
    LEO_API void leo_actor_remove_from_group(leo_Actor *a, int group_bit);
    LEO_API bool leo_actor_in_group(const leo_Actor *a, int group_bit);
    LEO_API leo_ActorGroupMask leo_actor_groups(const leo_Actor *a);

    /* Enumerate all actors currently in a group (system-wide).
       `fn` is called for each live actor in the group. */
    typedef void (*leo_ActorEnumFn)(leo_Actor *a, void *user);
    LEO_API void leo_actor_for_each_in_group(leo_ActorSystem *sys, int group_bit, leo_ActorEnumFn fn, void *user);

    /* ----------------------------- */
    /* Pause                         */
    /* ----------------------------- */

    /* Local paused flag for this actor. Effective pause is:
       system_paused || parent_effective_paused || self_paused */
    LEO_API void leo_actor_set_paused(leo_Actor *a, bool paused);
    LEO_API bool leo_actor_is_paused(const leo_Actor *a);             /* local flag */
    LEO_API bool leo_actor_is_effectively_paused(const leo_Actor *a); /* cascaded */

    /* ----------------------------- */
    /* Signals (existing system)     */
    /* ----------------------------- */

    /* Access the embedded signal emitter on any actor. Useful to connect/emit
       with leo_signal_* without exposing internals. */
    LEO_API leo_SignalEmitter *leo_actor_emitter(leo_Actor *a);
    LEO_API const leo_SignalEmitter *leo_actor_emitter_const(const leo_Actor *a);

    /* Common signal names you may optionally define/emit from actors:
       - "spawned"  : emitted after on_init returns true
       - "killed"   : emitted when kill is requested
       - "exiting"  : emitted just before on_exit
       You can define your own signals freely per-actor as well. */

    /* ----------------------------- */
    /* User data                     */
    /* ----------------------------- */

    /* Raw pointer for game code; the engine never touches this. */
    LEO_API void *leo_actor_userdata(leo_Actor *a);
    LEO_API void leo_actor_set_userdata(leo_Actor *a, void *p);

    /* ----------------------------- */
    /* Utilities                     */
    /* ----------------------------- */

    /* Child iteration helpers (immediate children only). */
    typedef void (*leo_ActorChildFn)(leo_Actor *child, void *user);
    LEO_API void leo_actor_for_each_child(leo_Actor *parent, leo_ActorChildFn fn, void *user);

    /* Z-order helpers (optional; stable order inside siblings).
       Larger z means drawn later (on top). Default = 0. */
    LEO_API void leo_actor_set_z(leo_Actor *a, int z);
    LEO_API int leo_actor_get_z(const leo_Actor *a);

#ifdef __cplusplus
} /* extern "C" */
#endif
