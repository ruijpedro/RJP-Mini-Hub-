# RJP Mini Hub v0.2 — ESP32-2432S028

Projeto independente para a placa amarela ESP32 com **ecrã tátil de 2,8"**, 320×240, duas portas USB e módulo ESP-32S.

**Estado: código de diagnóstico preparado.** Ainda não testado numa placa física. Comboios e Agenda são páginas demonstrativas (sem ligações às APIs nesta versão).

## Funções incluídas
- Ecrã horizontal com botões: **Comboios, Agenda, Sensores e Definições**
- Barras vermelha/verde/azul no arranque para diagnosticar o TFT
- Calibração tátil XPT2046 com três cruzes, guardada em memória persistente
- Navegação pelo botão **BOOT** mesmo quando o toque não funcionar
- Configuração Wi-Fi pelo telemóvel: hotspot **RJP-MiniHub-Setup**, palavra-passe inicial **rjpmini2026**
- Abrir **http://192.168.4.1** no telemóvel ligado ao hotspot e preencher o nome e palavra-passe da rede Wi-Fi de casa
- Hora de Portugal continental (com mudança de hora) e número da semana ISO
- Leitura analógica do sensor de luminosidade da placa

**Não colocar dados de acesso à rede nem tokens nos ficheiros do GitHub.** Os dados são guardados localmente no ESP32 através de Preferences.

## Compilar no GitHub
1. Abrir [Actions deste repositório](https://github.com/ruijpedro/RJP-Mini-Hub-/actions).
2. Selecionar **Compilar firmware ESP32** e verificar a execução iniciada automaticamente após um push, ou usar **Run workflow**.
3. Se ambos os trabalhos estiverem verdes, descarregar o artefacto **RJP-MiniHub-cyd_ili9341** em **Artifacts**. Há também o **cyd_st7789** para outro controlador.
4. Dentro do ZIP do artefacto, usar o ficheiro **RJP-MiniHub-cyd_ili9341-full.bin** (não o app.bin) para a gravação inicial.

## Gravar no ESP32 pelo Chrome/Edge
1. Usar apenas a **placa amarela**, ligada por cabo USB **de dados**.
2. Abrir [Espressif esptool-js](https://espressif.github.io/esptool-js/) no Chrome/Edge e selecionar a porta série.
3. Selecionar o ficheiro **-full.bin** em **Flash Address 0x0**.
4. Programar e reiniciar a placa (RESET ou desligar/ligar a alimentação).
5. Se o ecrã ficar branco/invertido, tentar a variante **cyd_st7789**, também em 0x0.

⚠️ Gravar firmware novo substitui o programa original; se o quiseres conservar, faz primeiro uma cópia de segurança do conteúdo da flash. Se o computador impedir acesso USB/portas série, fazer a primeira gravação num computador pessoal.

## Primeiro teste de aceitação
- São visíveis as três barras RGB e a mensagem RJP MINI HUB.
- Aparece **CALIBRACAO DO TOQUE**. Tocar no centro de cada uma das três cruzes.
- Se a calibração falhar, premir o botão **BOOT** da placa para navegar pelos ecrãs de diagnóstico.
- Na ausência de Wi-Fi configurado, ligar o telemóvel a **RJP-MiniHub-Setup** e abrir **http://192.168.4.1**.
- Após conectar à Internet, confirmar relógio e número da semana.
- Em **Definições**, verificar o IP da placa; é possível refazer a calibração e reconfigurar o Wi-Fi.

## Pinos desta variante CYD
| Função | GPIO |
|---|---|
| TFT SCLK / MOSI / MISO | 14 / 13 / 12 |
| TFT CS / DC / Backlight | 15 / 2 / 21 |
| Touch SCLK / MOSI / MISO / CS | 25 / 32 / 39 / 33 |
| Sensor de luz | 34 |
| BOOT | 0 |

## Próxima fase
Após validação do hardware: consumir dados da **RJP Train Live** e da **Agenda RJP**, depois preparar atualização OTA. As integrações dependem de endpoints e credenciais próprias e não devem ser consideradas funcionais nesta v0.2.
