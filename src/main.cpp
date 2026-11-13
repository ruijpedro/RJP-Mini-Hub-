// RJP Mini Hub v0.2 - ESP32-2432S028 / CYD 2.8" (duas USB)
// Primeira versao: testes de ecrã, toque, Wi-Fi e hora (sem APIs de terceiros).
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>
#include <Update.h>
#include "train_feed.h"

constexpr int SCREEN_W=320, SCREEN_H=240;
constexpr int TFT_BACKLIGHT=21, BOOT_BUTTON=0, TOUCH_CS=33;
constexpr int TOUCH_CLK=25, TOUCH_MISO=39, TOUCH_MOSI=32;
constexpr const char *TZ_PT="WET0WEST,M3.5.0/1,M10.5.0/2";
constexpr uint16_t BG=0x0842, PANEL=0x1926, WHITE=0xFFFF, MUTED=0xBDF7;
constexpr uint16_t CYAN=0x06FF, GOLD=0xFEA0;

TFT_eSPI tft;
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS);  // polling, sem IRQ
Preferences pref;
WebServer server(80);

bool apMode=false, calibrated=false, touching=false, redraw=true;
bool bootWasDown=false, skippedCalibration=false;
int page=0, calStep=0;
int rawX[3]={}, rawY[3]={};
const int targetX[3]={25,295,25}, targetY[3]={27,27,210};
float offX=0, mulXX=1, mulXY=0, offY=0, mulYX=0, mulYY=1;
uint32_t lastClock=0, lastButtonEvent=0;
uint32_t otaExpires=0, otaRebootAt=0;
String otaPin;
bool otaUploading=false, otaSucceeded=false;

void textAt(const char *s,int x,int y,int font=2,uint16_t color=WHITE){
  tft.setTextColor(color,BG); tft.drawString(s,x,y,font);
}
void boxButton(int x,int y,int w,int h,const char *title,const char *sub,uint16_t accent){
  tft.fillRoundRect(x,y,w,h,10,PANEL);
  tft.fillRect(x,y,5,h,accent);
  textAt(title,x+13,y+14,2,WHITE);
  if(sub && *sub && h>54) textAt(sub,x+13,y+39,2,MUTED);
}
void header(const char *title){
  tft.fillScreen(BG); tft.fillRect(0,0,SCREEN_W,38,0x10C6);
  tft.setTextColor(CYAN,0x10C6); tft.drawString(title,12,10,2);
  tft.drawFastHLine(0,38,SCREEN_W,0x29EB);
}
String clockText(){
  struct tm lt;
  if(getLocalTime(&lt,5)){char b[12]; strftime(b,sizeof(b),"%H:%M",&lt);return String(b);}
  return "--:--";
}
String weekText(){
  struct tm lt;
  if(getLocalTime(&lt,5)){char b[12]; strftime(b,sizeof(b),"%V",&lt);return String(b);}
  return "--";
}
void drawHome(){
  header("RJP MINI HUB  v0.3");
  String status=apMode?"CONFIG. WIFI":WiFi.status()==WL_CONNECTED?"WIFI OK":"SEM WIFI";
  textAt(clockText().c_str(),12,45,4);
  textAt(("Semana "+weekText()).c_str(),129,54,2,MUTED);
  textAt(status.c_str(),234,54,2,CYAN);
  boxButton(12,84,144,61,"COMBOIOS","RJP Train Live",CYAN);
  boxButton(164,84,144,61,"AGENDA","Semana / local",0x8E1F);
  boxButton(12,153,144,61,"SENSORES","Em breve",GOLD);
  boxButton(164,153,144,61,"DEFINICOES","Wi-Fi / info",0xC618);
  tft.setTextColor(MUTED,BG);
  tft.drawCentreString(skippedCalibration?"Toque nao calibrado - prima BOOT":"Dados ferroviarios: RJP / Rui Costa",160,224,1);
}
void drawPage(){
  if(page==0){drawHome();return;}
  const char *names[]={"","RJP TRAIN LIVE","AGENDA SEMANAL","SENSORES","DEFINICOES","ATUALIZACAO OTA"};
  header(names[page]);
  boxButton(12,51,83,43,"< VOLTAR","",CYAN);
  if(page==1){
    String time=trainLastSuccess?("Consultado ha "+String((millis()-trainLastSuccess)/1000)+"s"):"Ainda sem consulta";
    textAt(time.c_str(),110,60,2,MUTED);
    if(trainCount==0){
      textAt("Dados nao disponiveis",20,113,2,GOLD);
      textAt(trainError.c_str(),20,143,2,MUTED);
      textAt("Atualizacao cada 90s",20,173,2,MUTED);
    }else{
      for(int i=0;i<trainCount && i<4;i++){
        int y=101+i*32;
        TrainInfo &t=trainRows[i];
        String line=t.service+" "+t.number;
        if(t.delayKnown && t.delayMin>0)line+="  +"+String(t.delayMin)+" min";
        if(line.length()>32)line=line.substring(0,32);
        textAt(line.c_str(),12,y,2,t.delayKnown&&t.delayMin>0?GOLD:CYAN);
        String route=t.from+" > "+t.to;
        if(route.length()>40)route=route.substring(0,39);
        textAt(route.c_str(),12,y+15,1,MUTED);
      }
    }
    textAt("Fonte: RJP / Comboios Live (Rui Costa)",10,226,1,MUTED);
  }else if(page==2){
    String week="Semana "+weekText()+" | Agenda local";
    textAt(week.c_str(),110,63,2,CYAN);
    bool any=false;
    for(int i=0;i<3;i++){
      String title=pref.getString(("ev"+String(i)+"t").c_str(),"");
      String when=pref.getString(("ev"+String(i)+"d").c_str(),"");
      if(title.isEmpty())continue;
      any=true;
      textAt(when.c_str(),16,110+i*32,2,GOLD);
      if(title.length()>35)title=title.substring(0,35);
      textAt(title.c_str(),16,127+i*32,2,WHITE);
    }
    if(!any){
      textAt("Sem compromissos locais",16,120,2,MUTED);
      textAt("Configurar por navegador:",16,151,2,MUTED);
      String url=WiFi.localIP().toString()+"/agenda";
      textAt(url.c_str(),16,179,2,CYAN);
    }
    textAt("Google/Outlook: integracao futura",12,226,1,MUTED);
  }else if(page==3){
    textAt("Sensor de luz integrado",20,114);
    char b[50]; snprintf(b,sizeof(b),"Leitura bruta: %d",analogRead(34));
    textAt(b,20,144,2,GOLD);
  }else if(page==4){
    textAt(apMode?"Ponto de acesso ativo:":"Rede ligada:",20,108,2,MUTED);
    String nw=apMode?"RJP-MiniHub-Setup":WiFi.SSID();
    tft.drawString(nw.substring(0,27),20,132,2);
    textAt(apMode?"Abrir 192.168.4.1":WiFi.localIP().toString().c_str(),20,154,2,CYAN);
    boxButton(205,49,103,44,"OTA LOCAL","",CYAN);
    boxButton(12,177,145,48,"RECONFIG. WIFI","",GOLD);
    boxButton(164,177,144,48,"CALIBRAR TOQUE","",CYAN);
  }else if(page==5){
    if(!otaPin.isEmpty() && (int32_t)(otaExpires-millis())>0){
      textAt("No computador/telemovel:",12,103,2,MUTED);
      String url="http://"+WiFi.localIP().toString()+"/update";
      textAt(url.c_str(),12,125,2,CYAN);
      textAt("Utilizador: rjp   Codigo:",12,152,2,MUTED);
      textAt(otaPin.c_str(),12,174,4,GOLD);
      textAt("Valido por 5 minutos (rede local)",12,216,1,MUTED);
    }else{
      textAt("Abra Definicoes > OTA LOCAL",12,122,2,GOLD);
      textAt("para ativar o carregamento.",12,151,2,MUTED);
    }
  }
}
void drawCal(){
  tft.fillScreen(BG);
  textAt("CALIBRACAO DO TOQUE",25,10,2,CYAN);
  textAt("Toque no centro da cruz",25,35,2,MUTED);
  textAt("BOTAO BOOT: ignorar teste",25,222,1,GOLD);
  String stage=String(calStep+1)+" / 3";
  textAt(stage.c_str(),259,11,2,GOLD);
  int x=targetX[calStep], y=targetY[calStep];
  tft.drawFastHLine(x-12,y,25,WHITE);
  tft.drawFastVLine(x,y-12,25,WHITE);
  tft.drawCircle(x,y,5,GOLD);
}
void processCalibration(TS_Point p){
  rawX[calStep]=p.x; rawY[calStep]=p.y;
  Serial.printf("Cal %d: x=%d y=%d\n",calStep,p.x,p.y);
  calStep++;
  if(calStep<3){drawCal();return;}
  float dx1=rawX[1]-rawX[0],dy1=rawY[1]-rawY[0];
  float dx2=rawX[2]-rawX[0],dy2=rawY[2]-rawY[0];
  float det=dx1*dy2-dx2*dy1;
  if(fabsf(det)<2000){calStep=0;drawCal();return;}
  float ix1=targetX[1]-targetX[0],iy1=targetY[1]-targetY[0];
  float ix2=targetX[2]-targetX[0],iy2=targetY[2]-targetY[0];
  mulXX=(ix1*dy2-ix2*dy1)/det;
  mulXY=(dx1*ix2-dx2*ix1)/det;
  mulYX=(iy1*dy2-iy2*dy1)/det;
  mulYY=(dx1*iy2-dx2*iy1)/det;
  offX=targetX[0]-mulXX*rawX[0]-mulXY*rawY[0];
  offY=targetY[0]-mulYX*rawX[0]-mulYY*rawY[0];
  pref.putFloat("ox",offX);pref.putFloat("xx",mulXX);pref.putFloat("xy",mulXY);
  pref.putFloat("oy",offY);pref.putFloat("yx",mulYX);pref.putFloat("yy",mulYY);
  pref.putBool("cal",true);
  calibrated=true;skippedCalibration=false;page=0;redraw=true;
}
void onTap(int x,int y){
  Serial.printf("Touch x=%d y=%d page=%d\n",x,y,page);
  if(page==0){
    if(y>=84 && y<145){page=x<160?1:2;redraw=true;}
    else if(y>=153 && y<215){page=x<160?3:4;redraw=true;}
  }else if(page==4 && x>=205 && y>=49 && y<95){
    if(WiFi.status()==WL_CONNECTED){
      otaPin=String(10000000UL+(esp_random()%90000000UL));
      otaExpires=millis()+300000UL;
      page=5;redraw=true;
    }
  }else if(x>=12 && x<95 && y>=51 && y<96){page=0;redraw=true;}
  else if(page==4 && y>=177 && y<230){
    if(x<160){
      pref.remove("ssid"); pref.remove("pass");
      tft.fillScreen(BG);textAt("A reiniciar Wi-Fi...",20,70,2,CYAN);
      delay(700);ESP.restart();
    }else{
      pref.remove("cal");calibrated=false;skippedCalibration=false;
      calStep=0;drawCal();redraw=false;
    }
  }
}
String htmlPage(){
  return F("<!doctype html><html lang='pt'><meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<style>body{font:16px sans-serif;background:#102034;color:#fff;margin:25px}input,button{padding:12px;width:100%;box-sizing:border-box;margin:7px 0;border-radius:7px}button{background:#34c7ea}</style>"
  "<h2>RJP Mini Hub</h2><p>Configurar Wi-Fi</p><form action='/save' method='post'>"
  "<label>Nome da rede (SSID)</label><input name='ssid' required maxlength='32'>"
  "<label>Palavra-passe</label><input name='pass' type='password' maxlength='64'>"
  "<button type='submit'>Guardar e reiniciar</button></form></html>");
}
void startAP(){
  WiFi.disconnect(true);WiFi.mode(WIFI_AP);
  apMode=WiFi.softAP("RJP-MiniHub-Setup","rjpmini2026");
  if(!apMode){Serial.println("Falha ao iniciar AP");return;}
  server.on("/",HTTP_GET,[]{server.send(200,"text/html; charset=utf-8",htmlPage());});
  server.on("/save",HTTP_POST,[]{
    if(!server.hasArg("ssid")||server.arg("ssid").isEmpty()){
      server.send(400,"text/plain","SSID em falta");return;
    }
    pref.putString("ssid",server.arg("ssid"));pref.putString("pass",server.arg("pass"));
    server.send(200,"text/html; charset=utf-8","<h3>Configuracao guardada. A reiniciar.</h3>");
    delay(700);ESP.restart();
  });
  server.onNotFound([]{server.sendHeader("Location","http://192.168.4.1",true);server.send(302,"text/plain","");});
  server.begin();
  Serial.println("AP RJP-MiniHub-Setup | senha rjpmini2026 | http://192.168.4.1");
}
void startWifi(){
  String ssid=pref.getString("ssid",""), pass=pref.getString("pass","");
  if(ssid.isEmpty()){startAP();return;}
  WiFi.mode(WIFI_STA);WiFi.begin(ssid.c_str(),pass.c_str());
  uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<9000)delay(100);
  if(WiFi.status()==WL_CONNECTED){
    Serial.println(WiFi.localIP());
    configTzTime(TZ_PT,"pool.ntp.org","time.google.com");
  }else startAP();
}

String htmlEscape(String text){
  text.replace("&","&amp;"); text.replace("<","&lt;");text.replace(">","&gt;");
  text.replace("\"","&quot;");text.replace("'","&#39;");
  return text;
}
bool otaIsOpen(){
  return WiFi.status()==WL_CONNECTED && !otaPin.isEmpty() && (int32_t)(otaExpires-millis())>0;
}
bool checkOtaAuth(){
  if(!otaIsOpen()){server.send(403,"text/plain","Ativa OTA LOCAL no ecrã da placa.");return false;}
  if(!server.authenticate("rjp",otaPin.c_str())){server.requestAuthentication();return false;}
  return true;
}
void registerLocalWeb(){
  server.on("/",HTTP_GET,[]{
    server.send(200,"text/html; charset=utf-8",
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<h2>RJP Mini Hub v0.3</h2><p>Ligacao Wi-Fi ativa</p>"
      "<p><a href='/agenda'>Editar agenda local</a></p>"
      "<p>Atualizacoes: abre Definicoes no ecrã e toca em OTA LOCAL.</p>");
  });
  server.on("/agenda",HTTP_GET,[]{
    String html="<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<style>body{font:16px sans-serif;max-width:500px;margin:24px auto;padding:0 16px}"
      "input{padding:10px;width:95%;margin:5px 0 12px}button{padding:12px}</style>"
      "<h2>Agenda local RJP</h2><p>Os eventos aqui criados ficam apenas no Mini Hub, nao sao sincronizados com Google/Outlook.</p>"
      "<form method='post' action='/agenda'>";
    for(int i=0;i<3;i++){
      String title=pref.getString(("ev"+String(i)+"t").c_str(),"");
      String when=pref.getString(("ev"+String(i)+"d").c_str(),"");
      html+="<h4>Compromisso "+String(i+1)+"</h4><label>Data e hora (AAAA-MM-DD HH:MM)</label>"
        "<input maxlength='16' name='date"+String(i)+"' value='"+htmlEscape(when)+"'>"
        "<label>Descricao</label><input maxlength='55' name='title"+String(i)+"' value='"+htmlEscape(title)+"'>";
    }
    html+="<button type='submit'>Guardar no Mini Hub</button></form>";
    server.send(200,"text/html; charset=utf-8",html);
  });
  server.on("/agenda",HTTP_POST,[]{
    for(int i=0;i<3;i++){
      String title=server.arg("title"+String(i)).substring(0,55);
      String date=server.arg("date"+String(i)).substring(0,16);
      title.trim();date.trim();
      pref.putString(("ev"+String(i)+"t").c_str(),title);
      pref.putString(("ev"+String(i)+"d").c_str(),date);
    }
    redraw=true;
    server.sendHeader("Location","/agenda",true);
    server.send(303,"text/plain","");
  });
  server.on("/update",HTTP_GET,[]{
    if(!checkOtaAuth())return;
    server.send(200,"text/html; charset=utf-8",
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<h2>Atualizacao RJP Mini Hub</h2><p>Seleciona apenas o ficheiro <b>-app.bin</b> do artefacto GitHub Actions.</p>"
      "<p>O firmware so sera instalado se a validacao pelo ESP32 for bem sucedida.</p>"
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='firmware' accept='.bin' required><button>Instalar OTA</button></form>");
  });
  server.on("/update",HTTP_POST,[]{
    if(!checkOtaAuth())return;
    if(otaSucceeded){
      server.send(200,"text/html","<h2>Firmware recebido e validado. A reiniciar...</h2>");
      otaRebootAt=millis()+1800;
    }else{
      server.send(400,"text/plain","Falha na atualizacao; o firmware anterior foi mantido.");
    }
  },[]{
    HTTPUpload &upload=server.upload();
    if(upload.status==UPLOAD_FILE_START){
      otaSucceeded=false;
      otaUploading=otaIsOpen() && server.authenticate("rjp",otaPin.c_str()) &&
                   upload.filename.endsWith(".bin");
      if(otaUploading)otaUploading=Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);
    }else if(upload.status==UPLOAD_FILE_WRITE && otaUploading){
      if(Update.write(upload.buf,upload.currentSize)!=upload.currentSize){
        otaUploading=false;Update.abort();
      }
    }else if(upload.status==UPLOAD_FILE_END){
      if(otaUploading)otaSucceeded=Update.end(true) && !Update.hasError();
      otaUploading=false;
    }else if(upload.status==UPLOAD_FILE_ABORTED){
      if(otaUploading)Update.abort();
      otaUploading=false;
    }
  });
  server.begin();
  Serial.print("Servidor local: http://");Serial.println(WiFi.localIP());
}

void setup(){
  Serial.begin(115200);delay(300);
  pinMode(BOOT_BUTTON,INPUT_PULLUP);
  pinMode(TFT_BACKLIGHT,OUTPUT);digitalWrite(TFT_BACKLIGHT,HIGH);
  tft.init();tft.setRotation(1);tft.fillScreen(BG);
  textAt("RJP MINI HUB",45,85,4,CYAN);
  textAt("ESP32-2432S028",70,125,2,MUTED);
  tft.fillRect(28,161,82,13,TFT_RED);
  tft.fillRect(119,161,82,13,TFT_GREEN);
  tft.fillRect(210,161,82,13,TFT_BLUE);
  delay(1200);
  touchSPI.begin(TOUCH_CLK,TOUCH_MISO,TOUCH_MOSI,TOUCH_CS);
  if(!touch.begin(touchSPI))Serial.println("Aviso: touch nao inicializado");
  touch.setRotation(0);
  pref.begin("rjpmini",false);
  calibrated=pref.getBool("cal",false);
  if(calibrated){
    offX=pref.getFloat("ox",0);mulXX=pref.getFloat("xx",1);mulXY=pref.getFloat("xy",0);
    offY=pref.getFloat("oy",0);mulYX=pref.getFloat("yx",0);mulYY=pref.getFloat("yy",1);
  }
  startWifi();
  if(!apMode && WiFi.status()==WL_CONNECTED)registerLocalWeb();
  if(calibrated)redraw=true;
  else{calStep=0;drawCal();redraw=false;}
}
void loop(){
  if(apMode || WiFi.status()==WL_CONNECTED)server.handleClient();
  if(otaRebootAt && (int32_t)(millis()-otaRebootAt)>=0)ESP.restart();
  if(page==1 && calibrated && WiFi.status()==WL_CONNECTED &&
     (trainLastAttempt==0 || millis()-trainLastAttempt>90000UL)){
    header("RJP TRAIN LIVE");textAt("A consultar feed ferroviario...",10,115,2,CYAN);
    fetchTrainFeed();redraw=true;
  }
  bool down=digitalRead(BOOT_BUTTON)==LOW;
  if(bootWasDown && !down && millis()-lastButtonEvent>230){
    lastButtonEvent=millis();
    if(!calibrated){
      calibrated=true;skippedCalibration=true;page=0;redraw=true;
      Serial.println("BOOT: calibracao ignorada temporariamente");
    }else{
      page=(page+1)%6;redraw=true;
      Serial.printf("BOOT: pagina %d\n",page);
    }
  }
  bootWasDown=down;
  if(touch.touched()){
    if(!touching){
      touching=true;
      TS_Point p=touch.getPoint();
      if(p.z>100){
        if(!calibrated)processCalibration(p);
        else{
          int sx=roundf(offX+mulXX*p.x+mulXY*p.y);
          int sy=roundf(offY+mulYX*p.x+mulYY*p.y);
          onTap(constrain(sx,0,SCREEN_W-1),constrain(sy,0,SCREEN_H-1));
        }
      }
    }
  }else touching=false;
  if(calibrated){
    if(redraw){drawPage();redraw=false;lastClock=millis();}
    if(millis()-lastClock>=30000){drawPage();lastClock=millis();}
  }
  delay(15);
}
