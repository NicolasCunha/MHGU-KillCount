# Kill Count

Plugin [Forge](https://github.com/Fexty12573/forge) para o [MHGU Recomp](https://github.com/morerokk/MHGURecomp)
(Monster Hunter Generations Ultimate 1.4.0) que define **quantas mortes cada monstro vale** nas quests de caçar uma
quantidade de monstros. Pequenos e grandes têm configurações separadas, ajustadas no jogo pelo menu do Forge.

[Read in English](README.md)

Com "pequenos x3", matar um Rhenoplos avança 3 numa quest "caçar 5". A quest faz tudo o que faz por uma morte essa
quantidade de vezes, então contagem restante, pontos e conclusão da quest acompanham. Nenhum arquivo do jogo é alterado.

## Configurações

Aperte **Insert** no jogo e abra **Kill Count**:

| Opção | Padrão | O que faz |
|---|---|---|
| Enabled | ligado | Liga ou desliga o plugin |
| Small monster counts as | 1 morte | Mortes registradas por monstro pequeno (qualquer um), de 1 a 20 |
| Large monster counts as | 1 morte | Mortes registradas por monstro grande, de 1 a 20 |
| Log details to log.txt | desligado | Registra cada morte no log |

As configurações são salvas automaticamente em `KillCount.ini` na pasta de save (`save_0`) e carregadas ao abrir o jogo.

## Instalação

1. Baixe `KillCount-<versão>.zip` em Releases, ou compile (abaixo).
2. Extraia na pasta `mods` ao lado do `mhgurecomp.exe`, ficando `mods/KillCount/romfs/nativeNX/plugins/killcount.nro`.
3. Abra o jogo. O `log.txt` deve ter `[KillCount] 1.0.0 ready: kill hook ok`.

Para desinstalar, apague `mods/KillCount`. Testado em single player no Windows.

## Como funciona

O objeto da quest registra cada monstro morto na função em main `0x3A6108`: incrementa um contador (`+0x99` pequenos,
`+0x98` grandes, distinguidos pelo bit `0x1000` do id do monstro) e atualiza os objetivos. Depois da chamada do jogo,
o plugin a chama de novo com os mesmos argumentos N-1 vezes.

## Compilar

Requer shell POSIX (Git Bash no Windows), Arm GNU Toolchain (`arm-none-eabi-gcc`) e Python 3.

```sh
sh build.sh                                # out/killcount.nro
sh build.sh release                        # dist/KillCount-<versão>.zip
RECOMP=/d/MHGURecomp sh build.sh install   # compila e copia para <recomp>/mods/KillCount
```

## Licença

MIT, veja [LICENSE](LICENSE).
