// Keeps a plugin's settings between game sessions, in a small INI file inside the game's save folder
// (<save>/save_0/<Name>.ini), the way Forge plugins do it: Forge mounts the save data, nn::fs (the game's own SDK)
// reads and writes the file, and unmounting commits it.
//
// Use: list the settings in a PSetting array, call persist_tick(&p, frame) once per frame from forge_onUpdate.
// It loads the file once the save data can be mounted (never before: saving first would overwrite the file with the
// defaults), and saves a second and a half after the last change, so dragging a slider writes the file once.
#pragma once
#include "forge.h"

typedef long long s64;
typedef struct { void *h; } FsFile;
typedef struct { s32 flags; } FsWriteOption;

extern bool forge_fs_mountSaveData(void);
extern void forge_fs_unmountSaveData(void);
extern const char *forge_fs_getSaveDataMountPoint(void);

extern u32 nn_fs_OpenFile(FsFile *out, const char *path, s32 mode) __asm__("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci");
extern u32 nn_fs_CloseFile(FsFile f) __asm__("_ZN2nn2fs9CloseFileENS0_10FileHandleE");
extern u32 nn_fs_GetFileSize(s64 *out, FsFile f) __asm__("_ZN2nn2fs11GetFileSizeEPxNS0_10FileHandleE");
extern u32 nn_fs_ReadFile(FsFile f, s64 off, void *buf, u32 size) __asm__("_ZN2nn2fs8ReadFileENS0_10FileHandleExPvj");
extern u32 nn_fs_WriteFile(FsFile f, s64 off, const void *buf, u32 size, const FsWriteOption *o)
    __asm__("_ZN2nn2fs9WriteFileENS0_10FileHandleExPKvjRKNS0_11WriteOptionE");
extern u32 nn_fs_CreateFile(const char *path, s64 size) __asm__("_ZN2nn2fs10CreateFileEPKcx");
extern u32 nn_fs_DeleteFile(const char *path) __asm__("_ZN2nn2fs10DeleteFileEPKc");

enum { FS_READ = 1, FS_WRITE = 2 };

// One setting: its name in the file, where it lives (bool = 1 byte, s32 = 4 bytes) and its limits.
typedef struct { const char *key; void *val; u32 size; s32 lo, hi; } PSetting;
#define PS_BOOL(k, v)         { k, &(v), 1, 0, 1 }
#define PS_INT(k, v, lo, hi)  { k, &(v), 4, lo, hi }

#define PERSIST_MAX 32
#define PERSIST_BUF 1024
typedef struct {
    const char *file;           // "LootTweaks.ini"
    const char *header;         // first comment line
    const PSetting *s;
    u32 n;
    bool loaded;                // the file was read (or does not exist): saving is allowed from now on
    bool failed;                // the last save did not work
    u32 tries, changed_at;      // load attempts; frame of the last change
    s32 saved[PERSIST_MAX];     // the values as last loaded or saved
    s32 last[PERSIST_MAX];      // the values on the previous frame
} Persist;

static s32 ps_get(const PSetting *p) { return p->size == 1 ? *(u8 *)p->val : *(s32 *)p->val; }
static void ps_set(const PSetting *p, s32 v) {
    if (v < p->lo) v = p->lo;
    if (v > p->hi) v = p->hi;
    if (p->size == 1) *(u8 *)p->val = (u8)v; else *(s32 *)p->val = v;
}

static u32 ps_len(const char *s) { u32 n = 0; while (s[n]) n++; return n; }
static bool ps_eq(const char *a, const char *b, u32 n) { for (u32 i = 0; i < n; i++) if (a[i] != b[i]) return false; return true; }
static u32 ps_put(char *o, u32 at, const char *s) { while (*s) o[at++] = *s++; return at; }
static u32 ps_putint(char *o, u32 at, s32 v) {
    char t[12]; u32 n = 0;
    u32 u = v < 0 ? (u32)(-v) : (u32)v;
    if (v < 0) o[at++] = '-';
    do { t[n++] = (char)('0' + u % 10); u /= 10; } while (u);
    while (n) o[at++] = t[--n];
    return at;
}

static bool ps_path(char *out, u32 n, const char *file) {
    const char *mp = forge_fs_getSaveDataMountPoint();
    if (!mp || ps_len(mp) + ps_len(file) + 3 > n) return false;
    u32 at = ps_put(out, 0, mp);
    at = ps_put(out, at, ":/");
    at = ps_put(out, at, file);
    out[at] = 0;
    return true;
}

static void persist_snapshot(Persist *p) {
    for (u32 i = 0; i < p->n; i++) p->saved[i] = p->last[i] = ps_get(&p->s[i]);
}

// Reads the file. false: the save data could not be mounted yet (try again later). A missing file is fine.
static bool persist_load(Persist *p) {
    char path[96], buf[PERSIST_BUF + 1];
    if (!ps_path(path, sizeof path, p->file)) return false;
    if (!forge_fs_mountSaveData()) return false;
    u32 len = 0;
    FsFile f = {0};
    if (nn_fs_OpenFile(&f, path, FS_READ) == 0) {
        s64 size = 0;
        if (nn_fs_GetFileSize(&size, f) == 0 && size > 0) {
            len = size > PERSIST_BUF ? PERSIST_BUF : (u32)size;
            if (nn_fs_ReadFile(f, 0, buf, len) != 0) len = 0;
        }
        nn_fs_CloseFile(f);
    }
    forge_fs_unmountSaveData();
    buf[len] = 0;
    u32 hit = 0;
    for (u32 i = 0; i < len;) {
        u32 e = i;
        while (e < len && buf[e] != '\n') e++;
        u32 b = i, k = e;
        while (k > b && buf[k - 1] == '\r') k--;
        i = e + 1;
        if (k <= b || buf[b] == ';' || buf[b] == '[') continue;
        for (u32 j = 0; j < p->n; j++) {
            u32 kl = ps_len(p->s[j].key);
            if (k - b <= kl || buf[b + kl] != '=' || !ps_eq(buf + b, p->s[j].key, kl)) continue;
            u32 q = b + kl + 1;
            bool neg = q < k && buf[q] == '-';
            if (neg) q++;
            if (q >= k || buf[q] < '0' || buf[q] > '9') break;
            s32 v = 0;
            for (; q < k && buf[q] >= '0' && buf[q] <= '9' && v < 100000000; q++) v = v * 10 + (buf[q] - '0');
            ps_set(&p->s[j], neg ? -v : v);
            hit++;
            break;
        }
    }
    forge_log(LOG_INFO, "[%s] settings %s (%u read)", p->file, len ? "loaded" : "file not found, defaults", hit);
    return true;
}

static bool persist_save(Persist *p) {
    char path[96], buf[PERSIST_BUF];
    if (!ps_path(path, sizeof path, p->file)) return false;
    u32 at = ps_put(buf, 0, "; ");
    at = ps_put(buf, at, p->header);
    at = ps_put(buf, at, "\n[settings]\n");
    for (u32 i = 0; i < p->n && at + 48 < PERSIST_BUF; i++) {
        at = ps_put(buf, at, p->s[i].key);
        buf[at++] = '=';
        at = ps_putint(buf, at, ps_get(&p->s[i]));
        buf[at++] = '\n';
    }
    if (!forge_fs_mountSaveData()) return false;
    bool ok = true;
    nn_fs_DeleteFile(path);  // (an error just means there was no file)
    if (nn_fs_CreateFile(path, at) != 0) ok = false;
    FsFile f = {0};
    if (ok && nn_fs_OpenFile(&f, path, FS_WRITE) == 0) {
        FsWriteOption flush = {1};
        if (nn_fs_WriteFile(f, 0, buf, at, &flush) != 0) ok = false;
        nn_fs_CloseFile(f);
    } else
        ok = false;
    forge_fs_unmountSaveData();  // commits
    return ok;
}

// Once per frame.
static void persist_tick(Persist *p, u32 frame) {
    if (!p->loaded) {
        if (frame % 30 != 1 && p->tries) return;
        if (persist_load(p)) p->loaded = true, persist_snapshot(p);
        else if (++p->tries == 20) {
            forge_log(LOG_WARN, "[%s] the save data never mounted: settings will not be kept", p->file);
            p->loaded = true, persist_snapshot(p);  // (stop trying; saving may still work later)
        }
        return;
    }
    bool diff = false, moving = false;
    for (u32 i = 0; i < p->n; i++) {
        s32 v = ps_get(&p->s[i]);
        diff |= v != p->saved[i];
        moving |= v != p->last[i];
        p->last[i] = v;
    }
    if (!diff) return;
    if (moving) p->changed_at = frame;
    if (frame - p->changed_at < 90) return;  // (still being changed: a slider would write the file on every frame)
    p->failed = !persist_save(p);
    if (p->failed) forge_log(LOG_WARN, "[%s] could not save the settings", p->file);
    persist_snapshot(p);
}
