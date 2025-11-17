#include "leo/lua_bindings.h"

#include "leo/actor.h"
#include "leo/animation.h"
#include "leo/audio.h"
#include "leo/base64.h"
#include "leo/collisions.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/font.h"
#include "leo/gamepad.h"
#include "leo/image.h"
#include "leo/json.h"
#include "leo/keyboard.h"
#include "leo/keys.h"
#include "leo/tiled.h"
#include "leo/touch.h"
#include "leo/transitions.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    leo_Texture2D tex;
    int owns; // whether GC should unload
} leo__LuaTexture;

typedef struct
{
    leo_RenderTexture2D rt;
    int owns; // whether GC should unload
} leo__LuaRenderTexture;

typedef struct
{
    leo_Font font;
    int owns; // whether GC should unload
} leo__LuaFont;

typedef struct
{
    leo_JsonDoc *doc;
    int owns;
} leo__LuaJsonDoc;

typedef struct
{
    leo_JsonNode node;
} leo__LuaJsonNode;

typedef struct
{
    leo_TiledMap *map;
    int owns;
} leo__LuaTiledMap;

typedef struct
{
    leo__LuaTiledMap *map_ud;
    const leo_TiledTileLayer *layer;
} leo__LuaTiledTileLayer;

typedef struct
{
    leo__LuaTiledMap *map_ud;
    const leo_TiledObjectLayer *layer;
} leo__LuaTiledObjectLayer;

typedef struct leo__LuaActorSystemUD
{
    leo_ActorSystem *sys;
    lua_State *L;
    int owns;
    struct leo__LuaActorSystemUD *next;
} leo__LuaActorSystemUD;

typedef struct leo__LuaActorBinding
{
    uint32_t magic;
    leo__LuaActorSystemUD *sys_ud;
    int ref_table;
    void *user_ptr;
    char *name_owned;
} leo__LuaActorBinding;

#define LEO_LUA_ACTOR_MAGIC 0x4C414354u /* 'LACT' */

typedef struct
{
    lua_State *L;
    int ref_func;
    int ref_user;
} leo__LuaActorEnumCtx;

typedef struct
{
    leo_Animation anim;
    int owns;
} leo__LuaAnimation;

typedef struct
{
    leo_AnimationPlayer player;
} leo__LuaAnimationPlayer;

typedef struct
{
    leo_Sound sound;
    int owns;
} leo__LuaSound;

static const char *LEO_TEX_META = "leo_texture";
static const char *LEO_RT_META = "leo_render_texture";
static const char *LEO_FONT_META = "leo_font";
static const char *LEO_JSON_DOC_META = "leo_json_doc";
static const char *LEO_JSON_NODE_META = "leo_json_node";
static const char *LEO_TILED_MAP_META = "leo_tiled_map";
static const char *LEO_TILED_TILE_LAYER_META = "leo_tiled_tile_layer";
static const char *LEO_TILED_OBJECT_LAYER_META = "leo_tiled_object_layer";
static const char *LEO_ACTOR_SYSTEM_META = "leo_actor_system";
static const char *LEO_ANIMATION_META = "leo_animation";
static const char *LEO_ANIMATION_PLAYER_META = "leo_animation_player";
static const char *LEO_SOUND_META = "leo_sound";

static leo__LuaActorSystemUD *g_actor_systems = NULL;
static struct
{
    lua_State *L;
    int ref;
} g_transition_callback = {NULL, LUA_NOREF};

static bool leo__lua_actor_on_init(leo_Actor *self);
static void leo__lua_actor_on_update(leo_Actor *self, float dt);
static void leo__lua_actor_on_render(leo_Actor *self);
static void leo__lua_actor_on_exit(leo_Actor *self);

static const leo_ActorVTable LEO_LUA_ACTOR_VTABLE = {
    leo__lua_actor_on_init,
    leo__lua_actor_on_update,
    leo__lua_actor_on_render,
    leo__lua_actor_on_exit,
};

static leo__LuaTexture *push_texture(lua_State *L, leo_Texture2D t, int owns)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)lua_newuserdata(L, sizeof(leo__LuaTexture));
    ud->tex = t;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_TEX_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaTexture *check_texture_ud(lua_State *L, int idx)
{
    return (leo__LuaTexture *)luaL_checkudata(L, idx, LEO_TEX_META);
}

static leo__LuaRenderTexture *push_render_texture(lua_State *L, leo_RenderTexture2D rt, int owns)
{
    leo__LuaRenderTexture *ud = (leo__LuaRenderTexture *)lua_newuserdata(L, sizeof(leo__LuaRenderTexture));
    ud->rt = rt;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_RT_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaRenderTexture *check_render_texture_ud(lua_State *L, int idx)
{
    return (leo__LuaRenderTexture *)luaL_checkudata(L, idx, LEO_RT_META);
}

static leo__LuaFont *push_font(lua_State *L, leo_Font font, int owns)
{
    leo__LuaFont *ud = (leo__LuaFont *)lua_newuserdata(L, sizeof(leo__LuaFont));
    ud->font = font;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_FONT_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaFont *check_font_ud(lua_State *L, int idx)
{
    return (leo__LuaFont *)luaL_checkudata(L, idx, LEO_FONT_META);
}

static leo__LuaJsonDoc *push_json_doc(lua_State *L, leo_JsonDoc *doc, int owns)
{
    leo__LuaJsonDoc *ud = (leo__LuaJsonDoc *)lua_newuserdata(L, sizeof(leo__LuaJsonDoc));
    ud->doc = doc;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_JSON_DOC_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaJsonDoc *check_json_doc_ud(lua_State *L, int idx)
{
    return (leo__LuaJsonDoc *)luaL_checkudata(L, idx, LEO_JSON_DOC_META);
}

static leo__LuaJsonNode *push_json_node(lua_State *L, leo_JsonNode node)
{
    leo__LuaJsonNode *ud = (leo__LuaJsonNode *)lua_newuserdata(L, sizeof(leo__LuaJsonNode));
    ud->node = node;
    luaL_getmetatable(L, LEO_JSON_NODE_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaJsonNode *check_json_node_ud(lua_State *L, int idx)
{
    return (leo__LuaJsonNode *)luaL_checkudata(L, idx, LEO_JSON_NODE_META);
}

static leo_JsonDoc *lua_check_json_doc(lua_State *L, int idx)
{
    leo__LuaJsonDoc *ud = check_json_doc_ud(L, idx);
    luaL_argcheck(L, ud->doc != NULL, idx, "json document has been freed");
    return ud->doc;
}

static leo_JsonNode lua_check_json_node(lua_State *L, int idx)
{
    leo__LuaJsonNode *ud = check_json_node_ud(L, idx);
    return ud->node;
}

static leo_SignalEmitter *lua_check_signal_emitter_ptr(lua_State *L, int idx)
{
    luaL_argcheck(L, lua_islightuserdata(L, idx), idx, "expected lightuserdata (leo_SignalEmitter*)");
    return (leo_SignalEmitter *)lua_touserdata(L, idx);
}

static void lua_actor_system_register(leo__LuaActorSystemUD *ud)
{
    ud->next = g_actor_systems;
    g_actor_systems = ud;
}

static void lua_actor_system_unregister(leo__LuaActorSystemUD *ud)
{
    leo__LuaActorSystemUD **it = &g_actor_systems;
    while (*it)
    {
        if (*it == ud)
        {
            *it = ud->next;
            return;
        }
        it = &(*it)->next;
    }
}

static leo__LuaActorBinding *lua_actor_binding_from_actor(const leo_Actor *actor)
{
    if (!actor)
        return NULL;
    void *ptr = leo_actor_userdata((leo_Actor *)actor);
    if (!ptr)
        return NULL;
    leo__LuaActorBinding *binding = (leo__LuaActorBinding *)ptr;
    return (binding && binding->magic == LEO_LUA_ACTOR_MAGIC) ? binding : NULL;
}

static lua_State *lua_actor_binding_state(leo__LuaActorBinding *binding)
{
    if (!binding || !binding->sys_ud)
        return NULL;
    return binding->sys_ud->L;
}

static leo__LuaActorSystemUD *check_actor_system_ud(lua_State *L, int idx)
{
    return (leo__LuaActorSystemUD *)luaL_checkudata(L, idx, LEO_ACTOR_SYSTEM_META);
}

static leo_ActorSystem *lua_get_actor_system(lua_State *L, int idx, leo__LuaActorSystemUD **out_ud)
{
    leo__LuaActorSystemUD *ud = check_actor_system_ud(L, idx);
    luaL_argcheck(L, ud->sys != NULL, idx, "actor system has been destroyed");
    if (out_ud)
        *out_ud = ud;
    return ud->sys;
}

static leo_Actor *lua_check_actor(lua_State *L, int idx)
{
    luaL_argcheck(L, lua_islightuserdata(L, idx), idx, "expected actor pointer (lightuserdata)");
    return (leo_Actor *)lua_touserdata(L, idx);
}

static int lua_actor_binding_get_field(lua_State *L, leo__LuaActorBinding *binding, const char *field)
{
    if (!binding || binding->ref_table == LUA_NOREF)
        return 0;
    lua_rawgeti(L, LUA_REGISTRYINDEX, binding->ref_table);
    lua_getfield(L, -1, field);
    lua_remove(L, -2);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return 0;
    }
    return 1;
}

static void lua_actor_binding_release(lua_State *L, leo__LuaActorBinding *binding)
{
    if (!binding)
        return;
    if (binding->ref_table != LUA_NOREF && binding->ref_table != LUA_REFNIL)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, binding->ref_table);
        binding->ref_table = LUA_NOREF;
    }
    if (binding->name_owned)
    {
        free(binding->name_owned);
        binding->name_owned = NULL;
    }
    binding->magic = 0;
    free(binding);
}

static void leo__lua_actor_enum_invoke(leo__LuaActorEnumCtx *ctx, leo_Actor *actor)
{
    if (!ctx || ctx->ref_func == LUA_NOREF)
        return;
    lua_State *L = ctx->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ref_func);
    lua_pushlightuserdata(L, actor);
    if (ctx->ref_user != LUA_NOREF)
        lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ref_user);
    else
        lua_pushnil(L);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_actor callback error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void leo__lua_actor_child_fn(leo_Actor *child, void *user)
{
    leo__LuaActorEnumCtx *ctx = (leo__LuaActorEnumCtx *)user;
    leo__lua_actor_enum_invoke(ctx, child);
}

static void leo__lua_actor_group_fn(leo_Actor *actor, void *user)
{
    leo__LuaActorEnumCtx *ctx = (leo__LuaActorEnumCtx *)user;
    leo__lua_actor_enum_invoke(ctx, actor);
}

static void lua_transition_clear_callback(void)
{
    if (g_transition_callback.ref != LUA_NOREF && g_transition_callback.L)
    {
        luaL_unref(g_transition_callback.L, LUA_REGISTRYINDEX, g_transition_callback.ref);
    }
    g_transition_callback.ref = LUA_NOREF;
    g_transition_callback.L = NULL;
}

static void leo__lua_transition_on_complete(void)
{
    if (g_transition_callback.ref == LUA_NOREF || !g_transition_callback.L)
        return;
    lua_State *L = g_transition_callback.L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, g_transition_callback.ref);
    lua_transition_clear_callback();
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_transition callback error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void lua_transition_set_callback(lua_State *L, int idx)
{
    lua_transition_clear_callback();
    if (lua_isnoneornil(L, idx))
        return;
    luaL_checktype(L, idx, LUA_TFUNCTION);
    lua_pushvalue(L, idx);
    g_transition_callback.ref = luaL_ref(L, LUA_REGISTRYINDEX);
    g_transition_callback.L = L;
}

static leo__LuaAnimation *push_animation(lua_State *L, leo_Animation anim, int owns)
{
    leo__LuaAnimation *ud = (leo__LuaAnimation *)lua_newuserdata(L, sizeof(leo__LuaAnimation));
    ud->anim = anim;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_ANIMATION_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaAnimation *check_animation_ud(lua_State *L, int idx)
{
    return (leo__LuaAnimation *)luaL_checkudata(L, idx, LEO_ANIMATION_META);
}

static leo__LuaAnimationPlayer *push_animation_player(lua_State *L, leo_AnimationPlayer player)
{
    leo__LuaAnimationPlayer *ud =
        (leo__LuaAnimationPlayer *)lua_newuserdata(L, sizeof(leo__LuaAnimationPlayer));
    ud->player = player;
    luaL_getmetatable(L, LEO_ANIMATION_PLAYER_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaAnimationPlayer *check_animation_player_ud(lua_State *L, int idx)
{
    return (leo__LuaAnimationPlayer *)luaL_checkudata(L, idx, LEO_ANIMATION_PLAYER_META);
}

static leo__LuaTiledMap *push_tiled_map(lua_State *L, leo_TiledMap *map, int owns)
{
    leo__LuaTiledMap *ud = (leo__LuaTiledMap *)lua_newuserdata(L, sizeof(leo__LuaTiledMap));
    ud->map = map;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_TILED_MAP_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaTiledMap *check_tiled_map_ud(lua_State *L, int idx)
{
    return (leo__LuaTiledMap *)luaL_checkudata(L, idx, LEO_TILED_MAP_META);
}

static leo_TiledMap *lua_check_tiled_map(lua_State *L, int idx)
{
    leo__LuaTiledMap *ud = check_tiled_map_ud(L, idx);
    luaL_argcheck(L, ud->map != NULL, idx, "tiled map has been freed");
    return ud->map;
}

static leo__LuaTiledTileLayer *push_tiled_tile_layer(lua_State *L, leo__LuaTiledMap *map_ud,
                                                     const leo_TiledTileLayer *layer, int map_index)
{
    leo__LuaTiledTileLayer *ud =
        (leo__LuaTiledTileLayer *)lua_newuserdata(L, sizeof(leo__LuaTiledTileLayer));
    ud->map_ud = map_ud;
    ud->layer = layer;
    luaL_getmetatable(L, LEO_TILED_TILE_LAYER_META);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, map_index);
    lua_setiuservalue(L, -2, 1);
    return ud;
}

static leo__LuaTiledObjectLayer *push_tiled_object_layer(lua_State *L, leo__LuaTiledMap *map_ud,
                                                         const leo_TiledObjectLayer *layer, int map_index)
{
    leo__LuaTiledObjectLayer *ud =
        (leo__LuaTiledObjectLayer *)lua_newuserdata(L, sizeof(leo__LuaTiledObjectLayer));
    ud->map_ud = map_ud;
    ud->layer = layer;
    luaL_getmetatable(L, LEO_TILED_OBJECT_LAYER_META);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, map_index);
    lua_setiuservalue(L, -2, 1);
    return ud;
}

static leo__LuaTiledTileLayer *check_tiled_tile_layer_ud(lua_State *L, int idx)
{
    return (leo__LuaTiledTileLayer *)luaL_checkudata(L, idx, LEO_TILED_TILE_LAYER_META);
}

static const leo_TiledTileLayer *lua_check_tiled_tile_layer(lua_State *L, int idx)
{
    leo__LuaTiledTileLayer *ud = check_tiled_tile_layer_ud(L, idx);
    luaL_argcheck(L, ud->map_ud && ud->map_ud->map, idx, "tiled map has been freed");
    return ud->layer;
}

static leo__LuaTiledObjectLayer *check_tiled_object_layer_ud(lua_State *L, int idx)
{
    return (leo__LuaTiledObjectLayer *)luaL_checkudata(L, idx, LEO_TILED_OBJECT_LAYER_META);
}

static const leo_TiledObjectLayer *lua_check_tiled_object_layer(lua_State *L, int idx)
{
    leo__LuaTiledObjectLayer *ud = check_tiled_object_layer_ud(L, idx);
    luaL_argcheck(L, ud->map_ud && ud->map_ud->map, idx, "tiled map has been freed");
    return ud->layer;
}

static void lua_push_tiled_properties(lua_State *L, const leo_TiledProperty *props, int count)
{
    lua_createtable(L, 0, count > 0 ? count : 0);
    for (int i = 0; i < count; ++i)
    {
        const char *name = props[i].name ? props[i].name : "";
        switch (props[i].type)
        {
        case LEO_TILED_PROP_STRING:
            lua_pushstring(L, props[i].val.s ? props[i].val.s : "");
            break;
        case LEO_TILED_PROP_INT:
            lua_pushinteger(L, props[i].val.i);
            break;
        case LEO_TILED_PROP_FLOAT:
            lua_pushnumber(L, props[i].val.f);
            break;
        case LEO_TILED_PROP_BOOL:
            lua_pushboolean(L, props[i].val.b != 0);
            break;
        default:
            lua_pushnil(L);
            break;
        }
        lua_setfield(L, -2, name);
    }
}

static void lua_push_tiled_object(lua_State *L, const leo_TiledObject *obj)
{
    lua_createtable(L, 0, 9);
    lua_pushstring(L, obj->name ? obj->name : "");
    lua_setfield(L, -2, "name");
    lua_pushstring(L, obj->type ? obj->type : "");
    lua_setfield(L, -2, "type");
    lua_pushnumber(L, obj->x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, obj->y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, obj->width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, obj->height);
    lua_setfield(L, -2, "height");
    lua_pushinteger(L, (lua_Integer)obj->gid_raw);
    lua_setfield(L, -2, "gid");
    lua_push_tiled_properties(L, obj->props, obj->prop_count);
    lua_setfield(L, -2, "properties");
}

static void lua_push_tiled_tileset(lua_State *L, const leo_TiledTileset *ts)
{
    lua_createtable(L, 0, 8);
    lua_pushinteger(L, ts->first_gid);
    lua_setfield(L, -2, "first_gid");
    lua_pushinteger(L, ts->tilewidth);
    lua_setfield(L, -2, "tilewidth");
    lua_pushinteger(L, ts->tileheight);
    lua_setfield(L, -2, "tileheight");
    lua_pushinteger(L, ts->imagewidth);
    lua_setfield(L, -2, "imagewidth");
    lua_pushinteger(L, ts->imageheight);
    lua_setfield(L, -2, "imageheight");
    lua_pushinteger(L, ts->columns);
    lua_setfield(L, -2, "columns");
    lua_pushinteger(L, ts->tilecount);
    lua_setfield(L, -2, "tilecount");
    lua_pushstring(L, ts->name ? ts->name : "");
    lua_setfield(L, -2, "name");
    lua_pushstring(L, ts->image ? ts->image : "");
    lua_setfield(L, -2, "image");
}

static int leo_tiled_count_layers_of_type(const leo_TiledMap *map, leo_TiledLayerType type)
{
    int count = 0;
    if (!map)
        return 0;
    for (int i = 0; i < map->layer_count; ++i)
    {
        if (map->layers[i].type == type)
            count++;
    }
    return count;
}

static const leo_TiledTileLayer *leo_tiled_get_tile_layer_at(const leo_TiledMap *map, int idx)
{
    int count = 0;
    if (!map)
        return NULL;
    for (int i = 0; i < map->layer_count; ++i)
    {
        if (map->layers[i].type == LEO_TILED_LAYER_TILE)
        {
            count++;
            if (count == idx)
                return &map->layers[i].as.tile;
        }
    }
    return NULL;
}

static const leo_TiledObjectLayer *leo_tiled_get_object_layer_at(const leo_TiledMap *map, int idx)
{
    int count = 0;
    if (!map)
        return NULL;
    for (int i = 0; i < map->layer_count; ++i)
    {
        if (map->layers[i].type == LEO_TILED_LAYER_OBJECT)
        {
            count++;
            if (count == idx)
                return &map->layers[i].as.object;
        }
    }
    return NULL;
}

static int l_texture_gc(lua_State *L)
{
    leo__LuaTexture *ud = (leo__LuaTexture *)luaL_testudata(L, 1, LEO_TEX_META);
    if (ud && ud->owns)
    {
        leo_UnloadTexture(&ud->tex);
        ud->owns = 0;
        ud->tex.width = 0;
        ud->tex.height = 0;
        ud->tex._handle = NULL;
    }
    return 0;
}

static int l_render_texture_gc(lua_State *L)
{
    leo__LuaRenderTexture *ud = (leo__LuaRenderTexture *)luaL_testudata(L, 1, LEO_RT_META);
    if (ud && ud->owns)
    {
        leo_UnloadRenderTexture(ud->rt);
        ud->owns = 0;
        ud->rt.texture.width = 0;
        ud->rt.texture.height = 0;
        ud->rt.texture._handle = NULL;
        ud->rt.width = 0;
        ud->rt.height = 0;
        ud->rt._rt_handle = NULL;
    }
    return 0;
}

static int l_font_gc(lua_State *L)
{
    leo__LuaFont *ud = (leo__LuaFont *)luaL_testudata(L, 1, LEO_FONT_META);
    if (ud && ud->owns)
    {
        leo_UnloadFont(&ud->font);
        ud->owns = 0;
        ud->font._atlas = NULL;
        ud->font._glyphs = NULL;
        ud->font.baseSize = 0;
        ud->font.lineHeight = 0;
        ud->font.glyphCount = 0;
        ud->font._atlasW = 0;
        ud->font._atlasH = 0;
    }
    return 0;
}

static int l_json_doc_gc(lua_State *L)
{
    leo__LuaJsonDoc *ud = (leo__LuaJsonDoc *)luaL_testudata(L, 1, LEO_JSON_DOC_META);
    if (ud && ud->owns && ud->doc)
    {
        leo_json_free(ud->doc);
        ud->doc = NULL;
        ud->owns = 0;
    }
    return 0;
}

static int l_tiled_map_gc(lua_State *L)
{
    leo__LuaTiledMap *ud = (leo__LuaTiledMap *)luaL_testudata(L, 1, LEO_TILED_MAP_META);
    if (ud && ud->owns && ud->map)
    {
        leo_tiled_free(ud->map);
        ud->map = NULL;
        ud->owns = 0;
    }
    return 0;
}

static void register_texture_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_TEX_META))
    {
        lua_pushcfunction(L, l_texture_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_font_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_FONT_META))
    {
        lua_pushcfunction(L, l_font_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_render_texture_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_RT_META))
    {
        lua_pushcfunction(L, l_render_texture_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_json_doc_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_JSON_DOC_META))
    {
        lua_pushcfunction(L, l_json_doc_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_json_node_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_JSON_NODE_META))
    {
        lua_pushstring(L, "leo_json_node");
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
}

static void register_tiled_map_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_TILED_MAP_META))
    {
        lua_pushcfunction(L, l_tiled_map_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_tiled_tile_layer_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_TILED_TILE_LAYER_META))
    {
        lua_pushstring(L, "leo_tiled_tile_layer");
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
}

static void register_tiled_object_layer_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_TILED_OBJECT_LAYER_META))
    {
        lua_pushstring(L, "leo_tiled_object_layer");
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
}

static int l_actor_system_gc(lua_State *L)
{
    leo__LuaActorSystemUD *ud = (leo__LuaActorSystemUD *)luaL_testudata(L, 1, LEO_ACTOR_SYSTEM_META);
    if (!ud || !ud->owns)
        return 0;
    if (ud->sys)
    {
        lua_actor_system_unregister(ud);
        leo_actor_system_destroy(ud->sys);
        ud->sys = NULL;
    }
    ud->owns = 0;
    return 0;
}

static void register_actor_system_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_ACTOR_SYSTEM_META))
    {
        lua_pushcfunction(L, l_actor_system_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static int l_animation_gc(lua_State *L)
{
    leo__LuaAnimation *ud = (leo__LuaAnimation *)luaL_testudata(L, 1, LEO_ANIMATION_META);
    if (ud && ud->owns)
    {
        leo_UnloadAnimation(&ud->anim);
        ud->owns = 0;
        ud->anim.texture.width = 0;
        ud->anim.texture.height = 0;
        ud->anim.texture._handle = NULL;
        ud->anim.frameCount = 0;
    }
    return 0;
}

static void register_animation_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_ANIMATION_META))
    {
        lua_pushcfunction(L, l_animation_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static void register_animation_player_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_ANIMATION_PLAYER_META))
    {
        lua_pushstring(L, "leo_animation_player");
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
}

static int l_sound_gc(lua_State *L)
{
    leo__LuaSound *ud = (leo__LuaSound *)luaL_testudata(L, 1, LEO_SOUND_META);
    if (ud && ud->owns)
    {
        leo_UnloadSound(&ud->sound);
        ud->owns = 0;
        ud->sound._handle = NULL;
        ud->sound.channels = 0;
        ud->sound.sampleRate = 0;
    }
    return 0;
}

static void register_sound_mt(lua_State *L)
{
    if (luaL_newmetatable(L, LEO_SOUND_META))
    {
        lua_pushcfunction(L, l_sound_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

static float lua_get_field_float(lua_State *L, int index, const char *name, float def)
{
    float value = def;
    lua_getfield(L, index, name);
    if (!lua_isnil(L, -1))
        value = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);
    return value;
}

static void lua_camera_from_table(lua_State *L, int index, leo_Camera2D *cam)
{
    cam->target.x = lua_get_field_float(L, index, "target_x", 0.0f);
    cam->target.y = lua_get_field_float(L, index, "target_y", 0.0f);
    cam->offset.x = lua_get_field_float(L, index, "offset_x", 0.0f);
    cam->offset.y = lua_get_field_float(L, index, "offset_y", 0.0f);
    cam->rotation = lua_get_field_float(L, index, "rotation", 0.0f);
    cam->zoom = lua_get_field_float(L, index, "zoom", 1.0f);
}

static void lua_camera_from_numbers(lua_State *L, int index, leo_Camera2D *cam)
{
    cam->target.x = (float)luaL_checknumber(L, index);
    cam->target.y = (float)luaL_checknumber(L, index + 1);
    cam->offset.x = (float)luaL_checknumber(L, index + 2);
    cam->offset.y = (float)luaL_checknumber(L, index + 3);
    cam->rotation = (float)luaL_checknumber(L, index + 4);
    cam->zoom = (float)luaL_checknumber(L, index + 5);
}

static int lua_try_camera(lua_State *L, int index, leo_Camera2D *cam)
{
    if (lua_isnoneornil(L, index))
        return 0;
    if (lua_istable(L, index))
    {
        lua_camera_from_table(L, index, cam);
        return 1;
    }
    lua_camera_from_numbers(L, index, cam);
    return 6;
}

static void lua_push_camera_table(lua_State *L, const leo_Camera2D *cam)
{
    lua_createtable(L, 0, 6);
    lua_pushnumber(L, cam->target.x);
    lua_setfield(L, -2, "target_x");
    lua_pushnumber(L, cam->target.y);
    lua_setfield(L, -2, "target_y");
    lua_pushnumber(L, cam->offset.x);
    lua_setfield(L, -2, "offset_x");
    lua_pushnumber(L, cam->offset.y);
    lua_setfield(L, -2, "offset_y");
    lua_pushnumber(L, cam->rotation);
    lua_setfield(L, -2, "rotation");
    lua_pushnumber(L, cam->zoom);
    lua_setfield(L, -2, "zoom");
}

static void lua_push_vector2(lua_State *L, leo_Vector2 v)
{
    lua_pushnumber(L, v.x);
    lua_pushnumber(L, v.y);
}

static int lua_push_vector2_touch(lua_State *L, leo_Vector2Touch v)
{
    lua_pushnumber(L, v.x);
    lua_pushnumber(L, v.y);
    return 2;
}

static leo_Rectangle lua_read_rect_args(lua_State *L, int idx)
{
    leo_Rectangle rect;
    rect.x = (float)luaL_checknumber(L, idx);
    rect.y = (float)luaL_checknumber(L, idx + 1);
    rect.width = (float)luaL_checknumber(L, idx + 2);
    rect.height = (float)luaL_checknumber(L, idx + 3);
    return rect;
}

static leo_Vector2 lua_read_vec2_args(lua_State *L, int idx)
{
    leo_Vector2 v;
    v.x = (float)luaL_checknumber(L, idx);
    v.y = (float)luaL_checknumber(L, idx + 1);
    return v;
}

static void lua_push_rectangle_table(lua_State *L, leo_Rectangle rect)
{
    lua_createtable(L, 0, 4);
    lua_pushnumber(L, rect.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, rect.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, rect.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, rect.height);
    lua_setfield(L, -2, "height");
}

// -----------------------------------------------------------------------------
// Engine window/system bindings
// -----------------------------------------------------------------------------
static int l_leo_init_window(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    const char *title = luaL_optstring(L, 3, "leo");
    lua_pushboolean(L, leo_InitWindow(width, height, title));
    return 1;
}

static int l_leo_close_window(lua_State *L)
{
    (void)L;
    leo_CloseWindow();
    return 0;
}

static int l_leo_get_window(lua_State *L)
{
    void *win = leo_GetWindow();
    if (win)
        lua_pushlightuserdata(L, win);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_get_renderer(lua_State *L)
{
    void *renderer = leo_GetRenderer();
    if (renderer)
        lua_pushlightuserdata(L, renderer);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_set_fullscreen(lua_State *L)
{
    int enabled = lua_toboolean(L, 1);
    lua_pushboolean(L, leo_SetFullscreen(enabled != 0));
    return 1;
}

static int l_leo_set_window_mode(lua_State *L)
{
    int mode = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_SetWindowMode((leo_WindowMode)mode));
    return 1;
}

static int l_leo_get_window_mode(lua_State *L)
{
    lua_pushinteger(L, leo_GetWindowMode());
    return 1;
}

static int l_leo_window_should_close(lua_State *L)
{
    (void)L;
    lua_pushboolean(L, leo_WindowShouldClose());
    return 1;
}

static int l_leo_begin_drawing(lua_State *L)
{
    (void)L;
    leo_BeginDrawing();
    return 0;
}

static int l_leo_end_drawing(lua_State *L)
{
    (void)L;
    leo_EndDrawing();
    return 0;
}

static int l_leo_clear_background(lua_State *L)
{
    int r = (int)luaL_checkinteger(L, 1);
    int g = (int)luaL_checkinteger(L, 2);
    int b = (int)luaL_checkinteger(L, 3);
    int a = (int)luaL_optinteger(L, 4, 255);
    leo_ClearBackground(r, g, b, a);
    return 0;
}

// -----------------------------------------------------------------------------
// Timing / frame info
// -----------------------------------------------------------------------------
static int l_leo_set_target_fps(lua_State *L)
{
    int fps = (int)luaL_checkinteger(L, 1);
    leo_SetTargetFPS(fps);
    return 0;
}

static int l_leo_get_frame_time(lua_State *L)
{
    lua_pushnumber(L, leo_GetFrameTime());
    return 1;
}

static int l_leo_get_time(lua_State *L)
{
    lua_pushnumber(L, leo_GetTime());
    return 1;
}

static int l_leo_get_fps(lua_State *L)
{
    lua_pushinteger(L, leo_GetFPS());
    return 1;
}

// -----------------------------------------------------------------------------
// Screen info
// -----------------------------------------------------------------------------
static int l_leo_get_screen_width(lua_State *L)
{
    lua_pushinteger(L, leo_GetScreenWidth());
    return 1;
}

static int l_leo_get_screen_height(lua_State *L)
{
    lua_pushinteger(L, leo_GetScreenHeight());
    return 1;
}

// -----------------------------------------------------------------------------
// Keyboard input
// -----------------------------------------------------------------------------
static int l_leo_is_key_pressed(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyPressed(key));
    return 1;
}

static int l_leo_is_key_pressed_repeat(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyPressedRepeat(key));
    return 1;
}

static int l_leo_is_key_down(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyDown(key));
    return 1;
}

static int l_leo_is_key_released(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyReleased(key));
    return 1;
}

static int l_leo_is_key_up(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsKeyUp(key));
    return 1;
}

static int l_leo_get_key_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetKeyPressed());
    return 1;
}

static int l_leo_get_char_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetCharPressed());
    return 1;
}

static int l_leo_set_exit_key(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    leo_SetExitKey(key);
    return 0;
}

static int l_leo_update_keyboard(lua_State *L)
{
    (void)L;
    leo_UpdateKeyboard();
    return 0;
}

static int l_leo_is_exit_key_pressed(lua_State *L)
{
    lua_pushboolean(L, leo_IsExitKeyPressed());
    return 1;
}

static int l_leo_cleanup_keyboard(lua_State *L)
{
    (void)L;
    leo_CleanupKeyboard();
    return 0;
}

// -----------------------------------------------------------------------------
// Touch input & gestures
// -----------------------------------------------------------------------------
static int l_leo_is_touch_down(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsTouchDown(touch));
    return 1;
}

static int l_leo_is_touch_pressed(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsTouchPressed(touch));
    return 1;
}

static int l_leo_is_touch_released(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsTouchReleased(touch));
    return 1;
}

static int l_leo_get_touch_position(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    return lua_push_vector2_touch(L, leo_GetTouchPosition(touch));
}

static int l_leo_get_touch_x(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_GetTouchX(touch));
    return 1;
}

static int l_leo_get_touch_y(lua_State *L)
{
    int touch = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_GetTouchY(touch));
    return 1;
}

static int l_leo_get_touch_point_count(lua_State *L)
{
    lua_pushinteger(L, leo_GetTouchPointCount());
    return 1;
}

static int l_leo_get_touch_point_id(lua_State *L)
{
    int index = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_GetTouchPointId(index));
    return 1;
}

static int l_leo_is_gesture_detected(lua_State *L)
{
    int gesture = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsGestureDetected(gesture));
    return 1;
}

static int l_leo_get_gesture_detected(lua_State *L)
{
    lua_pushinteger(L, leo_GetGestureDetected());
    return 1;
}

static int l_leo_get_gesture_hold_duration(lua_State *L)
{
    lua_pushnumber(L, leo_GetGestureHoldDuration());
    return 1;
}

static int l_leo_get_gesture_drag_vector(lua_State *L)
{
    return lua_push_vector2_touch(L, leo_GetGestureDragVector());
}

static int l_leo_get_gesture_drag_angle(lua_State *L)
{
    lua_pushnumber(L, leo_GetGestureDragAngle());
    return 1;
}

static int l_leo_get_gesture_pinch_vector(lua_State *L)
{
    return lua_push_vector2_touch(L, leo_GetGesturePinchVector());
}

static int l_leo_get_gesture_pinch_angle(lua_State *L)
{
    lua_pushnumber(L, leo_GetGesturePinchAngle());
    return 1;
}

static int l_leo_set_gestures_enabled(lua_State *L)
{
    int flags = (int)luaL_checkinteger(L, 1);
    leo_SetGesturesEnabled(flags);
    return 0;
}

// -----------------------------------------------------------------------------
// Gamepad input
// -----------------------------------------------------------------------------
static int l_leo_init_gamepads(lua_State *L)
{
    (void)L;
    leo_InitGamepads();
    return 0;
}

static int l_leo_update_gamepads(lua_State *L)
{
    (void)L;
    leo_UpdateGamepads();
    return 0;
}

static int l_leo_handle_gamepad_event(lua_State *L)
{
    luaL_argcheck(L, lua_islightuserdata(L, 1), 1, "expected lightuserdata event");
    void *evt = lua_touserdata(L, 1);
    luaL_argcheck(L, evt != NULL, 1, "event pointer must not be NULL");
    leo_HandleGamepadEvent(evt);
    return 0;
}

static int l_leo_shutdown_gamepads(lua_State *L)
{
    (void)L;
    leo_ShutdownGamepads();
    return 0;
}

static int l_leo_is_gamepad_available(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, leo_IsGamepadAvailable(gamepad));
    return 1;
}

static int l_leo_get_gamepad_name(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    const char *name = leo_GetGamepadName(gamepad);
    if (name)
        lua_pushstring(L, name);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_is_gamepad_button_pressed(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonPressed(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_down(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonDown(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_released(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonReleased(gamepad, button));
    return 1;
}

static int l_leo_is_gamepad_button_up(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_IsGamepadButtonUp(gamepad, button));
    return 1;
}

static int l_leo_get_gamepad_button_pressed(lua_State *L)
{
    lua_pushinteger(L, leo_GetGamepadButtonPressed());
    return 1;
}

static int l_leo_get_gamepad_axis_count(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_GetGamepadAxisCount(gamepad));
    return 1;
}

static int l_leo_get_gamepad_axis_movement(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int axis = (int)luaL_checkinteger(L, 2);
    lua_pushnumber(L, leo_GetGamepadAxisMovement(gamepad, axis));
    return 1;
}

static int l_leo_set_gamepad_vibration(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    float left = (float)luaL_checknumber(L, 2);
    float right = (float)luaL_checknumber(L, 3);
    float duration = (float)luaL_checknumber(L, 4);
    lua_pushboolean(L, leo_SetGamepadVibration(gamepad, left, right, duration));
    return 1;
}

static int l_leo_set_gamepad_axis_deadzone(lua_State *L)
{
    float deadzone = (float)luaL_checknumber(L, 1);
    leo_SetGamepadAxisDeadzone(deadzone);
    return 0;
}

static int l_leo_set_gamepad_stick_threshold(lua_State *L)
{
    float press_threshold = (float)luaL_checknumber(L, 1);
    float release_threshold = (float)luaL_checknumber(L, 2);
    leo_SetGamepadStickThreshold(press_threshold, release_threshold);
    return 0;
}

static int l_leo_get_gamepad_stick(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    leo_Vector2 v = leo_GetGamepadStick(gamepad, stick);
    lua_push_vector2(L, v);
    return 2;
}

static int l_leo_is_gamepad_stick_pressed(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickPressed(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_down(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickDown(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_released(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickReleased(gamepad, stick, dir));
    return 1;
}

static int l_leo_is_gamepad_stick_up(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int stick = (int)luaL_checkinteger(L, 2);
    int dir = (int)luaL_checkinteger(L, 3);
    lua_pushboolean(L, leo_IsGamepadStickUp(gamepad, stick, dir));
    return 1;
}

// -----------------------------------------------------------------------------
// Camera bindings
// -----------------------------------------------------------------------------
static int l_leo_begin_mode2d(lua_State *L)
{
    leo_Camera2D cam;
    if (lua_istable(L, 1))
        lua_camera_from_table(L, 1, &cam);
    else
        lua_camera_from_numbers(L, 1, &cam);
    leo_BeginMode2D(cam);
    return 0;
}

static int l_leo_end_mode2d(lua_State *L)
{
    (void)L;
    leo_EndMode2D();
    return 0;
}

static int l_leo_is_camera_active(lua_State *L)
{
    (void)L;
    lua_pushboolean(L, leo_IsCameraActive());
    return 1;
}

static int l_leo_get_world_to_screen2d(lua_State *L)
{
    leo_Vector2 position;
    position.x = (float)luaL_checknumber(L, 1);
    position.y = (float)luaL_checknumber(L, 2);

    leo_Camera2D cam;
    if (lua_try_camera(L, 3, &cam) == 0)
        cam = leo_GetCurrentCamera2D();

    lua_push_vector2(L, leo_GetWorldToScreen2D(position, cam));
    return 2;
}

static int l_leo_get_screen_to_world2d(lua_State *L)
{
    leo_Vector2 position;
    position.x = (float)luaL_checknumber(L, 1);
    position.y = (float)luaL_checknumber(L, 2);

    leo_Camera2D cam;
    if (lua_try_camera(L, 3, &cam) == 0)
        cam = leo_GetCurrentCamera2D();

    lua_push_vector2(L, leo_GetScreenToWorld2D(position, cam));
    return 2;
}

static int l_leo_get_current_camera2d(lua_State *L)
{
    leo_Camera2D cam = leo_GetCurrentCamera2D();
    lua_push_camera_table(L, &cam);
    return 1;
}

// -----------------------------------------------------------------------------
// Render texture bindings
// -----------------------------------------------------------------------------
static int l_leo_load_render_texture(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    leo_RenderTexture2D rt = leo_LoadRenderTexture(width, height);
    push_render_texture(L, rt, 1);
    return 1;
}

static int l_leo_unload_render_texture(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadRenderTexture(ud->rt);
        ud->owns = 0;
    }
    ud->rt.texture.width = 0;
    ud->rt.texture.height = 0;
    ud->rt.texture._handle = NULL;
    ud->rt.width = 0;
    ud->rt.height = 0;
    ud->rt._rt_handle = NULL;
    return 0;
}

static int l_leo_begin_texture_mode(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    leo_BeginTextureMode(ud->rt);
    return 0;
}

static int l_leo_end_texture_mode(lua_State *L)
{
    (void)L;
    leo_EndTextureMode();
    return 0;
}

static int l_leo_render_texture_get_texture(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    push_texture(L, ud->rt.texture, 0);
    return 1;
}

static int l_leo_render_texture_get_size(lua_State *L)
{
    leo__LuaRenderTexture *ud = check_render_texture_ud(L, 1);
    lua_pushinteger(L, ud->rt.width);
    lua_pushinteger(L, ud->rt.height);
    return 2;
}

// -----------------------------------------------------------------------------
// Image / texture bindings
// -----------------------------------------------------------------------------
static int l_image_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    leo_Texture2D tex = leo_LoadTexture(path);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_memory(lua_State *L)
{
    size_t len = 0;
    const char *ext = luaL_checkstring(L, 1);
    const char *data = luaL_checklstring(L, 2, &len);
    leo_Texture2D tex = leo_LoadTextureFromMemory(ext, (const unsigned char *)data, (int)len);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_texture(lua_State *L)
{
    leo__LuaTexture *src = check_texture_ud(L, 1);
    leo_Texture2D tex = leo_LoadTextureFromTexture(src->tex);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_load_from_pixels(lua_State *L)
{
    size_t len = 0;
    const void *pixels = luaL_checklstring(L, 1, &len);
    int width = (int)luaL_checkinteger(L, 2);
    int height = (int)luaL_checkinteger(L, 3);
    int pitch = (int)luaL_optinteger(L, 4, 0);
    int format = (int)luaL_optinteger(L, 5, LEO_TEXFORMAT_R8G8B8A8);
    if (pitch == 0)
    {
        int bpp = leo_TexFormatBytesPerPixel((leo_TexFormat)format);
        pitch = width * bpp;
    }
    (void)len;
    leo_Texture2D tex = leo_LoadTextureFromPixels(pixels, width, height, pitch, (leo_TexFormat)format);
    push_texture(L, tex, 1);
    return 1;
}

static int l_image_is_ready(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    lua_pushboolean(L, leo_IsTextureReady(tex->tex));
    return 1;
}

static int l_image_unload(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    if (tex->owns)
    {
        leo_UnloadTexture(&tex->tex);
        tex->owns = 0;
    }
    tex->tex.width = 0;
    tex->tex.height = 0;
    tex->tex._handle = NULL;
    return 0;
}

static int l_image_bytes_per_pixel(lua_State *L)
{
    int format = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, leo_TexFormatBytesPerPixel((leo_TexFormat)format));
    return 1;
}

static int l_leo_texture_get_size(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    lua_pushinteger(L, tex->tex.width);
    lua_pushinteger(L, tex->tex.height);
    return 2;
}

// -----------------------------------------------------------------------------
// Texture drawing bindings
// -----------------------------------------------------------------------------
static int l_leo_draw_texture_rec(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    leo_Rectangle src;
    src.x = (float)luaL_checknumber(L, 2);
    src.y = (float)luaL_checknumber(L, 3);
    src.width = (float)luaL_checknumber(L, 4);
    src.height = (float)luaL_checknumber(L, 5);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 6);
    pos.y = (float)luaL_checknumber(L, 7);
    int r = (int)luaL_optinteger(L, 8, 255);
    int g = (int)luaL_optinteger(L, 9, 255);
    int b = (int)luaL_optinteger(L, 10, 255);
    int a = (int)luaL_optinteger(L, 11, 255);
    leo_Color tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    leo_DrawTextureRec(tex->tex, src, pos, tint);
    return 0;
}

static int l_leo_draw_texture_pro(lua_State *L)
{
    leo__LuaTexture *tex = check_texture_ud(L, 1);
    leo_Rectangle src;
    src.x = (float)luaL_checknumber(L, 2);
    src.y = (float)luaL_checknumber(L, 3);
    src.width = (float)luaL_checknumber(L, 4);
    src.height = (float)luaL_checknumber(L, 5);
    leo_Rectangle dest;
    dest.x = (float)luaL_checknumber(L, 6);
    dest.y = (float)luaL_checknumber(L, 7);
    dest.width = (float)luaL_checknumber(L, 8);
    dest.height = (float)luaL_checknumber(L, 9);
    leo_Vector2 origin;
    origin.x = (float)luaL_checknumber(L, 10);
    origin.y = (float)luaL_checknumber(L, 11);
    float rotation = (float)luaL_optnumber(L, 12, 0.0);
    int r = (int)luaL_optinteger(L, 13, 255);
    int g = (int)luaL_optinteger(L, 14, 255);
    int b = (int)luaL_optinteger(L, 15, 255);
    int a = (int)luaL_optinteger(L, 16, 255);
    leo_Color tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    leo_DrawTexturePro(tex->tex, src, dest, origin, rotation, tint);
    return 0;
}

// -----------------------------------------------------------------------------
// Font bindings
// -----------------------------------------------------------------------------
static int l_leo_load_font(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int pixel_size = (int)luaL_checkinteger(L, 2);
    leo_Font font = leo_LoadFont(path, pixel_size);
    push_font(L, font, 1);
    return 1;
}

static int l_leo_load_font_from_memory(lua_State *L)
{
    const char *file_type = NULL;
    size_t len = 0;
    const unsigned char *data = NULL;
    int pixel_size = 0;
    int top = lua_gettop(L);
    if (top >= 3)
    {
        if (!lua_isnoneornil(L, 1))
            file_type = luaL_checkstring(L, 1);
        data = (const unsigned char *)luaL_checklstring(L, 2, &len);
        pixel_size = (int)luaL_checkinteger(L, 3);
    }
    else
    {
        data = (const unsigned char *)luaL_checklstring(L, 1, &len);
        pixel_size = (int)luaL_checkinteger(L, 2);
    }
    leo_Font font = leo_LoadFontFromMemory(file_type, data, (int)len, pixel_size);
    push_font(L, font, 1);
    return 1;
}

static int l_leo_unload_font(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadFont(&ud->font);
        ud->owns = 0;
    }
    ud->font._atlas = NULL;
    ud->font._glyphs = NULL;
    ud->font.baseSize = 0;
    ud->font.lineHeight = 0;
    ud->font.glyphCount = 0;
    ud->font._atlasW = 0;
    ud->font._atlasH = 0;
    return 0;
}

static int l_leo_is_font_ready(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    lua_pushboolean(L, leo_IsFontReady(ud->font));
    return 1;
}

static int l_leo_set_default_font(lua_State *L)
{
    if (lua_isnoneornil(L, 1))
    {
        leo_Font empty = {0};
        leo_SetDefaultFont(empty);
    }
    else
    {
        leo__LuaFont *ud = check_font_ud(L, 1);
        leo_SetDefaultFont(ud->font);
    }
    return 0;
}

static int l_leo_get_default_font(lua_State *L)
{
    leo_Font font = leo_GetDefaultFont();
    if (!leo_IsFontReady(font))
    {
        lua_pushnil(L);
        return 1;
    }
    push_font(L, font, 0);
    return 1;
}

static inline leo_Color lua_check_color(lua_State *L, int idx)
{
    int r = (int)luaL_checkinteger(L, idx);
    int g = (int)luaL_checkinteger(L, idx + 1);
    int b = (int)luaL_checkinteger(L, idx + 2);
    int a = (int)luaL_optinteger(L, idx + 3, 255);
    leo_Color col = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    return col;
}

static leo_Color lua_check_color_param(lua_State *L, int idx, int *next_idx)
{
    leo_Color col = {255, 255, 255, 255};
    if (lua_istable(L, idx))
    {
        lua_getfield(L, idx, "r");
        col.r = (unsigned char)luaL_optinteger(L, -1, 255);
        lua_pop(L, 1);
        lua_getfield(L, idx, "g");
        col.g = (unsigned char)luaL_optinteger(L, -1, 255);
        lua_pop(L, 1);
        lua_getfield(L, idx, "b");
        col.b = (unsigned char)luaL_optinteger(L, -1, 255);
        lua_pop(L, 1);
        lua_getfield(L, idx, "a");
        col.a = (unsigned char)luaL_optinteger(L, -1, 255);
        lua_pop(L, 1);
        if (next_idx)
            *next_idx = idx + 1;
    }
    else
    {
        col = lua_check_color(L, idx);
        if (next_idx)
            *next_idx = idx + 4;
    }
    return col;
}

static int l_leo_draw_fps(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    leo_DrawFPS(x, y);
    return 0;
}

static int l_leo_draw_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int font_size = (int)luaL_checkinteger(L, 4);
    leo_Color tint = lua_check_color(L, 5);
    leo_DrawText(text, x, y, font_size, tint);
    return 0;
}

static int l_leo_draw_text_ex(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 3);
    pos.y = (float)luaL_checknumber(L, 4);
    float font_size = (float)luaL_checknumber(L, 5);
    float spacing = (float)luaL_checknumber(L, 6);
    leo_Color tint = lua_check_color(L, 7);
    leo_DrawTextEx(ud->font, text, pos, font_size, spacing, tint);
    return 0;
}

static int l_leo_draw_text_pro(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    leo_Vector2 pos;
    pos.x = (float)luaL_checknumber(L, 3);
    pos.y = (float)luaL_checknumber(L, 4);
    leo_Vector2 origin;
    origin.x = (float)luaL_checknumber(L, 5);
    origin.y = (float)luaL_checknumber(L, 6);
    float rotation = (float)luaL_checknumber(L, 7);
    float font_size = (float)luaL_checknumber(L, 8);
    float spacing = (float)luaL_checknumber(L, 9);
    leo_Color tint = lua_check_color(L, 10);
    leo_DrawTextPro(ud->font, text, pos, origin, rotation, font_size, spacing, tint);
    return 0;
}

static int l_leo_measure_text_ex(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    const char *text = luaL_checkstring(L, 2);
    float font_size = (float)luaL_checknumber(L, 3);
    float spacing = (float)luaL_checknumber(L, 4);
    leo_Vector2 dims = leo_MeasureTextEx(ud->font, text, font_size, spacing);
    lua_pushnumber(L, dims.x);
    lua_pushnumber(L, dims.y);
    return 2;
}

static int l_leo_measure_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int font_size = (int)luaL_checkinteger(L, 2);
    lua_pushinteger(L, leo_MeasureText(text, font_size));
    return 1;
}

static int l_leo_get_font_line_height(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    float font_size = (float)luaL_checknumber(L, 2);
    lua_pushinteger(L, leo_GetFontLineHeight(ud->font, font_size));
    return 1;
}

static int l_leo_get_font_base_size(lua_State *L)
{
    leo__LuaFont *ud = check_font_ud(L, 1);
    lua_pushinteger(L, leo_GetFontBaseSize(ud->font));
    return 1;
}

// -----------------------------------------------------------------------------
// JSON bindings
// -----------------------------------------------------------------------------
static int l_leo_json_parse(lua_State *L)
{
    size_t len = 0;
    const char *data = luaL_checklstring(L, 1, &len);
    const char *err = NULL;
    leo_JsonDoc *doc = leo_json_parse(data, len, &err);
    if (!doc)
    {
        lua_pushnil(L);
        lua_pushstring(L, err ? err : "leo_json_parse failed");
        return 2;
    }
    push_json_doc(L, doc, 1);
    return 1;
}

static int l_leo_json_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    const char *err = NULL;
    leo_JsonDoc *doc = leo_json_load(path, &err);
    if (!doc)
    {
        lua_pushnil(L);
        lua_pushstring(L, err ? err : "leo_json_load failed");
        return 2;
    }
    push_json_doc(L, doc, 1);
    return 1;
}

static int l_leo_json_free(lua_State *L)
{
    leo__LuaJsonDoc *ud = check_json_doc_ud(L, 1);
    if (ud->doc)
    {
        leo_json_free(ud->doc);
        ud->doc = NULL;
    }
    ud->owns = 0;
    return 0;
}

static int l_leo_json_root(lua_State *L)
{
    leo_JsonDoc *doc = lua_check_json_doc(L, 1);
    push_json_node(L, leo_json_root(doc));
    return 1;
}

static int l_leo_json_is_null(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_null(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_object(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_object(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_array(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_array(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_string(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_string(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_number(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_number(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_is_bool(lua_State *L)
{
    lua_pushboolean(L, leo_json_is_bool(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_obj_get(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    push_json_node(L, leo_json_obj_get(obj, key));
    return 1;
}

static int l_leo_json_arr_size(lua_State *L)
{
    size_t size = leo_json_arr_size(lua_check_json_node(L, 1));
    lua_pushinteger(L, (lua_Integer)size);
    return 1;
}

static int l_leo_json_arr_get(lua_State *L)
{
    leo_JsonNode arr = lua_check_json_node(L, 1);
    size_t index = (size_t)luaL_checkinteger(L, 2);
    push_json_node(L, leo_json_arr_get(arr, index));
    return 1;
}

static int l_leo_json_get_string(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    const char *out = NULL;
    bool ok = leo_json_get_string(obj, key, &out) && out;
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushstring(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_int(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    int out = 0;
    bool ok = leo_json_get_int(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushinteger(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_double(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    double out = 0.0;
    bool ok = leo_json_get_double(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushnumber(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_get_bool(lua_State *L)
{
    leo_JsonNode obj = lua_check_json_node(L, 1);
    const char *key = luaL_checkstring(L, 2);
    bool out = false;
    bool ok = leo_json_get_bool(obj, key, &out);
    lua_pushboolean(L, ok);
    if (ok)
        lua_pushboolean(L, out);
    else
        lua_pushnil(L);
    return 2;
}

static int l_leo_json_as_string(lua_State *L)
{
    const char *s = leo_json_as_string(lua_check_json_node(L, 1));
    if (s)
        lua_pushstring(L, s);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_json_as_int(lua_State *L)
{
    lua_pushinteger(L, leo_json_as_int(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_as_double(lua_State *L)
{
    lua_pushnumber(L, leo_json_as_double(lua_check_json_node(L, 1)));
    return 1;
}

static int l_leo_json_as_bool(lua_State *L)
{
    lua_pushboolean(L, leo_json_as_bool(lua_check_json_node(L, 1)));
    return 1;
}


// -----------------------------------------------------------------------------
// Actor bindings
// -----------------------------------------------------------------------------
static int l_leo_actor_system_create(lua_State *L)
{
    leo_ActorSystem *sys = leo_actor_system_create();
    if (!sys)
    {
        lua_pushnil(L);
        lua_pushstring(L, "leo_actor_system_create failed");
        return 2;
    }
    leo__LuaActorSystemUD *ud = (leo__LuaActorSystemUD *)lua_newuserdata(L, sizeof(*ud));
    ud->sys = sys;
    ud->L = L;
    ud->owns = 1;
    lua_actor_system_register(ud);
    luaL_getmetatable(L, LEO_ACTOR_SYSTEM_META);
    lua_setmetatable(L, -2);
    return 1;
}

static int l_leo_actor_system_destroy(lua_State *L)
{
    leo__LuaActorSystemUD *ud = check_actor_system_ud(L, 1);
    if (ud->sys)
    {
        lua_actor_system_unregister(ud);
        leo_actor_system_destroy(ud->sys);
        ud->sys = NULL;
    }
    ud->owns = 0;
    return 0;
}

static int l_leo_actor_system_update(lua_State *L)
{
    leo__LuaActorSystemUD *ud = NULL;
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, &ud);
    float dt = (float)luaL_checknumber(L, 2);
    ud->L = L;
    leo_actor_system_update(sys, dt);
    return 0;
}

static int l_leo_actor_system_render(lua_State *L)
{
    leo__LuaActorSystemUD *ud = NULL;
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, &ud);
    ud->L = L;
    leo_actor_system_render(sys);
    return 0;
}

static int l_leo_actor_system_set_paused(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    bool paused = lua_toboolean(L, 2) != 0;
    leo_actor_system_set_paused(sys, paused);
    return 0;
}

static int l_leo_actor_system_is_paused(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    lua_pushboolean(L, leo_actor_system_is_paused(sys));
    return 1;
}

static int l_leo_actor_system_root(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    leo_Actor *root = leo_actor_system_root(sys);
    if (root)
        lua_pushlightuserdata(L, root);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_spawn(lua_State *L)
{
    leo__LuaActorSystemUD *sys_ud = NULL;
    lua_get_actor_system(L, 1, &sys_ud);
    leo_Actor *parent = lua_check_actor(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    sys_ud->L = L;

    const int desc_index = 3;
    leo_ActorDesc desc;
    memset(&desc, 0, sizeof(desc));

    lua_getfield(L, desc_index, "name");
    int has_name = !lua_isnil(L, -1);
    const char *name_str = NULL;
    if (has_name)
    {
        name_str = luaL_checkstring(L, -1);
        desc.name = name_str;
    }
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "groups");
    if (!lua_isnil(L, -1))
        desc.groups = (leo_ActorGroupMask)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "start_paused");
    if (!lua_isnil(L, -1))
        desc.start_paused = lua_toboolean(L, -1) ? true : false;
    lua_pop(L, 1);

    const leo_ActorVTable *external_vtable = NULL;
    lua_getfield(L, desc_index, "vtable");
    if (!lua_isnil(L, -1))
        external_vtable = (const leo_ActorVTable *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    void *user_ptr = NULL;
    lua_getfield(L, desc_index, "user_ptr");
    if (!lua_isnil(L, -1))
        user_ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);

    int has_on_init = 0, has_on_update = 0, has_on_render = 0, has_on_exit = 0;

    lua_getfield(L, desc_index, "on_init");
    has_on_init = lua_isfunction(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "on_update");
    has_on_update = lua_isfunction(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "on_render");
    has_on_render = lua_isfunction(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "on_exit");
    has_on_exit = lua_isfunction(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, desc_index, "user_value");
    int has_user_value = !lua_isnil(L, -1);
    lua_pop(L, 1);

    if (external_vtable && (has_on_init || has_on_update || has_on_render || has_on_exit))
        return luaL_error(L, "leo_actor_spawn: cannot supply both custom vtable and Lua callbacks");
    if (external_vtable && has_name)
        return luaL_error(L, "leo_actor_spawn: cannot set Lua string name when supplying custom vtable");

    int needs_binding = (!external_vtable) && (has_on_init || has_on_update || has_on_render ||
                                               has_on_exit || has_user_value || has_name);
    leo__LuaActorBinding *binding = NULL;

    if (needs_binding)
    {
        binding = (leo__LuaActorBinding *)calloc(1, sizeof(*binding));
        if (!binding)
            return luaL_error(L, "leo_actor_spawn: OOM allocating binding");
        binding->magic = 0;
        binding->sys_ud = sys_ud;
        binding->user_ptr = user_ptr;
        binding->ref_table = LUA_NOREF;

        lua_newtable(L);
        if (has_on_init)
        {
            lua_getfield(L, desc_index, "on_init");
            lua_setfield(L, -2, "on_init");
        }
        if (has_on_update)
        {
            lua_getfield(L, desc_index, "on_update");
            lua_setfield(L, -2, "on_update");
        }
        if (has_on_render)
        {
            lua_getfield(L, desc_index, "on_render");
            lua_setfield(L, -2, "on_render");
        }
        if (has_on_exit)
        {
            lua_getfield(L, desc_index, "on_exit");
            lua_setfield(L, -2, "on_exit");
        }
        if (has_user_value)
        {
            lua_getfield(L, desc_index, "user_value");
            lua_setfield(L, -2, "user_value");
        }
        binding->ref_table = luaL_ref(L, LUA_REGISTRYINDEX);
        desc.vtable = &LEO_LUA_ACTOR_VTABLE;
        desc.user_data = binding;

        if (has_name && name_str)
        {
            binding->name_owned = strdup(name_str);
            if (!binding->name_owned)
            {
                lua_actor_binding_release(L, binding);
                return luaL_error(L, "leo_actor_spawn: OOM duplicating name");
            }
            desc.name = binding->name_owned;
        }
    }
    else
    {
        desc.vtable = external_vtable;
        desc.user_data = user_ptr;
    }

    sys_ud->L = L;
    leo_Actor *actor = leo_actor_spawn(parent, &desc);
    if (!actor)
    {
        if (binding && binding->magic != LEO_LUA_ACTOR_MAGIC)
            lua_actor_binding_release(L, binding);
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, actor);
    return 1;
}

static int l_leo_actor_kill(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    leo_actor_kill(actor);
    return 0;
}

static int l_leo_actor_uid(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushinteger(L, (lua_Integer)leo_actor_uid(actor));
    return 1;
}

static int l_leo_actor_name(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    const char *name = leo_actor_name(actor);
    if (name)
        lua_pushstring(L, name);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_parent(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    leo_Actor *parent = leo_actor_parent(actor);
    if (parent)
        lua_pushlightuserdata(L, parent);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_find_child_by_name(lua_State *L)
{
    leo_Actor *parent = lua_check_actor(L, 1);
    const char *name = luaL_checkstring(L, 2);
    leo_Actor *child = leo_actor_find_child_by_name(parent, name);
    if (child)
        lua_pushlightuserdata(L, child);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_find_recursive_by_name(lua_State *L)
{
    leo_Actor *parent = lua_check_actor(L, 1);
    const char *name = luaL_checkstring(L, 2);
    leo_Actor *child = leo_actor_find_recursive_by_name(parent, name);
    if (child)
        lua_pushlightuserdata(L, child);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_find_by_uid(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    leo_ActorUID uid = (leo_ActorUID)luaL_checkinteger(L, 2);
    leo_Actor *actor = leo_actor_find_by_uid(sys, uid);
    if (actor)
        lua_pushlightuserdata(L, actor);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_group_get_or_create(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    const char *name = luaL_checkstring(L, 2);
    lua_pushinteger(L, leo_actor_group_get_or_create(sys, name));
    return 1;
}

static int l_leo_actor_group_find(lua_State *L)
{
    const leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    const char *name = luaL_checkstring(L, 2);
    lua_pushinteger(L, leo_actor_group_find(sys, name));
    return 1;
}

static int l_leo_actor_add_to_group(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    int bit = (int)luaL_checkinteger(L, 2);
    leo_actor_add_to_group(actor, bit);
    return 0;
}

static int l_leo_actor_remove_from_group(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    int bit = (int)luaL_checkinteger(L, 2);
    leo_actor_remove_from_group(actor, bit);
    return 0;
}

static int l_leo_actor_in_group(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    int bit = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, leo_actor_in_group(actor, bit));
    return 1;
}

static int l_leo_actor_groups(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushinteger(L, (lua_Integer)leo_actor_groups(actor));
    return 1;
}

static int l_leo_actor_for_each_in_group(lua_State *L)
{
    leo_ActorSystem *sys = lua_get_actor_system(L, 1, NULL);
    int bit = (int)luaL_checkinteger(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    leo__LuaActorEnumCtx ctx = {0};
    ctx.L = L;
    lua_pushvalue(L, 3);
    ctx.ref_func = luaL_ref(L, LUA_REGISTRYINDEX);
    if (!lua_isnoneornil(L, 4))
    {
        lua_pushvalue(L, 4);
        ctx.ref_user = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        ctx.ref_user = LUA_NOREF;
    }

    leo_actor_for_each_in_group(sys, bit, leo__lua_actor_group_fn, &ctx);

    luaL_unref(L, LUA_REGISTRYINDEX, ctx.ref_func);
    if (ctx.ref_user != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, ctx.ref_user);
    return 0;
}

static int l_leo_actor_set_paused(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    bool paused = lua_toboolean(L, 2) != 0;
    leo_actor_set_paused(actor, paused);
    return 0;
}

static int l_leo_actor_is_paused(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushboolean(L, leo_actor_is_paused(actor));
    return 1;
}

static int l_leo_actor_is_effectively_paused(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushboolean(L, leo_actor_is_effectively_paused(actor));
    return 1;
}

static int l_leo_actor_emitter(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushlightuserdata(L, leo_actor_emitter(actor));
    return 1;
}

static int l_leo_actor_emitter_const(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushlightuserdata(L, (void *)leo_actor_emitter_const(actor));
    return 1;
}

static int l_leo_actor_userdata(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(actor);
    void *ptr = binding ? binding->user_ptr : leo_actor_userdata(actor);
    if (ptr)
        lua_pushlightuserdata(L, ptr);
    else
        lua_pushnil(L);
    return 1;
}

static int l_leo_actor_set_userdata(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    void *ptr = lua_isnoneornil(L, 2) ? NULL : lua_touserdata(L, 2);
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(actor);
    if (binding)
        binding->user_ptr = ptr;
    else
        leo_actor_set_userdata(actor, ptr);
    return 0;
}

static int l_leo_actor_for_each_child(lua_State *L)
{
    leo_Actor *parent = lua_check_actor(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    leo__LuaActorEnumCtx ctx = {0};
    ctx.L = L;
    lua_pushvalue(L, 2);
    ctx.ref_func = luaL_ref(L, LUA_REGISTRYINDEX);
    if (!lua_isnoneornil(L, 3))
    {
        lua_pushvalue(L, 3);
        ctx.ref_user = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        ctx.ref_user = LUA_NOREF;
    }

    leo_actor_for_each_child(parent, leo__lua_actor_child_fn, &ctx);

    luaL_unref(L, LUA_REGISTRYINDEX, ctx.ref_func);
    if (ctx.ref_user != LUA_NOREF)
        luaL_unref(L, LUA_REGISTRYINDEX, ctx.ref_user);
    return 0;
}

static int l_leo_actor_set_z(lua_State *L)
{
    leo_Actor *actor = lua_check_actor(L, 1);
    int z = (int)luaL_checkinteger(L, 2);
    leo_actor_set_z(actor, z);
    return 0;
}

static int l_leo_actor_get_z(lua_State *L)
{
    const leo_Actor *actor = lua_check_actor(L, 1);
    lua_pushinteger(L, leo_actor_get_z(actor));
    return 1;
}

static bool leo__lua_actor_on_init(leo_Actor *self)
{
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(self);
    if (!binding)
        return true;
    binding->magic = LEO_LUA_ACTOR_MAGIC;
    lua_State *L = lua_actor_binding_state(binding);
    if (!L)
        return true;
    if (!lua_actor_binding_get_field(L, binding, "on_init"))
        return true;
    lua_pushlightuserdata(L, self);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_actor on_init error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        return true;
    }
    bool ok = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return ok;
}

static void leo__lua_actor_on_update(leo_Actor *self, float dt)
{
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(self);
    if (!binding)
        return;
    lua_State *L = lua_actor_binding_state(binding);
    if (!L || !lua_actor_binding_get_field(L, binding, "on_update"))
        return;
    lua_pushlightuserdata(L, self);
    lua_pushnumber(L, dt);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_actor on_update error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void leo__lua_actor_on_render(leo_Actor *self)
{
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(self);
    if (!binding)
        return;
    lua_State *L = lua_actor_binding_state(binding);
    if (!L || !lua_actor_binding_get_field(L, binding, "on_render"))
        return;
    lua_pushlightuserdata(L, self);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_actor on_render error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

static void leo__lua_actor_on_exit(leo_Actor *self)
{
    leo__LuaActorBinding *binding = lua_actor_binding_from_actor(self);
    if (!binding)
        return;
    lua_State *L = lua_actor_binding_state(binding);
    if (L && lua_actor_binding_get_field(L, binding, "on_exit"))
    {
        lua_pushlightuserdata(L, self);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "leo_actor on_exit error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    if (!L)
        L = binding->sys_ud ? binding->sys_ud->L : NULL;
    if (L)
        lua_actor_binding_release(L, binding);
    else
        free(binding);
    leo_actor_set_userdata(self, NULL);
}


// -----------------------------------------------------------------------------
// Tiled bindings
// -----------------------------------------------------------------------------
static int l_leo_tiled_load(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    leo_TiledLoadOptions opt;
    leo_TiledLoadOptions *opt_ptr = NULL;
    if (!lua_isnoneornil(L, 2))
    {
        luaL_checktype(L, 2, LUA_TTABLE);
        memset(&opt, 0, sizeof(opt));
        opt.allow_compression = 1;
        lua_getfield(L, 2, "image_base");
        if (!lua_isnil(L, -1))
            opt.image_base = luaL_checkstring(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 2, "allow_compression");
        if (!lua_isnil(L, -1))
            opt.allow_compression = lua_toboolean(L, -1) ? 1 : 0;
        lua_pop(L, 1);

        lua_getfield(L, 2, "remap_image");
        if (!lua_isnil(L, -1))
        {
            return luaL_error(L, "leo_tiled_load: remap_image option is not supported in Lua bindings");
        }
        lua_pop(L, 1);

        opt_ptr = &opt;
    }

    leo_TiledMap *map = leo_tiled_load(path, opt_ptr);
    if (!map)
    {
        const char *err = leo_GetError();
        lua_pushnil(L);
        lua_pushstring(L, (err && *err) ? err : "leo_tiled_load failed");
        return 2;
    }

    push_tiled_map(L, map, 1);
    return 1;
}

static int l_leo_tiled_free(lua_State *L)
{
    leo__LuaTiledMap *ud = check_tiled_map_ud(L, 1);
    if (ud->map)
    {
        leo_tiled_free(ud->map);
        ud->map = NULL;
    }
    ud->owns = 0;
    return 0;
}

static int l_leo_tiled_map_get_size(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_pushinteger(L, map->width);
    lua_pushinteger(L, map->height);
    return 2;
}

static int l_leo_tiled_map_get_tile_size(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_pushinteger(L, map->tilewidth);
    lua_pushinteger(L, map->tileheight);
    return 2;
}

static int l_leo_tiled_map_get_metadata(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_createtable(L, 0, 2);
    lua_pushstring(L, map->orientation ? map->orientation : "");
    lua_setfield(L, -2, "orientation");
    lua_pushstring(L, map->renderorder ? map->renderorder : "");
    lua_setfield(L, -2, "renderorder");
    return 1;
}

static int l_leo_tiled_map_get_properties(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_push_tiled_properties(L, map->props, map->prop_count);
    return 1;
}

static int l_leo_tiled_map_tile_layer_count(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_pushinteger(L, leo_tiled_count_layers_of_type(map, LEO_TILED_LAYER_TILE));
    return 1;
}

static int l_leo_tiled_map_object_layer_count(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_pushinteger(L, leo_tiled_count_layers_of_type(map, LEO_TILED_LAYER_OBJECT));
    return 1;
}

static int l_leo_tiled_map_get_tile_layer(lua_State *L)
{
    leo__LuaTiledMap *map_ud = check_tiled_map_ud(L, 1);
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    int idx = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, idx >= 1, 2, "index must be >= 1");
    const leo_TiledTileLayer *layer = leo_tiled_get_tile_layer_at(map, idx);
    if (!layer)
    {
        lua_pushnil(L);
        return 1;
    }
    int abs_map = lua_absindex(L, 1);
    push_tiled_tile_layer(L, map_ud, layer, abs_map);
    return 1;
}

static int l_leo_tiled_map_get_object_layer(lua_State *L)
{
    leo__LuaTiledMap *map_ud = check_tiled_map_ud(L, 1);
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    int idx = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, idx >= 1, 2, "index must be >= 1");
    const leo_TiledObjectLayer *layer = leo_tiled_get_object_layer_at(map, idx);
    if (!layer)
    {
        lua_pushnil(L);
        return 1;
    }
    int abs_map = lua_absindex(L, 1);
    push_tiled_object_layer(L, map_ud, layer, abs_map);
    return 1;
}

static int l_leo_tiled_find_tile_layer(lua_State *L)
{
    leo__LuaTiledMap *map_ud = check_tiled_map_ud(L, 1);
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    const char *name = luaL_checkstring(L, 2);
    const leo_TiledTileLayer *layer = leo_tiled_find_tile_layer(map, name);
    if (!layer)
    {
        lua_pushnil(L);
        return 1;
    }
    int abs_map = lua_absindex(L, 1);
    push_tiled_tile_layer(L, map_ud, layer, abs_map);
    return 1;
}

static int l_leo_tiled_find_object_layer(lua_State *L)
{
    leo__LuaTiledMap *map_ud = check_tiled_map_ud(L, 1);
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    const char *name = luaL_checkstring(L, 2);
    const leo_TiledObjectLayer *layer = leo_tiled_find_object_layer(map, name);
    if (!layer)
    {
        lua_pushnil(L);
        return 1;
    }
    int abs_map = lua_absindex(L, 1);
    push_tiled_object_layer(L, map_ud, layer, abs_map);
    return 1;
}

static int l_leo_tiled_tile_layer_get_name(lua_State *L)
{
    const leo_TiledTileLayer *layer = lua_check_tiled_tile_layer(L, 1);
    lua_pushstring(L, layer->name ? layer->name : "");
    return 1;
}

static int l_leo_tiled_tile_layer_get_size(lua_State *L)
{
    const leo_TiledTileLayer *layer = lua_check_tiled_tile_layer(L, 1);
    lua_pushinteger(L, layer->width);
    lua_pushinteger(L, layer->height);
    return 2;
}

static int l_leo_tiled_get_gid(lua_State *L)
{
    const leo_TiledTileLayer *layer = lua_check_tiled_tile_layer(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    lua_pushinteger(L, (lua_Integer)leo_tiled_get_gid(layer, x, y));
    return 1;
}

static int l_leo_tiled_object_layer_get_name(lua_State *L)
{
    const leo_TiledObjectLayer *layer = lua_check_tiled_object_layer(L, 1);
    lua_pushstring(L, layer->name ? layer->name : "");
    return 1;
}

static int l_leo_tiled_object_layer_get_count(lua_State *L)
{
    const leo_TiledObjectLayer *layer = lua_check_tiled_object_layer(L, 1);
    lua_pushinteger(L, layer->object_count);
    return 1;
}

static int l_leo_tiled_object_layer_get_object(lua_State *L)
{
    const leo_TiledObjectLayer *layer = lua_check_tiled_object_layer(L, 1);
    int idx = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, idx >= 1 && idx <= layer->object_count, 2, "index out of range");
    lua_push_tiled_object(L, &layer->objects[idx - 1]);
    return 1;
}

static int l_leo_tiled_tileset_count(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    lua_pushinteger(L, map->tileset_count);
    return 1;
}

static int l_leo_tiled_map_get_tileset(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    int idx = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, idx >= 1 && idx <= map->tileset_count, 2, "index out of range");
    lua_push_tiled_tileset(L, &map->tilesets[idx - 1]);
    return 1;
}

static int l_leo_tiled_gid_info(lua_State *L)
{
    uint32_t gid = (uint32_t)luaL_checkinteger(L, 1);
    leo_TiledGidInfo info = leo_tiled_gid_info(gid);
    lua_createtable(L, 0, 5);
    lua_pushinteger(L, info.gid_raw);
    lua_setfield(L, -2, "gid_raw");
    lua_pushinteger(L, info.id);
    lua_setfield(L, -2, "id");
    lua_pushboolean(L, info.flip_h);
    lua_setfield(L, -2, "flip_h");
    lua_pushboolean(L, info.flip_v);
    lua_setfield(L, -2, "flip_v");
    lua_pushboolean(L, info.flip_d);
    lua_setfield(L, -2, "flip_d");
    return 1;
}

static int l_leo_tiled_resolve_gid(lua_State *L)
{
    leo_TiledMap *map = lua_check_tiled_map(L, 1);
    uint32_t gid = (uint32_t)luaL_checkinteger(L, 2);
    const leo_TiledTileset *ts = NULL;
    leo_Rectangle src;
    if (!leo_tiled_resolve_gid(map, gid, &ts, &src))
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 1);
    lua_push_tiled_tileset(L, ts);
    lua_createtable(L, 0, 4);
    lua_pushnumber(L, src.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, src.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, src.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, src.height);
    lua_setfield(L, -2, "height");
    return 3;
}

// -----------------------------------------------------------------------------
// Collision bindings
// -----------------------------------------------------------------------------
static int l_leo_check_collision_recs(lua_State *L)
{
    leo_Rectangle a = lua_read_rect_args(L, 1);
    leo_Rectangle b = lua_read_rect_args(L, 5);
    lua_pushboolean(L, leo_CheckCollisionRecs(a, b));
    return 1;
}

static int l_leo_check_collision_circles(lua_State *L)
{
    leo_Vector2 c1 = lua_read_vec2_args(L, 1);
    float r1 = (float)luaL_checknumber(L, 3);
    leo_Vector2 c2 = lua_read_vec2_args(L, 4);
    float r2 = (float)luaL_checknumber(L, 6);
    lua_pushboolean(L, leo_CheckCollisionCircles(c1, r1, c2, r2));
    return 1;
}

static int l_leo_check_collision_circle_rec(lua_State *L)
{
    leo_Vector2 center = lua_read_vec2_args(L, 1);
    float radius = (float)luaL_checknumber(L, 3);
    leo_Rectangle rec = lua_read_rect_args(L, 4);
    lua_pushboolean(L, leo_CheckCollisionCircleRec(center, radius, rec));
    return 1;
}

static int l_leo_check_collision_circle_line(lua_State *L)
{
    leo_Vector2 center = lua_read_vec2_args(L, 1);
    float radius = (float)luaL_checknumber(L, 3);
    leo_Vector2 p1 = lua_read_vec2_args(L, 4);
    leo_Vector2 p2 = lua_read_vec2_args(L, 6);
    lua_pushboolean(L, leo_CheckCollisionCircleLine(center, radius, p1, p2));
    return 1;
}

static int l_leo_check_collision_point_rec(lua_State *L)
{
    leo_Vector2 point = lua_read_vec2_args(L, 1);
    leo_Rectangle rec = lua_read_rect_args(L, 3);
    lua_pushboolean(L, leo_CheckCollisionPointRec(point, rec));
    return 1;
}

static int l_leo_check_collision_point_circle(lua_State *L)
{
    leo_Vector2 point = lua_read_vec2_args(L, 1);
    leo_Vector2 center = lua_read_vec2_args(L, 3);
    float radius = (float)luaL_checknumber(L, 5);
    lua_pushboolean(L, leo_CheckCollisionPointCircle(point, center, radius));
    return 1;
}

static int l_leo_check_collision_point_triangle(lua_State *L)
{
    leo_Vector2 point = lua_read_vec2_args(L, 1);
    leo_Vector2 p1 = lua_read_vec2_args(L, 3);
    leo_Vector2 p2 = lua_read_vec2_args(L, 5);
    leo_Vector2 p3 = lua_read_vec2_args(L, 7);
    lua_pushboolean(L, leo_CheckCollisionPointTriangle(point, p1, p2, p3));
    return 1;
}

static int l_leo_check_collision_point_line(lua_State *L)
{
    leo_Vector2 point = lua_read_vec2_args(L, 1);
    leo_Vector2 p1 = lua_read_vec2_args(L, 3);
    leo_Vector2 p2 = lua_read_vec2_args(L, 5);
    float threshold = (float)luaL_checknumber(L, 7);
    lua_pushboolean(L, leo_CheckCollisionPointLine(point, p1, p2, threshold));
    return 1;
}

static int l_leo_check_collision_lines(lua_State *L)
{
    leo_Vector2 start1 = lua_read_vec2_args(L, 1);
    leo_Vector2 end1 = lua_read_vec2_args(L, 3);
    leo_Vector2 start2 = lua_read_vec2_args(L, 5);
    leo_Vector2 end2 = lua_read_vec2_args(L, 7);
    leo_Vector2 point;
    bool hit = leo_CheckCollisionLines(start1, end1, start2, end2, &point);
    lua_pushboolean(L, hit);
    if (hit)
    {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, point.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, point.y);
        lua_setfield(L, -2, "y");
    }
    else
    {
        lua_pushnil(L);
    }
    return 2;
}

static int l_leo_get_collision_rec(lua_State *L)
{
    leo_Rectangle a = lua_read_rect_args(L, 1);
    leo_Rectangle b = lua_read_rect_args(L, 5);
    leo_Rectangle result = leo_GetCollisionRec(a, b);
    lua_push_rectangle_table(L, result);
    return 1;
}

// -----------------------------------------------------------------------------
// Base64 bindings
// -----------------------------------------------------------------------------
static const char *lua_b64_result_string(leo_b64_result res)
{
    switch (res)
    {
    case LEO_B64_OK:
        return "ok";
    case LEO_B64_E_ARG:
        return "invalid arguments";
    case LEO_B64_E_NOSPACE:
        return "buffer too small";
    case LEO_B64_E_FORMAT:
        return "invalid format";
    default:
        return "unknown";
    }
}

static int l_leo_base64_encoded_len(lua_State *L)
{
    size_t n = (size_t)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)leo_base64_encoded_len(n));
    return 1;
}

static int l_leo_base64_decoded_cap(lua_State *L)
{
    size_t n = (size_t)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)leo_base64_decoded_cap(n));
    return 1;
}

static int l_leo_base64_encode(lua_State *L)
{
    size_t len = 0;
    const char *src = luaL_checklstring(L, 1, &len);
    char *out = NULL;
    size_t out_len = 0;
    leo_b64_result res = leo_base64_encode_alloc(src, len, &out, &out_len);
    if (res != LEO_B64_OK)
    {
        lua_pushnil(L);
        lua_pushstring(L, lua_b64_result_string(res));
        return 2;
    }
    lua_pushlstring(L, out, out_len);
    free(out);
    return 1;
}

static int l_leo_base64_decode(lua_State *L)
{
    size_t len = 0;
    const char *src = luaL_checklstring(L, 1, &len);
    unsigned char *out = NULL;
    size_t out_len = 0;
    leo_b64_result res = leo_base64_decode_alloc(src, len, &out, &out_len);
    if (res != LEO_B64_OK)
    {
        lua_pushnil(L);
        lua_pushstring(L, lua_b64_result_string(res));
        return 2;
    }
    lua_pushlstring(L, (const char *)out, out_len);
    free(out);
    return 1;
}

// -----------------------------------------------------------------------------
// Animation bindings
// -----------------------------------------------------------------------------
static int l_leo_load_animation(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    int frame_w = (int)luaL_checkinteger(L, 2);
    int frame_h = (int)luaL_checkinteger(L, 3);
    int frame_count = (int)luaL_checkinteger(L, 4);
    float frame_time = (float)luaL_checknumber(L, 5);
    bool loop = lua_toboolean(L, 6) != 0;
    leo_Animation anim = leo_LoadAnimation(path, frame_w, frame_h, frame_count, frame_time, loop);
    return push_animation(L, anim, 1) ? 1 : luaL_error(L, "leo_load_animation: allocation failed");
}

static int l_leo_unload_animation(lua_State *L)
{
    leo__LuaAnimation *ud = check_animation_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadAnimation(&ud->anim);
        ud->owns = 0;
        ud->anim.texture.width = 0;
        ud->anim.texture.height = 0;
        ud->anim.texture._handle = NULL;
    }
    return 0;
}

static int l_leo_create_animation_player(lua_State *L)
{
    leo__LuaAnimation *anim_ud = check_animation_ud(L, 1);
    leo_AnimationPlayer player = leo_CreateAnimationPlayer(&anim_ud->anim);
    leo__LuaAnimationPlayer *player_ud = push_animation_player(L, player);
    int anim_index = lua_absindex(L, 1);
    lua_pushvalue(L, anim_index);
    lua_setiuservalue(L, -2, 1);
    return 1;
}

static int l_leo_update_animation(lua_State *L)
{
    leo__LuaAnimationPlayer *player_ud = check_animation_player_ud(L, 1);
    float dt = (float)luaL_checknumber(L, 2);
    leo_UpdateAnimation(&player_ud->player, dt);
    return 0;
}

static int l_leo_draw_animation(lua_State *L)
{
    leo__LuaAnimationPlayer *player_ud = check_animation_player_ud(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    leo_DrawAnimation(&player_ud->player, x, y);
    return 0;
}

static int l_leo_play_animation(lua_State *L)
{
    leo__LuaAnimationPlayer *player_ud = check_animation_player_ud(L, 1);
    leo_PlayAnimation(&player_ud->player);
    return 0;
}

static int l_leo_pause_animation(lua_State *L)
{
    leo__LuaAnimationPlayer *player_ud = check_animation_player_ud(L, 1);
    leo_PauseAnimation(&player_ud->player);
    return 0;
}

static int l_leo_reset_animation(lua_State *L)
{
    leo__LuaAnimationPlayer *player_ud = check_animation_player_ud(L, 1);
    leo_ResetAnimation(&player_ud->player);
    return 0;
}

// -----------------------------------------------------------------------------
// Audio bindings
// -----------------------------------------------------------------------------
static int l_leo_init_audio(lua_State *L)
{
    lua_pushboolean(L, leo_InitAudio());
    return 1;
}

static int l_leo_shutdown_audio(lua_State *L)
{
    (void)L;
    leo_ShutdownAudio();
    return 0;
}

static leo__LuaSound *push_sound(lua_State *L, leo_Sound sound, int owns)
{
    leo__LuaSound *ud = (leo__LuaSound *)lua_newuserdata(L, sizeof(leo__LuaSound));
    ud->sound = sound;
    ud->owns = owns;
    luaL_getmetatable(L, LEO_SOUND_META);
    lua_setmetatable(L, -2);
    return ud;
}

static leo__LuaSound *check_sound_ud(lua_State *L, int idx)
{
    return (leo__LuaSound *)luaL_checkudata(L, idx, LEO_SOUND_META);
}

static int l_leo_load_sound(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    leo_Sound sound = leo_LoadSound(path);
    return push_sound(L, sound, 1) ? 1 : luaL_error(L, "leo_load_sound: allocation failed");
}

static int l_leo_unload_sound(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    if (ud->owns)
    {
        leo_UnloadSound(&ud->sound);
        ud->owns = 0;
        ud->sound._handle = NULL;
    }
    return 0;
}

static int l_leo_is_sound_ready(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    lua_pushboolean(L, leo_IsSoundReady(ud->sound));
    return 1;
}

static int l_leo_play_sound(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    float volume = (float)luaL_optnumber(L, 2, 1.0);
    bool loop = lua_toboolean(L, 3) != 0;
    lua_pushboolean(L, leo_PlaySound(&ud->sound, volume, loop));
    return 1;
}

static int l_leo_stop_sound(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    leo_StopSound(&ud->sound);
    return 0;
}

static int l_leo_pause_sound(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    leo_PauseSound(&ud->sound);
    return 0;
}

static int l_leo_resume_sound(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    leo_ResumeSound(&ud->sound);
    return 0;
}

static int l_leo_is_sound_playing(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    lua_pushboolean(L, leo_IsSoundPlaying(&ud->sound));
    return 1;
}

static int l_leo_set_sound_volume(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    float vol = (float)luaL_checknumber(L, 2);
    leo_SetSoundVolume(&ud->sound, vol);
    return 0;
}

static int l_leo_set_sound_pitch(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    float pitch = (float)luaL_checknumber(L, 2);
    leo_SetSoundPitch(&ud->sound, pitch);
    return 0;
}

static int l_leo_set_sound_pan(lua_State *L)
{
    leo__LuaSound *ud = check_sound_ud(L, 1);
    float pan = (float)luaL_checknumber(L, 2);
    leo_SetSoundPan(&ud->sound, pan);
    return 0;
}

// -----------------------------------------------------------------------------
// Transition bindings
// -----------------------------------------------------------------------------
static int l_leo_start_fade_in(lua_State *L)
{
    float duration = (float)luaL_checknumber(L, 1);
    leo_Color color = lua_check_color_param(L, 2, NULL);
    lua_transition_clear_callback();
    leo_StartFadeIn(duration, color);
    return 0;
}

static int l_leo_start_fade_out(lua_State *L)
{
    float duration = (float)luaL_checknumber(L, 1);
    int next_idx = 0;
    leo_Color color = lua_check_color_param(L, 2, &next_idx);
    lua_transition_set_callback(L, next_idx);
    leo_StartFadeOut(duration, color, (g_transition_callback.ref == LUA_NOREF) ? NULL : leo__lua_transition_on_complete);
    return 0;
}

static int l_leo_start_transition(lua_State *L)
{
    int type = (int)luaL_checkinteger(L, 1);
    float duration = (float)luaL_checknumber(L, 2);
    int next_idx = 0;
    leo_Color color = lua_check_color_param(L, 3, &next_idx);
    lua_transition_set_callback(L, next_idx);
    leo_StartTransition((leo_TransitionType)type, duration, color,
                        (g_transition_callback.ref == LUA_NOREF) ? NULL : leo__lua_transition_on_complete);
    return 0;
}

static int l_leo_update_transitions(lua_State *L)
{
    float dt = (float)luaL_checknumber(L, 1);
    leo_UpdateTransitions(dt);
    return 0;
}

static int l_leo_render_transitions(lua_State *L)
{
    (void)L;
    leo_RenderTransitions();
    return 0;
}

static int l_leo_is_transitioning(lua_State *L)
{
    lua_pushboolean(L, leo_IsTransitioning());
    return 1;
}

// -----------------------------------------------------------------------------
// Logical resolution
// -----------------------------------------------------------------------------
static int l_leo_set_logical_resolution(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int presentation = (int)luaL_checkinteger(L, 3);
    int scale = (int)luaL_checkinteger(L, 4);
    lua_pushboolean(L, leo_SetLogicalResolution(width, height,
                                                (leo_LogicalPresentation)presentation,
                                                (leo_ScaleMode)scale));
    return 1;
}

int leo_LuaOpenBindings(void *vL)
{
    lua_State *L = (lua_State *)vL;

    register_texture_mt(L);
    register_render_texture_mt(L);
    register_font_mt(L);
    register_json_doc_mt(L);
    register_json_node_mt(L);
    register_tiled_map_mt(L);
    register_tiled_tile_layer_mt(L);
    register_tiled_object_layer_mt(L);
    register_actor_system_mt(L);
    register_animation_mt(L);
    register_animation_player_mt(L);
    register_sound_mt(L);

    static const luaL_Reg leo_funcs[] = {
        {"leo_init_window", l_leo_init_window},
        {"leo_close_window", l_leo_close_window},
        {"leo_get_window", l_leo_get_window},
        {"leo_get_renderer", l_leo_get_renderer},
        {"leo_set_fullscreen", l_leo_set_fullscreen},
        {"leo_set_window_mode", l_leo_set_window_mode},
        {"leo_get_window_mode", l_leo_get_window_mode},
        {"leo_window_should_close", l_leo_window_should_close},
        {"leo_begin_drawing", l_leo_begin_drawing},
        {"leo_end_drawing", l_leo_end_drawing},
        {"leo_clear_background", l_leo_clear_background},
        {"leo_set_target_fps", l_leo_set_target_fps},
        {"leo_get_frame_time", l_leo_get_frame_time},
        {"leo_get_time", l_leo_get_time},
        {"leo_get_fps", l_leo_get_fps},
        {"leo_get_screen_width", l_leo_get_screen_width},
        {"leo_get_screen_height", l_leo_get_screen_height},
        {"leo_is_key_pressed", l_leo_is_key_pressed},
        {"leo_is_key_pressed_repeat", l_leo_is_key_pressed_repeat},
        {"leo_is_key_down", l_leo_is_key_down},
        {"leo_is_key_released", l_leo_is_key_released},
        {"leo_is_key_up", l_leo_is_key_up},
        {"leo_get_key_pressed", l_leo_get_key_pressed},
        {"leo_get_char_pressed", l_leo_get_char_pressed},
        {"leo_set_exit_key", l_leo_set_exit_key},
        {"leo_update_keyboard", l_leo_update_keyboard},
        {"leo_is_exit_key_pressed", l_leo_is_exit_key_pressed},
        {"leo_cleanup_keyboard", l_leo_cleanup_keyboard},
        {"leo_actor_system_create", l_leo_actor_system_create},
        {"leo_actor_system_destroy", l_leo_actor_system_destroy},
        {"leo_actor_system_update", l_leo_actor_system_update},
        {"leo_actor_system_render", l_leo_actor_system_render},
        {"leo_actor_system_set_paused", l_leo_actor_system_set_paused},
        {"leo_actor_system_is_paused", l_leo_actor_system_is_paused},
        {"leo_actor_system_root", l_leo_actor_system_root},
        {"leo_actor_spawn", l_leo_actor_spawn},
        {"leo_actor_kill", l_leo_actor_kill},
        {"leo_actor_uid", l_leo_actor_uid},
        {"leo_actor_name", l_leo_actor_name},
        {"leo_actor_parent", l_leo_actor_parent},
        {"leo_actor_find_child_by_name", l_leo_actor_find_child_by_name},
        {"leo_actor_find_recursive_by_name", l_leo_actor_find_recursive_by_name},
        {"leo_actor_find_by_uid", l_leo_actor_find_by_uid},
        {"leo_actor_group_get_or_create", l_leo_actor_group_get_or_create},
        {"leo_actor_group_find", l_leo_actor_group_find},
        {"leo_actor_add_to_group", l_leo_actor_add_to_group},
        {"leo_actor_remove_from_group", l_leo_actor_remove_from_group},
        {"leo_actor_in_group", l_leo_actor_in_group},
        {"leo_actor_groups", l_leo_actor_groups},
        {"leo_actor_for_each_in_group", l_leo_actor_for_each_in_group},
        {"leo_actor_set_paused", l_leo_actor_set_paused},
        {"leo_actor_is_paused", l_leo_actor_is_paused},
        {"leo_actor_is_effectively_paused", l_leo_actor_is_effectively_paused},
        {"leo_actor_emitter", l_leo_actor_emitter},
        {"leo_actor_emitter_const", l_leo_actor_emitter_const},
        {"leo_actor_userdata", l_leo_actor_userdata},
        {"leo_actor_set_userdata", l_leo_actor_set_userdata},
        {"leo_actor_for_each_child", l_leo_actor_for_each_child},
        {"leo_actor_set_z", l_leo_actor_set_z},
        {"leo_actor_get_z", l_leo_actor_get_z},
        {"leo_is_touch_down", l_leo_is_touch_down},
        {"leo_is_touch_pressed", l_leo_is_touch_pressed},
        {"leo_is_touch_released", l_leo_is_touch_released},
        {"leo_get_touch_position", l_leo_get_touch_position},
        {"leo_get_touch_x", l_leo_get_touch_x},
        {"leo_get_touch_y", l_leo_get_touch_y},
        {"leo_get_touch_point_count", l_leo_get_touch_point_count},
        {"leo_get_touch_point_id", l_leo_get_touch_point_id},
        {"leo_is_gesture_detected", l_leo_is_gesture_detected},
        {"leo_get_gesture_detected", l_leo_get_gesture_detected},
        {"leo_get_gesture_hold_duration", l_leo_get_gesture_hold_duration},
        {"leo_get_gesture_drag_vector", l_leo_get_gesture_drag_vector},
        {"leo_get_gesture_drag_angle", l_leo_get_gesture_drag_angle},
        {"leo_get_gesture_pinch_vector", l_leo_get_gesture_pinch_vector},
        {"leo_get_gesture_pinch_angle", l_leo_get_gesture_pinch_angle},
        {"leo_set_gestures_enabled", l_leo_set_gestures_enabled},
        {"leo_init_gamepads", l_leo_init_gamepads},
        {"leo_update_gamepads", l_leo_update_gamepads},
        {"leo_handle_gamepad_event", l_leo_handle_gamepad_event},
        {"leo_shutdown_gamepads", l_leo_shutdown_gamepads},
        {"leo_is_gamepad_available", l_leo_is_gamepad_available},
        {"leo_get_gamepad_name", l_leo_get_gamepad_name},
        {"leo_is_gamepad_button_pressed", l_leo_is_gamepad_button_pressed},
        {"leo_is_gamepad_button_down", l_leo_is_gamepad_button_down},
        {"leo_is_gamepad_button_released", l_leo_is_gamepad_button_released},
        {"leo_is_gamepad_button_up", l_leo_is_gamepad_button_up},
        {"leo_get_gamepad_button_pressed", l_leo_get_gamepad_button_pressed},
        {"leo_get_gamepad_axis_count", l_leo_get_gamepad_axis_count},
        {"leo_get_gamepad_axis_movement", l_leo_get_gamepad_axis_movement},
        {"leo_set_gamepad_vibration", l_leo_set_gamepad_vibration},
        {"leo_set_gamepad_axis_deadzone", l_leo_set_gamepad_axis_deadzone},
        {"leo_set_gamepad_stick_threshold", l_leo_set_gamepad_stick_threshold},
        {"leo_get_gamepad_stick", l_leo_get_gamepad_stick},
        {"leo_is_gamepad_stick_pressed", l_leo_is_gamepad_stick_pressed},
        {"leo_is_gamepad_stick_down", l_leo_is_gamepad_stick_down},
        {"leo_is_gamepad_stick_released", l_leo_is_gamepad_stick_released},
        {"leo_is_gamepad_stick_up", l_leo_is_gamepad_stick_up},
        {"leo_begin_mode2d", l_leo_begin_mode2d},
        {"leo_end_mode2d", l_leo_end_mode2d},
        {"leo_is_camera_active", l_leo_is_camera_active},
        {"leo_get_world_to_screen2d", l_leo_get_world_to_screen2d},
        {"leo_get_screen_to_world2d", l_leo_get_screen_to_world2d},
        {"leo_get_current_camera2d", l_leo_get_current_camera2d},
        {"leo_load_render_texture", l_leo_load_render_texture},
        {"leo_unload_render_texture", l_leo_unload_render_texture},
        {"leo_begin_texture_mode", l_leo_begin_texture_mode},
        {"leo_end_texture_mode", l_leo_end_texture_mode},
        {"leo_render_texture_get_texture", l_leo_render_texture_get_texture},
        {"leo_render_texture_get_size", l_leo_render_texture_get_size},
        {"leo_image_load", l_image_load},
        {"leo_image_load_from_memory", l_image_load_from_memory},
        {"leo_image_load_from_texture", l_image_load_from_texture},
        {"leo_image_load_from_pixels", l_image_load_from_pixels},
        {"leo_image_is_ready", l_image_is_ready},
        {"leo_image_unload", l_image_unload},
        {"leo_image_bytes_per_pixel", l_image_bytes_per_pixel},
        {"leo_texture_get_size", l_leo_texture_get_size},
        {"leo_draw_texture_rec", l_leo_draw_texture_rec},
        {"leo_draw_texture_pro", l_leo_draw_texture_pro},
        {"leo_load_font", l_leo_load_font},
        {"leo_load_font_from_memory", l_leo_load_font_from_memory},
        {"leo_unload_font", l_leo_unload_font},
        {"leo_is_font_ready", l_leo_is_font_ready},
        {"leo_set_default_font", l_leo_set_default_font},
        {"leo_get_default_font", l_leo_get_default_font},
        {"leo_draw_fps", l_leo_draw_fps},
        {"leo_draw_text", l_leo_draw_text},
        {"leo_draw_text_ex", l_leo_draw_text_ex},
        {"leo_draw_text_pro", l_leo_draw_text_pro},
        {"leo_measure_text_ex", l_leo_measure_text_ex},
        {"leo_measure_text", l_leo_measure_text},
        {"leo_get_font_line_height", l_leo_get_font_line_height},
        {"leo_get_font_base_size", l_leo_get_font_base_size},
        {"leo_json_parse", l_leo_json_parse},
        {"leo_json_load", l_leo_json_load},
        {"leo_json_free", l_leo_json_free},
        {"leo_json_root", l_leo_json_root},
        {"leo_json_is_null", l_leo_json_is_null},
        {"leo_json_is_object", l_leo_json_is_object},
        {"leo_json_is_array", l_leo_json_is_array},
        {"leo_json_is_string", l_leo_json_is_string},
        {"leo_json_is_number", l_leo_json_is_number},
        {"leo_json_is_bool", l_leo_json_is_bool},
        {"leo_json_obj_get", l_leo_json_obj_get},
        {"leo_json_arr_size", l_leo_json_arr_size},
        {"leo_json_arr_get", l_leo_json_arr_get},
        {"leo_json_get_string", l_leo_json_get_string},
        {"leo_json_get_int", l_leo_json_get_int},
        {"leo_json_get_double", l_leo_json_get_double},
        {"leo_json_get_bool", l_leo_json_get_bool},
        {"leo_json_as_string", l_leo_json_as_string},
        {"leo_json_as_int", l_leo_json_as_int},
        {"leo_json_as_double", l_leo_json_as_double},
        {"leo_json_as_bool", l_leo_json_as_bool},
        {"leo_tiled_load", l_leo_tiled_load},
        {"leo_tiled_free", l_leo_tiled_free},
        {"leo_tiled_map_get_size", l_leo_tiled_map_get_size},
        {"leo_tiled_map_get_tile_size", l_leo_tiled_map_get_tile_size},
        {"leo_tiled_map_get_metadata", l_leo_tiled_map_get_metadata},
        {"leo_tiled_map_get_properties", l_leo_tiled_map_get_properties},
        {"leo_tiled_map_tile_layer_count", l_leo_tiled_map_tile_layer_count},
        {"leo_tiled_map_object_layer_count", l_leo_tiled_map_object_layer_count},
        {"leo_tiled_map_get_tile_layer", l_leo_tiled_map_get_tile_layer},
        {"leo_tiled_map_get_object_layer", l_leo_tiled_map_get_object_layer},
        {"leo_tiled_find_tile_layer", l_leo_tiled_find_tile_layer},
        {"leo_tiled_find_object_layer", l_leo_tiled_find_object_layer},
        {"leo_tiled_tile_layer_get_name", l_leo_tiled_tile_layer_get_name},
        {"leo_tiled_tile_layer_get_size", l_leo_tiled_tile_layer_get_size},
        {"leo_tiled_get_gid", l_leo_tiled_get_gid},
        {"leo_tiled_object_layer_get_name", l_leo_tiled_object_layer_get_name},
        {"leo_tiled_object_layer_get_count", l_leo_tiled_object_layer_get_count},
        {"leo_tiled_object_layer_get_object", l_leo_tiled_object_layer_get_object},
        {"leo_tiled_tileset_count", l_leo_tiled_tileset_count},
        {"leo_tiled_map_get_tileset", l_leo_tiled_map_get_tileset},
        {"leo_tiled_gid_info", l_leo_tiled_gid_info},
        {"leo_tiled_resolve_gid", l_leo_tiled_resolve_gid},
        {"leo_check_collision_recs", l_leo_check_collision_recs},
        {"leo_check_collision_circles", l_leo_check_collision_circles},
        {"leo_check_collision_circle_rec", l_leo_check_collision_circle_rec},
        {"leo_check_collision_circle_line", l_leo_check_collision_circle_line},
        {"leo_check_collision_point_rec", l_leo_check_collision_point_rec},
        {"leo_check_collision_point_circle", l_leo_check_collision_point_circle},
        {"leo_check_collision_point_triangle", l_leo_check_collision_point_triangle},
        {"leo_check_collision_point_line", l_leo_check_collision_point_line},
        {"leo_check_collision_lines", l_leo_check_collision_lines},
        {"leo_get_collision_rec", l_leo_get_collision_rec},
        {"leo_base64_encoded_len", l_leo_base64_encoded_len},
        {"leo_base64_decoded_cap", l_leo_base64_decoded_cap},
        {"leo_base64_encode", l_leo_base64_encode},
        {"leo_base64_decode", l_leo_base64_decode},
        {"leo_load_animation", l_leo_load_animation},
        {"leo_unload_animation", l_leo_unload_animation},
        {"leo_create_animation_player", l_leo_create_animation_player},
        {"leo_update_animation", l_leo_update_animation},
        {"leo_draw_animation", l_leo_draw_animation},
        {"leo_play_animation", l_leo_play_animation},
        {"leo_pause_animation", l_leo_pause_animation},
        {"leo_reset_animation", l_leo_reset_animation},
        {"leo_init_audio", l_leo_init_audio},
        {"leo_shutdown_audio", l_leo_shutdown_audio},
        {"leo_load_sound", l_leo_load_sound},
        {"leo_unload_sound", l_leo_unload_sound},
        {"leo_is_sound_ready", l_leo_is_sound_ready},
        {"leo_play_sound", l_leo_play_sound},
        {"leo_stop_sound", l_leo_stop_sound},
        {"leo_pause_sound", l_leo_pause_sound},
        {"leo_resume_sound", l_leo_resume_sound},
        {"leo_is_sound_playing", l_leo_is_sound_playing},
        {"leo_set_sound_volume", l_leo_set_sound_volume},
        {"leo_set_sound_pitch", l_leo_set_sound_pitch},
        {"leo_set_sound_pan", l_leo_set_sound_pan},
        {"leo_start_fade_in", l_leo_start_fade_in},
        {"leo_start_fade_out", l_leo_start_fade_out},
        {"leo_start_transition", l_leo_start_transition},
        {"leo_update_transitions", l_leo_update_transitions},
        {"leo_render_transitions", l_leo_render_transitions},
        {"leo_is_transitioning", l_leo_is_transitioning},
        {"leo_set_logical_resolution", l_leo_set_logical_resolution},
        {NULL, NULL}
    };

    for (const luaL_Reg *reg = leo_funcs; reg->name; ++reg)
    {
        lua_pushcfunction(L, reg->func);
        lua_setglobal(L, reg->name);
    }

    // Constants for texture formats
    lua_pushinteger(L, LEO_TEXFORMAT_UNDEFINED); lua_setglobal(L, "leo_TEXFORMAT_UNDEFINED");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8A8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8A8");
    lua_pushinteger(L, LEO_TEXFORMAT_R8G8B8); lua_setglobal(L, "leo_TEXFORMAT_R8G8B8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8); lua_setglobal(L, "leo_TEXFORMAT_GRAY8");
    lua_pushinteger(L, LEO_TEXFORMAT_GRAY8_ALPHA); lua_setglobal(L, "leo_TEXFORMAT_GRAY8_ALPHA");

    // Window mode constants
    lua_pushinteger(L, LEO_WINDOW_MODE_WINDOWED); lua_setglobal(L, "leo_WINDOW_MODE_WINDOWED");
    lua_pushinteger(L, LEO_WINDOW_MODE_BORDERLESS_FULLSCREEN); lua_setglobal(L, "leo_WINDOW_MODE_BORDERLESS_FULLSCREEN");
    lua_pushinteger(L, LEO_WINDOW_MODE_FULLSCREEN_EXCLUSIVE); lua_setglobal(L, "leo_WINDOW_MODE_FULLSCREEN_EXCLUSIVE");

    // Logical presentation constants
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_DISABLED); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_DISABLED");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_STRETCH); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_STRETCH");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_LETTERBOX); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_LETTERBOX");
    lua_pushinteger(L, LEO_LOGICAL_PRESENTATION_OVERSCAN); lua_setglobal(L, "leo_LOGICAL_PRESENTATION_OVERSCAN");

    // Scale mode constants
    lua_pushinteger(L, LEO_SCALE_NEAREST); lua_setglobal(L, "leo_SCALE_NEAREST");
    lua_pushinteger(L, LEO_SCALE_LINEAR); lua_setglobal(L, "leo_SCALE_LINEAR");
    lua_pushinteger(L, LEO_SCALE_PIXELART); lua_setglobal(L, "leo_SCALE_PIXELART");

    // Tiled GID flags
    lua_pushinteger(L, (lua_Integer)LEO_TILED_FLIP_H); lua_setglobal(L, "leo_TILED_FLIP_H");
    lua_pushinteger(L, (lua_Integer)LEO_TILED_FLIP_V); lua_setglobal(L, "leo_TILED_FLIP_V");
    lua_pushinteger(L, (lua_Integer)LEO_TILED_FLIP_D); lua_setglobal(L, "leo_TILED_FLIP_D");
    lua_pushinteger(L, (lua_Integer)LEO_TILED_GID_MASK); lua_setglobal(L, "leo_TILED_GID_MASK");

    // Base64 result codes
    lua_pushinteger(L, LEO_B64_OK); lua_setglobal(L, "leo_B64_OK");
    lua_pushinteger(L, LEO_B64_E_ARG); lua_setglobal(L, "leo_B64_E_ARG");
    lua_pushinteger(L, LEO_B64_E_NOSPACE); lua_setglobal(L, "leo_B64_E_NOSPACE");
    lua_pushinteger(L, LEO_B64_E_FORMAT); lua_setglobal(L, "leo_B64_E_FORMAT");

    // Transition types
    lua_pushinteger(L, LEO_TRANSITION_FADE_IN); lua_setglobal(L, "leo_TRANSITION_FADE_IN");
    lua_pushinteger(L, LEO_TRANSITION_FADE_OUT); lua_setglobal(L, "leo_TRANSITION_FADE_OUT");
    lua_pushinteger(L, LEO_TRANSITION_CIRCLE_IN); lua_setglobal(L, "leo_TRANSITION_CIRCLE_IN");
    lua_pushinteger(L, LEO_TRANSITION_CIRCLE_OUT); lua_setglobal(L, "leo_TRANSITION_CIRCLE_OUT");

    // Touch/gesture constants
    lua_pushinteger(L, LEO_GESTURE_NONE); lua_setglobal(L, "leo_GESTURE_NONE");
    lua_pushinteger(L, LEO_GESTURE_TAP); lua_setglobal(L, "leo_GESTURE_TAP");
    lua_pushinteger(L, LEO_GESTURE_DOUBLETAP); lua_setglobal(L, "leo_GESTURE_DOUBLETAP");
    lua_pushinteger(L, LEO_GESTURE_HOLD); lua_setglobal(L, "leo_GESTURE_HOLD");
    lua_pushinteger(L, LEO_GESTURE_DRAG); lua_setglobal(L, "leo_GESTURE_DRAG");
    lua_pushinteger(L, LEO_GESTURE_SWIPE_RIGHT); lua_setglobal(L, "leo_GESTURE_SWIPE_RIGHT");
    lua_pushinteger(L, LEO_GESTURE_SWIPE_LEFT); lua_setglobal(L, "leo_GESTURE_SWIPE_LEFT");
    lua_pushinteger(L, LEO_GESTURE_SWIPE_UP); lua_setglobal(L, "leo_GESTURE_SWIPE_UP");
    lua_pushinteger(L, LEO_GESTURE_SWIPE_DOWN); lua_setglobal(L, "leo_GESTURE_SWIPE_DOWN");
    lua_pushinteger(L, LEO_GESTURE_PINCH_IN); lua_setglobal(L, "leo_GESTURE_PINCH_IN");
    lua_pushinteger(L, LEO_GESTURE_PINCH_OUT); lua_setglobal(L, "leo_GESTURE_PINCH_OUT");

#define LEO_SET_KEY_CONST(name)            \
    do                                     \
    {                                      \
        lua_pushinteger(L, name);          \
        lua_setglobal(L, "leo_" #name);    \
    } while (0)

    // Keyboard key constants
    LEO_SET_KEY_CONST(KEY_UNKNOWN);
    LEO_SET_KEY_CONST(KEY_A);
    LEO_SET_KEY_CONST(KEY_B);
    LEO_SET_KEY_CONST(KEY_C);
    LEO_SET_KEY_CONST(KEY_D);
    LEO_SET_KEY_CONST(KEY_E);
    LEO_SET_KEY_CONST(KEY_F);
    LEO_SET_KEY_CONST(KEY_G);
    LEO_SET_KEY_CONST(KEY_H);
    LEO_SET_KEY_CONST(KEY_I);
    LEO_SET_KEY_CONST(KEY_J);
    LEO_SET_KEY_CONST(KEY_K);
    LEO_SET_KEY_CONST(KEY_L);
    LEO_SET_KEY_CONST(KEY_M);
    LEO_SET_KEY_CONST(KEY_N);
    LEO_SET_KEY_CONST(KEY_O);
    LEO_SET_KEY_CONST(KEY_P);
    LEO_SET_KEY_CONST(KEY_Q);
    LEO_SET_KEY_CONST(KEY_R);
    LEO_SET_KEY_CONST(KEY_S);
    LEO_SET_KEY_CONST(KEY_T);
    LEO_SET_KEY_CONST(KEY_U);
    LEO_SET_KEY_CONST(KEY_V);
    LEO_SET_KEY_CONST(KEY_W);
    LEO_SET_KEY_CONST(KEY_X);
    LEO_SET_KEY_CONST(KEY_Y);
    LEO_SET_KEY_CONST(KEY_Z);
    LEO_SET_KEY_CONST(KEY_0);
    LEO_SET_KEY_CONST(KEY_1);
    LEO_SET_KEY_CONST(KEY_2);
    LEO_SET_KEY_CONST(KEY_3);
    LEO_SET_KEY_CONST(KEY_4);
    LEO_SET_KEY_CONST(KEY_5);
    LEO_SET_KEY_CONST(KEY_6);
    LEO_SET_KEY_CONST(KEY_7);
    LEO_SET_KEY_CONST(KEY_8);
    LEO_SET_KEY_CONST(KEY_9);
    LEO_SET_KEY_CONST(KEY_RETURN);
    LEO_SET_KEY_CONST(KEY_ESCAPE);
    LEO_SET_KEY_CONST(KEY_BACKSPACE);
    LEO_SET_KEY_CONST(KEY_TAB);
    LEO_SET_KEY_CONST(KEY_SPACE);
    LEO_SET_KEY_CONST(KEY_LEFT);
    LEO_SET_KEY_CONST(KEY_RIGHT);
    LEO_SET_KEY_CONST(KEY_UP);
    LEO_SET_KEY_CONST(KEY_DOWN);
    LEO_SET_KEY_CONST(KEY_LCTRL);
    LEO_SET_KEY_CONST(KEY_LSHIFT);
    LEO_SET_KEY_CONST(KEY_LALT);
    LEO_SET_KEY_CONST(KEY_RCTRL);
    LEO_SET_KEY_CONST(KEY_RSHIFT);
    LEO_SET_KEY_CONST(KEY_RALT);
    LEO_SET_KEY_CONST(KEY_F1);
    LEO_SET_KEY_CONST(KEY_F2);
    LEO_SET_KEY_CONST(KEY_F3);
    LEO_SET_KEY_CONST(KEY_F4);
    LEO_SET_KEY_CONST(KEY_F5);
    LEO_SET_KEY_CONST(KEY_F6);
    LEO_SET_KEY_CONST(KEY_F7);
    LEO_SET_KEY_CONST(KEY_F8);
    LEO_SET_KEY_CONST(KEY_F9);
    LEO_SET_KEY_CONST(KEY_F10);
    LEO_SET_KEY_CONST(KEY_F11);
    LEO_SET_KEY_CONST(KEY_F12);
    LEO_SET_KEY_CONST(KEY_MINUS);
    LEO_SET_KEY_CONST(KEY_EQUALS);
    LEO_SET_KEY_CONST(KEY_LEFTBRACKET);
    LEO_SET_KEY_CONST(KEY_RIGHTBRACKET);
    LEO_SET_KEY_CONST(KEY_BACKSLASH);
    LEO_SET_KEY_CONST(KEY_SEMICOLON);
    LEO_SET_KEY_CONST(KEY_APOSTROPHE);
    LEO_SET_KEY_CONST(KEY_GRAVE);
    LEO_SET_KEY_CONST(KEY_COMMA);
    LEO_SET_KEY_CONST(KEY_PERIOD);
    LEO_SET_KEY_CONST(KEY_SLASH);
    LEO_SET_KEY_CONST(KEY_INSERT);
    LEO_SET_KEY_CONST(KEY_DELETE);
    LEO_SET_KEY_CONST(KEY_HOME);
    LEO_SET_KEY_CONST(KEY_END);
    LEO_SET_KEY_CONST(KEY_PAGEUP);
    LEO_SET_KEY_CONST(KEY_PAGEDOWN);
    LEO_SET_KEY_CONST(KEY_F13);
    LEO_SET_KEY_CONST(KEY_F14);
    LEO_SET_KEY_CONST(KEY_F15);
    LEO_SET_KEY_CONST(KEY_F16);
    LEO_SET_KEY_CONST(KEY_F17);
    LEO_SET_KEY_CONST(KEY_F18);
    LEO_SET_KEY_CONST(KEY_F19);
    LEO_SET_KEY_CONST(KEY_F20);
    LEO_SET_KEY_CONST(KEY_F21);
    LEO_SET_KEY_CONST(KEY_F22);
    LEO_SET_KEY_CONST(KEY_F23);
    LEO_SET_KEY_CONST(KEY_F24);
    LEO_SET_KEY_CONST(KEY_KP_0);
    LEO_SET_KEY_CONST(KEY_KP_1);
    LEO_SET_KEY_CONST(KEY_KP_2);
    LEO_SET_KEY_CONST(KEY_KP_3);
    LEO_SET_KEY_CONST(KEY_KP_4);
    LEO_SET_KEY_CONST(KEY_KP_5);
    LEO_SET_KEY_CONST(KEY_KP_6);
    LEO_SET_KEY_CONST(KEY_KP_7);
    LEO_SET_KEY_CONST(KEY_KP_8);
    LEO_SET_KEY_CONST(KEY_KP_9);
    LEO_SET_KEY_CONST(KEY_KP_PLUS);
    LEO_SET_KEY_CONST(KEY_KP_MINUS);
    LEO_SET_KEY_CONST(KEY_KP_MULTIPLY);
    LEO_SET_KEY_CONST(KEY_KP_DIVIDE);
    LEO_SET_KEY_CONST(KEY_KP_ENTER);
    LEO_SET_KEY_CONST(KEY_KP_PERIOD);
    LEO_SET_KEY_CONST(KEY_KP_EQUALS);
    LEO_SET_KEY_CONST(KEY_PRINTSCREEN);
    LEO_SET_KEY_CONST(KEY_SCROLLLOCK);
    LEO_SET_KEY_CONST(KEY_PAUSE);
    LEO_SET_KEY_CONST(KEY_MENU);
    LEO_SET_KEY_CONST(KEY_VOLUMEUP);
    LEO_SET_KEY_CONST(KEY_VOLUMEDOWN);
    LEO_SET_KEY_CONST(KEY_MUTE);
    LEO_SET_KEY_CONST(KEY_AUDIONEXT);
    LEO_SET_KEY_CONST(KEY_AUDIOPREV);
    LEO_SET_KEY_CONST(KEY_AUDIOSTOP);
    LEO_SET_KEY_CONST(KEY_AUDIOPLAY);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL1);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL2);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL3);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL4);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL5);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL6);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL7);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL8);
    LEO_SET_KEY_CONST(KEY_INTERNATIONAL9);
    LEO_SET_KEY_CONST(KEY_LANG1);
    LEO_SET_KEY_CONST(KEY_LANG2);
    LEO_SET_KEY_CONST(KEY_LANG3);
    LEO_SET_KEY_CONST(KEY_LANG4);
    LEO_SET_KEY_CONST(KEY_LANG5);
    LEO_SET_KEY_CONST(KEY_LANG6);
    LEO_SET_KEY_CONST(KEY_LANG7);
    LEO_SET_KEY_CONST(KEY_LANG8);
    LEO_SET_KEY_CONST(KEY_LANG9);
    LEO_SET_KEY_CONST(KEY_CAPSLOCK);
    LEO_SET_KEY_CONST(KEY_NUMLOCKCLEAR);
    LEO_SET_KEY_CONST(KEY_SYSREQ);
    LEO_SET_KEY_CONST(KEY_APPLICATION);
    LEO_SET_KEY_CONST(KEY_POWER);
    LEO_SET_KEY_CONST(KEY_SLEEP);
    LEO_SET_KEY_CONST(KEY_AC_SEARCH);
    LEO_SET_KEY_CONST(KEY_AC_HOME);
    LEO_SET_KEY_CONST(KEY_AC_BACK);
    LEO_SET_KEY_CONST(KEY_AC_FORWARD);
    LEO_SET_KEY_CONST(KEY_AC_STOP);

#undef LEO_SET_KEY_CONST

#define LEO_SET_GAMEPAD_CONST(value, label)    \
    do                                         \
    {                                          \
        lua_pushinteger(L, value);             \
        lua_setglobal(L, "leo_" label);        \
    } while (0)

    // Gamepad limits
    LEO_SET_GAMEPAD_CONST(LEO_MAX_GAMEPADS, "MAX_GAMEPADS");

    // Gamepad button constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_UNKNOWN, "GAMEPAD_BUTTON_UNKNOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_DOWN, "GAMEPAD_BUTTON_FACE_DOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_RIGHT, "GAMEPAD_BUTTON_FACE_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_LEFT, "GAMEPAD_BUTTON_FACE_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_FACE_UP, "GAMEPAD_BUTTON_FACE_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_LEFT_BUMPER, "GAMEPAD_BUTTON_LEFT_BUMPER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_RIGHT_BUMPER, "GAMEPAD_BUTTON_RIGHT_BUMPER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_BACK, "GAMEPAD_BUTTON_BACK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_GUIDE, "GAMEPAD_BUTTON_GUIDE");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_START, "GAMEPAD_BUTTON_START");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_LEFT_STICK, "GAMEPAD_BUTTON_LEFT_STICK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_RIGHT_STICK, "GAMEPAD_BUTTON_RIGHT_STICK");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_UP, "GAMEPAD_BUTTON_DPAD_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_RIGHT, "GAMEPAD_BUTTON_DPAD_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_DOWN, "GAMEPAD_BUTTON_DPAD_DOWN");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_DPAD_LEFT, "GAMEPAD_BUTTON_DPAD_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_BUTTON_COUNT, "GAMEPAD_BUTTON_COUNT");

    // Gamepad axis constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_X, "GAMEPAD_AXIS_LEFT_X");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_Y, "GAMEPAD_AXIS_LEFT_Y");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_X, "GAMEPAD_AXIS_RIGHT_X");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_Y, "GAMEPAD_AXIS_RIGHT_Y");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_LEFT_TRIGGER, "GAMEPAD_AXIS_LEFT_TRIGGER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_RIGHT_TRIGGER, "GAMEPAD_AXIS_RIGHT_TRIGGER");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_AXIS_COUNT, "GAMEPAD_AXIS_COUNT");

    // Gamepad stick and direction constants
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_STICK_LEFT, "GAMEPAD_STICK_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_STICK_RIGHT, "GAMEPAD_STICK_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_LEFT, "GAMEPAD_DIR_LEFT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_RIGHT, "GAMEPAD_DIR_RIGHT");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_UP, "GAMEPAD_DIR_UP");
    LEO_SET_GAMEPAD_CONST(LEO_GAMEPAD_DIR_DOWN, "GAMEPAD_DIR_DOWN");

#undef LEO_SET_GAMEPAD_CONST

    return 0;
}
