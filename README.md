# Xash3D FWGS Engine (callsign Brother Hermes) <img align="right" width="128" height="128" src="https://github.com/Hidrocarbono/xash3d-fwgs/raw/master/game_launch/icon-xash-material.png" alt="Xash3D FWGS icon" />

[![GitHub Actions Status](https://github.com/Hidrocarbono/xash3d-fwgs/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/Hidrocarbono/xash3d-fwgs/actions/workflows/c-cpp.yml) [![Discord Server](https://img.shields.io/discord/355697768582610945?logo=Discord&label=International%20Discord%20chat)](http://xash.su/discord/) [![Telegram Chat](https://img.shields.io/badge/Telegram_chat-gray?logo=Telegram)](https://t.me/flyingwithgauss)

> [!CAUTION]
> **Baixe o Xash3D FWGS apenas de fontes oficiais.** Builds de terceiros, "launchers modificados", "repacks otimizados" e mirrors aleatórios frequentemente vêm com malware, mineradores, spyware e roubadores de credenciais. Não podemos garantir nada que não tenhamos construído. Obtenha binários oficiais apenas na [página de releases](https://github.com/Hidrocarbono/xash3d-fwgs/releases/tag/continuous).

Este é um fork do [Xash3D FWGS](https://github.com/FWGS/xash3d-fwgs) com modificações personalizadas, incluindo:

- **Sistema de armas via script** (inspirado no sistema do Uncle Mike para Paranoia 2) — adicione novas armas apenas colocando modelos na pasta `models/` e criando arquivos de script em `scripts/weapons/`, sem recompilar o código-fonte.
- **Sistema de legendas personalizadas** — exiba textos formatados na tela com fontes `.ttf`, cores personalizadas e substituição automática de `%player_name%` pelo nome do jogador.
- **Compatibilidade total com o Xash3D FWGS** e com o ecossistema Half-Life/GoldSrc.

## Donate
[![Donate to FWGS button](https://img.shields.io/badge/Donate_to_FWGS-%3C3-magenta)](Documentation/donate.md) \
If you like Xash3D FWGS, consider supporting individual engine maintainers. By supporting us, you help to continue developing this game engine further. The sponsorship links are available in [documentation](Documentation/donate.md).

## Fork features (além das features originais)
* Sistema de armas via script — carregamento dinâmico de armas a partir de arquivos `.txt` em `scripts/weapons/`.
* Sistema de legendas na tela — mensagens formatadas com título, texto, cores e fontes personalizadas, acionadas por entidades no mapa.
* Suporte a substituição de `%player_name%` pelo nome do jogador em textos.
* Todas as features do Xash3D FWGS original (Steam HLSDK 2.5, crossplatform, multiplayer avançado, múltiplos renderers, etc.).

