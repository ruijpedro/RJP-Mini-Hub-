# RJP Mini Hub v0.2

Firmware de diagnóstico para **ESP32-2432S028** (ecrã tátil 2,8", 320×240, duas portas USB).

## Estado

Primeira versão de testes: ecrã, toque, Wi-Fi, relógio de Portugal e número da semana. Os menus Comboios/Agenda são demonstrativos. **Não gravado nem testado fisicamente**.

O código completo do projeto será adicionado a este repositório juntamente com a compilação GitHub Actions.

## Compilação

Abrir **Actions → Compilar firmware ESP32**. No sucesso, descarregar o artefacto `RJP-MiniHub-cyd_ili9341` e utilizar `RJP-MiniHub-cyd_ili9341-full.bin` para a primeira gravação em **0x0** através do [esptool-js](https://espressif.github.io/esptool-js/).

Se o ecrã não funcionar, experimentar `cyd_st7789`. Ambas as variantes são experimentais até teste na placa.

⚠️ A gravação substitui o firmware original. A ligação ao ESP32 precisa de cabo USB de dados.
