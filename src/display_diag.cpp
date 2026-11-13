#ifdef DISPLAY_DIAGNOSTIC
// RJP Mini Hub - Teste isolado do ecrã CYD, sem Wi-Fi nem aplicações
// Este ficheiro so compila nos ambientes diag_*. O firmware normal fica intacto.
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI lcd;
const uint16_t COLORS[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK, TFT_WHITE};
const char* NAMES[] = {"VERMELHO", "VERDE", "AZUL", "PRETO", "BRANCO"};
unsigned long changedAt=0;
int phase=0;
bool invertMode=false;

void showPhase() {
  // O LED de fundo deve ficar ligado durante todo o teste.
  digitalWrite(21, HIGH);
  const uint16_t color=COLORS[phase];
  lcd.invertDisplay(invertMode);
  lcd.fillScreen(color);
  if(phase != 4){
    lcd.setTextColor(TFT_WHITE,color);
  }else{
    lcd.setTextColor(TFT_BLACK,color);
  }
  lcd.drawString("RJP LCD DIAGNOSTICO", 10, 14, 2);
  lcd.drawString(NAMES[phase], 10, 43, 4);
  lcd.drawString(invertMode?"INVERSAO: ON":"INVERSAO: OFF", 10, 82, 2);
  lcd.drawString("Ciclo automatico 3s", 10, 114, 2);
  lcd.drawString("Sem acesso Wi-Fi", 10, 142, 2);
  Serial.printf("[RJP-DIAG] Teste: %s | inversao=%s | %lu ms\n", NAMES[phase],invertMode?"ON":"OFF",millis());
}

void setup(){
  Serial.begin(115200);
  delay(300);
  Serial.println("\n==============================");
  Serial.println("[RJP-DIAG] ESP32-2432S028 LCD v0.3.1");
#if defined(ST7789_DRIVER)
  Serial.println("[RJP-DIAG] DRIVER: ST7789");
#elif defined(ILI9341_2_DRIVER)
  Serial.println("[RJP-DIAG] DRIVER: ILI9341_2");
#else
  Serial.println("[RJP-DIAG] DRIVER: INDEFINIDO");
#endif
  Serial.printf("[RJP-DIAG] heap livre: %u bytes\n", ESP.getFreeHeap());
  pinMode(21,OUTPUT);
  digitalWrite(21,HIGH);
  Serial.println("[RJP-DIAG] A iniciar TFT_eSPI...");
  lcd.init();
  Serial.println("[RJP-DIAG] TFT_eSPI init() devolveu");
  lcd.setRotation(1);
  Serial.printf("[RJP-DIAG] LCD dimensoes %d x %d\n",lcd.width(),lcd.height());
  showPhase();
  changedAt=millis();
}

void loop(){
  if(millis()-changedAt>3000){
    phase++;
    if(phase>=5){phase=0;invertMode=!invertMode;}
    showPhase();
    changedAt=millis();
  }
  delay(20);
}

#endif // DISPLAY_DIAGNOSTIC
