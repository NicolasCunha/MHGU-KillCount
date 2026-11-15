# Kill Count

A [Forge](https://github.com/Fexty12573/forge) plugin for [MHGU Recomp](https://github.com/morerokk/MHGURecomp)
(Monster Hunter Generations Ultimate 1.4.0) that sets **how many kills each monster is worth** in quests that ask you
to hunt a number of monsters. Small and large monsters have separate settings, changed in game from Forge's menu.

[Leia em português](README.pt-BR.md)

With "small monsters x3", killing one Rhenoplos moves a "hunt 5" quest by 3. The quest does everything it does for a
kill that many times, so remaining counts, points and quest clear all follow. No game file is changed.

## Settings

Press **Insert** in game and open **Kill Count**:

| Setting | Default | What it does |
|---|---|---|
| Enabled | on | Turns the plugin on or off |
| Small monster counts as | 1 kill | Kills registered for each small monster (any small monster), 1 to 20 |
| Large monster counts as | 1 kill | Kills registered for each large monster, 1 to 20 |
| Log details to log.txt | off | Logs each kill registered |

Settings are saved automatically to `KillCount.ini` in your save folder (`save_0`) and loaded when the game starts.

## Install

1. Download `KillCount-<version>.zip` from Releases, or build it (below).
2. Extract it into the `mods` folder next to `mhgurecomp.exe`, so you get `mods/KillCount/romfs/nativeNX/plugins/killcount.nro`.
3. Start the game. `log.txt` should have `[KillCount] 1.0.0 ready: kill hook ok`.

To uninstall, delete `mods/KillCount`. Tested in single player on Windows.

## How it works

The quest object registers every dead monster in the function at main `0x3A6108`: it bumps a counter (`+0x99` small,
`+0x98` large, told apart by bit `0x1000` of the monster id) and updates the quest goals. After the game's own call, the
plugin calls it again with the same arguments N-1 more times.

## Build

Needs a POSIX shell (Git Bash on Windows), the Arm GNU Toolchain (`arm-none-eabi-gcc`) and Python 3.

```sh
sh build.sh                                # out/killcount.nro
sh build.sh release                        # dist/KillCount-<version>.zip
RECOMP=/d/MHGURecomp sh build.sh install   # build and copy into <recomp>/mods/KillCount
```

## Credits

`src/start.S`, `build/nro.ld` and `build/elf2nro.py` come from lick-mee's
[mhgu-gathering-map](https://github.com/lick-mee/mhgu-gathering-map); `src/forge.h` and the build setup from Vuze's
[mhgu-field-qol](https://github.com/mhvuze/mhgu-field-qol). Both MIT, see [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES).

## License

MIT, see [LICENSE](LICENSE).
