# Alterações

## v0.3 (em testes)
- Fonte RJP Train Live consultada no ecrã Comboios (até 4 serviços, atualização 90 s)
- Agenda local de três compromissos, configurável no browser por `/agenda`; ainda sem Google/Outlook
- Atualização OTA local com autenticação temporária aberta pelo toque no ecrã
- Mantém Wi-Fi e calibração guardados na NVS
- Proteção operacional: não modificar a RJP Train Live original
- Limitações: feed HTTPS sem validação CA no protótipo e OTA por HTTP apenas na LAN; ambos a melhorar antes de produção

# Alterações

## v0.2 — firmware inicial de diagnóstico
- Teste de barras RGB, ecrã e menu
- Toque XPT2046 com calibração afim de três pontos guardada em flash
- Navegação por BOOT quando o toque não funciona
- Configuração Wi-Fi local por ponto de acesso e relógio NTP
- Compilação de duas variantes de controlador TFT
- Artefactos .bin completos e hashes SHA256

## Limitações
- Comboios e Agenda ainda são menus demonstrativos
- OTA ainda não implementado; primeiro testar a gravação USB
