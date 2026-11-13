#pragma once
// RJP Mini Hub: leitura compacta do feed publico RJP Train Live.
// A fonte em 2026 e o Worker RJP, que depende de Comboios Live (Rui Costa).
// So serve para informacao; NUNCA para seguranca ferroviaria.
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char *TRAIN_URL="https://rjp-train-live-api.rjpedro.workers.dev/api/trains/active";
struct TrainInfo {
  String number;
  String from;
  String to;
  String service;
  int delayMin=0;
  bool delayKnown=false;
};
static TrainInfo trainRows[6];
static int trainCount=0;
static String trainError="Sem consulta";
static unsigned long trainLastAttempt=0;
static unsigned long trainLastSuccess=0;

static bool fetchTrainFeed() {
  trainLastAttempt=millis();
  if(WiFi.status()!=WL_CONNECTED){trainError="Wi-Fi desligado";return false;}
  WiFiClientSecure client;
  // Protótipo v0.3: sem material sensivel na chamada. Validacao de certificado pendente.
  client.setInsecure();
  client.setTimeout(15);
  HTTPClient http;
  http.useHTTP10(true); // evitar transfer-encoding chunked no parser de stream JSON
  http.setConnectTimeout(5500);
  http.setTimeout(15000);
  if(!http.begin(client,TRAIN_URL)){trainError="Falha HTTPS";return false;}
  http.addHeader("Accept","application/json");
  int code=http.GET();
  if(code!=HTTP_CODE_OK){
    trainError="HTTP "+String(code);
    http.end();return false;
  }
  // A origem e um array de centenas de comboios com campos grandes.
  // Guardar apenas os campos usados no pequeno ecrã.
  StaticJsonDocument<2048> filter;
  JsonObject f=filter[0].to<JsonObject>();
  f["train_id"]=true;
  f["data"]["status"]["trainNumber"]=true;
  f["data"]["status"]["delay"]=true;
  f["fixed"]["trainNumber"]=true;
  f["fixed"]["delay"]=true;
  f["fixed"]["trainService"]["designation"]=true;
  f["db"]["trainNumber"]=true;
  f["db"]["trainOrigin"]["designation"]=true;
  f["db"]["trainDestination"]["designation"]=true;
  f["db"]["trainService"]["designation"]=true;
  DynamicJsonDocument doc(65000);
  DeserializationError e=deserializeJson(doc,*http.getStreamPtr(),DeserializationOption::Filter(filter));
  http.end();
  if(e){
    trainError="JSON: "+String(e.c_str());
    return false;
  }
  JsonArray arr=doc.as<JsonArray>();
  if(arr.isNull()){
    trainError="Formato inesperado";
    return false;
  }
  int n=0;
  for(JsonObject row : arr){
    if(n>=6)break;
    JsonObject live=row["data"]["status"];
    JsonObject fixed=row["fixed"];
    JsonObject db=row["db"];
    String num=String((const char*)(live["trainNumber"] | ""));
    if(num.isEmpty())num=String((const char*)(fixed["trainNumber"] | ""));
    if(num.isEmpty())num=String((const char*)(db["trainNumber"] | ""));
    if(num.isEmpty())num=String((const char*)(row["train_id"] | ""));
    if(num.isEmpty())continue;
    TrainInfo &t=trainRows[n];
    t.number=num;
    t.from=String((const char*)(db["trainOrigin"]["designation"] | "?"));
    t.to=String((const char*)(db["trainDestination"]["designation"] | "?"));
    t.service=String((const char*)(db["trainService"]["designation"] | ""));
    if(t.service.isEmpty())t.service=String((const char*)(fixed["trainService"]["designation"] | ""));
    t.delayKnown=false;
    if(!live["delay"].isNull()){
      t.delayMin=(int)roundf(live["delay"].as<float>()/60.0f);
      t.delayKnown=true;
    }else if(!fixed["delay"].isNull()){
      t.delayMin=fixed["delay"].as<int>();t.delayKnown=true;
    }else t.delayMin=0;
    n++;
  }
  trainCount=n;
  trainError=n?"":"Sem comboios na fonte";
  if(n)trainLastSuccess=millis();
  return n>0;
}
