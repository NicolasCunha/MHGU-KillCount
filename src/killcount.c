// Kill Count: sets how many kills each monster counts for in quests ("hunt N monsters"). Everything is set in Forge's menu,
// with one value for small monsters and another for large ones.
//
// The quest object (sQuest) records every monster that dies in QUEST_KILL, whose first argument is the quest itself (this):
//   this+0x99  u8  how many small monsters died (the monster's id has bit 0x1000: Rhenoplos 0x1019, Ioprey 0x100B)
//   this+0x98  u8  how many large monsters died
// and also counts it for the quest's goals (what is left to hunt, the points, whether the quest is done).
// After the game has done that for a kill, this plugin calls QUEST_KILL again with the same arguments, N - 1 more times,
// so that the quest does everything it does for a kill N times, goals and quest clear included.
#include "forge.h"
#include "persist.h"

#define QUEST_KILL   0x3A6108  // (quest, event, flag, ...) a monster died
#define QUEST_KILL2  0x3A7F40  // (quest, ...) also bumps the same two counters (only logged)
#define MAX_MULT     20

typedef u32 (*F8)(u32, u32, u32, u32, u32, u32, u32, u32);

// ---------------------------------------------------------------- settings (Forge menu)

static bool g_on = true;
static s32 g_small = 1;   // one small monster counts as this many kills
static s32 g_large = 1;   // one large monster counts as this many kills
static bool g_log = false;

static const PSetting g_settings[] = {
    PS_BOOL("enabled", g_on),
    PS_INT("small_kills", g_small, 1, MAX_MULT),
    PS_INT("large_kills", g_large, 1, MAX_MULT),
    PS_BOOL("log_details", g_log),
};
static Persist g_persist = {
    .file = "KillCount.ini",
    .header = "Kill Count settings. Change them in game: Insert -> Kill Count. Delete this file to go back to the defaults.",
    .s = g_settings,
    .n = sizeof g_settings / sizeof g_settings[0],
};
static u32 g_main, g_frame, g_extra_small, g_extra_large;
static ForgeHook h_kill, h_kill2;
static bool g_busy;

static bool ok(u32 p) { return p - 0x10000u <= 0xEFFEFFFFu; }

// ---------------------------------------------------------------- kills

static u32 kill_hook(u32 q, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h) {
    if (!ok(q) || g_busy) return ((F8)h_kill.original)(q, b, c, d, e, f, g, h);
    u8 s0 = *(u8 *)(q + 0x99), l0 = *(u8 *)(q + 0x98);
    u32 r = ((F8)h_kill.original)(q, b, c, d, e, f, g, h);
    u32 ds = (u8)(*(u8 *)(q + 0x99) - s0), dl = (u8)(*(u8 *)(q + 0x98) - l0);
    if (g_log && (ds || dl))
        forge_log(LOG_INFO, "[KillCount] kill registered: %u small, %u large (totals %u / %u)", ds, dl, *(u8 *)(q + 0x99), *(u8 *)(q + 0x98));
    if (!g_on || (!ds && !dl) || ds > 4 || dl > 4) return r;  // (a bigger jump is not a kill)
    u32 again_small = ds ? ds * (u32)(g_small - 1) : 0, again_large = dl ? dl * (u32)(g_large - 1) : 0;
    g_busy = true;  // (the calls below are not kills of their own to multiply)
    for (u32 i = 0; i < again_small + again_large; i++) ((F8)h_kill.original)(q, b, c, d, e, f, g, h);
    g_busy = false;
    g_extra_small += again_small, g_extra_large += again_large;
    if (g_log) forge_log(LOG_INFO, "[KillCount] counted %u more small and %u more large (totals %u / %u)", again_small, again_large, *(u8 *)(q + 0x99), *(u8 *)(q + 0x98));
    return r;
}

// Only to learn whether this function is the one that registers kills in a quest: logs when it moves the counters.
static u32 kill2_hook(u32 q, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h) {
    if (!g_log || !ok(q)) return ((F8)h_kill2.original)(q, b, c, d, e, f, g, h);
    u8 s0 = *(u8 *)(q + 0x99), l0 = *(u8 *)(q + 0x98);
    u32 r = ((F8)h_kill2.original)(q, b, c, d, e, f, g, h);
    if (*(u8 *)(q + 0x99) != s0 || *(u8 *)(q + 0x98) != l0)
        forge_log(LOG_INFO, "[KillCount] (the second function moved the counters: %u/%u -> %u/%u)", s0, l0, *(u8 *)(q + 0x99), *(u8 *)(q + 0x98));
    return r;
}

// ---------------------------------------------------------------- Forge entry points

EXPORT void forge_onLoad(ForgePluginInfo *info) {
    static const char name[] = "Kill Count";
    if (!info) return;
    info->major = 1, info->minor = 5, info->patch = 1;
    for (u32 i = 0; i < sizeof info->name; i++) info->name[i] = i < sizeof name ? name[i] : 0;
}

EXPORT void forge_onInit(void) {
    g_main = forge_mem_getMainTextAddr();
    u32 orig = 0;
    forge_hook_create(&h_kill, g_main + QUEST_KILL, (u32)kill_hook, &orig);
    forge_hook_create(&h_kill2, g_main + QUEST_KILL2, (u32)kill2_hook, &orig);
    forge_log(LOG_INFO, "[KillCount] %s ready: kill hook %s", PLUGIN_VERSION, h_kill.original ? "ok" : "FAILED");
}

EXPORT void forge_onUpdate(float dt) {
    (void)dt;
    persist_tick(&g_persist, ++g_frame);
}

EXPORT void forge_onImGuiInit(void *ctx, void *alloc, void *freefn) {
    if (!ctx || !alloc || !freefn) return;
    igSetCurrentContext(ctx);
    igSetAllocatorFunctions(alloc, freefn, NULL);
}

EXPORT void forge_onImGuiRender(void) {
    if (!igTreeNode_Str("Kill Count###killcount")) return;
    igCheckbox("Enabled", &g_on);
    igSliderInt("Small monster counts as", &g_small, 1, MAX_MULT, "%d kills", 0);
    igSliderInt("Large monster counts as", &g_large, 1, MAX_MULT, "%d kills", 0);
    igTextUnformatted("(for quests that ask you to hunt a number of monsters)", NULL);
    igCheckbox("Log details to log.txt", &g_log);
    igTextUnformatted("(settings are saved automatically)", NULL);
    igTreePop();
}
