# RJP Mini Hub v0.3 — ESP32-2432S028

Firmware para ESP32 com ecrã 2,8" 320×240, duas portas USB. **Versão experimental de desenvolvimento:** testar o ecrã e os periféricos numa placa física antes de distribuir.

## Funcionalidades

- Mantém as credenciais Wi-Fi e a calibração XPT2046 da v0.2 (Preferences `rjpmini`), desde que não se apague a NVS.
- Relógio Europe/Lisbon e número da semana ISO.
- **Comboios:** lê o endpoint RJP Train Live `https://rjp-train-live-api.rjpedro.workers.dev/api/trains/active` quando se abre o menu e atualiza de 90 em 90 segundos enquanto esse menu estiver aberto. Mostra até 4 comboios com origem/destino e atraso reportado. A disponibilidade depende do Worker e da fonte Comboios Live de Rui Costa; informação não destinada à segurança da exploração ferroviária. **Provisoriamente, nesta versão de teste, não é validado o certificado TLS do feed: não são transmitidos dados sensíveis.**
- **Agenda:** mostra o número da semana e até três compromissos configurados no próprio ESP32 em `http://IP-DA-PLACA/agenda`. Ainda **não sincroniza** com a Agenda RJP, Google nem Outlook.
- **Atualização OTA local:** requer uma única gravação USB para instalar a v0.3. Nas versões posteriores é possível atualizar pelo navegador de um dispositivo na mesma rede, sem novo cabo USB.
- **Diagnóstico:** sensor de luz e possibilidade de recalibrar o ecrã e reconfigurar a rede.

## Instalação inicial da v0.3 (a partir da v0.2)

1. Abrir [GitHub Actions](https://github.com/ruijpedro/RJP-Mini-Hub-/actions) e entrar na compilação da v0.3 após aprovação.
2. Descarregar o artefacto `RJP-MiniHub-cyd_st7789` (se foi este o controlador que funcionou anteriormente) ou `RJP-MiniHub-cyd_ili9341`.
3. **Para atualizar a v0.2 sem perder o Wi-Fi e a calibração**, extrair o ZIP e, no [esptool-js](https://espressif.github.io/esptool-js/), gravar **`-app.bin` no endereço `0x10000`**, com **Erase all desmarcado**. A tabela de partições mantém-se inalterada. **Não utilizar `-full.bin` para esta atualização:** por preencher também os setores intermédios, essa imagem pode apagar a NVS, mesmo com Erase all desmarcado.
4. Reiniciar e verificar Wi-Fi, calibração, Comboios e Agenda.

## OTA nas versões posteriores

1. Descarregar o novo artefacto do GitHub Actions.
2. Na placa, abrir **Definições → OTA LOCAL**. O ecrã mostra o IP da placa e um código temporário de oito dígitos válido por cinco minutos.
3. Num telemóvel ou computador **na mesma rede Wi-Fi**, abrir `http://IP-DA-PLACA/update`.
4. Na autenticação do navegador: utilizador **rjp**, palavra-passe **o código mostrado no ecrã**.
5. Selecionar **apenas o `-app.bin`** da nova versão. Não usar o `-full.bin` no OTA.
6. A placa valida o firmware recebido antes de reiniciar. Se ocorrer erro, conserva o firmware anterior.

**Segurança:** a interface OTA é servida por HTTP na LAN, sem cifragem do tráfego. Só fica disponível durante a janela aberta no ecrã. Não encaminhar a porta 80 nem expor a placa à Internet. Para produção, planeia-se assinatura do firmware e entrega automática por TLS validado.

## Ambiente
- PlatformIO, Arduino ESP32, TFT_eSPI, XPT2046_Touchscreen e ArduinoJson
- GitHub Actions: duas variantes de ecrã, binários `-full.bin` (USB/0x0) e `-app.bin` (OTA local)
- [Código do firmware](src/main.cpp), [cliente RJP Train Live](src/train_feed.h)

## Futuro
Sincronização autenticada dos eventos da Agenda RJP e atualização OTA automática com canal de releases. Manter o projeto RJP Train Live original sem alterações.

## Recuperação total / primeira gravação
Quando for necessário gravar do zero ou recuperar a placa, usar `-full.bin` no endereço `0x0`. Esta operação **pode apagar a NVS**, obrigando a configurar novamente Wi-Fi e calibração do toque.
