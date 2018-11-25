//**************************IHM**********************************************************
//AUTOR: Johnatam Renan Horst
//CURSO: Engenharia de Controle e Automação
//DATA: 06/09/2018
//PROJETO: 
//DESCRISAO:
//
//
//
//
//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Keypad.h>
#include <SPI.h>
#include <TFT_eSPI.h>      
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#define bomba D1
#define valvula D2
#define vazao A0
#define buttonConf D3
#define resetParametros D4
//Wifi Acess Point/Roteador da rede wifi
String ssid_AP     = "wifi_Johnatan";
String password_AP = "99434266";
//MQTT 
String brokerUrl   = "192.168.0.107";        //URL do broker MQTT 
String brokerPort  = "1883";                 //Porta do Broker MQTT
String brokerID    = "IHM";
String brokerUser  =  "IHM";                 //Usuário Broker MQTT
String brokerPWD   = "IHM";                  //Senha Broker MQTT
String topicPubControleSensorVazao = "CONTROLE/SENSOR_VAZAO";     //Topico de pubscrive do sensor Vazao
String topicSubBomba = "BOMBA/BOMBA";                  //Topico de subscrive da Bomba
String topicSubValvula = "BOMBA/VALVULA";              //Topico de subscrive Valvula
//Acess Point ssid e pwd
String user_config = "admin";
String pwd_config  = "admin";
String ssid_config = "Configuration Module";     //ssid REDE WIFI para configuração
String password_config = "espadmin";             //senha REDE WIFI para configuração
//Objetos
WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);
void loop_config();
void handle_configuration_save();
void handle_setup_page();
void handle_login();
bool is_authentified();
void Wi_Fi();
void MQTT();

bool configuration = false;
bool varBomba = false;               //Preset False para Bomba
bool varValvula = false;             //Preset False para Valvula
unsigned float varVazao = 0.0;        //Preset de vazao = 0.0
unsigned int auxVazao 0;
unsigned int tempo = 0;
void setup(){
    Serial.begin(9600);
    pinMode(bomba,OUTPUT);
    pinMode(valvula,OUTPUT);
    pinMode(vazao,OUTPUT);
    pinMode(buttonConf,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonConf),confInterrupt,RISING);
    attachInterrupt(digitalPinToInterrupt(vazao),vazaoInterrupt,RISING);
    EEPROM.begin(512);
    if(digitalRead(resetParametros) > 200){      ///esquema pois tive q resetar modulo para conectar ao broker depois de confgurado
      EEPROM.put(0, ssid_AP);       //se ageitar config exluir este botao e esta logica
      EEPROM.put(20, password_AP);
      EEPROM.put(40, password_AP);
      EEPROM.put(60, brokerUrl);
      EEPROM.put(80, brokerPort);
      EEPROM.put(100, brokerID);
      EEPROM.put(120, brokerUser);
      EEPROM.put(140, brokerPWD);
      EEPROM.put(160, brokerTopic);
      EEPROM.put(180, user_config);
      EEPROM.put(200, pwd_config);
      EEPROM.put(220, ssid_config);
      EEPROM.put(200, pwd_config);
      EEPROM.commit();
      EEPROM.end();
    }else{
      EEPROM.get(0, ssid_AP);
      EEPROM.get(20, password_AP);
      EEPROM.get(40, password_AP);
      EEPROM.get(60, brokerUrl);
      EEPROM.get(80, brokerPort);
      EEPROM.get(100, brokerID);
      EEPROM.get(120, brokerUser);
      EEPROM.get(140, brokerPWD);
      EEPROM.get(160, brokerTopic);
      EEPROM.put(180, user_config);
      EEPROM.put(200, pwd_config);
      EEPROM.put(220, ssid_config);
      EEPROM.put(200, pwd_config);
      EEPROM.end();
      }
}

void loop(){

    if(configuration){
      WiFi.disconnect(true);
      MQTTServer.disconnect();
      loop_config();        
    }else{
      Wi_Fi();
      MQTT();
      MQTTServer.loop();           
    }
    
    
}
////*****************ITERRUPÇÂO PARA vazao*********************************
void vazaoInterrupt(){
  auxVazao++; 
  }

////*****************ITERRUPÇÂO PARA CONFIGUTAÇÂO*********************************
void confInterrupt(){
  configuration = true; 
  }

//**************FUNÇÃO QUE VERIFICA AUTENTIFICAÇÃO DE LOGIN******************************
//retorna true e false conforme o cookie estiver com o ID correto
bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {           
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}
////*****************escrever*********************************
void handle_login() {
  String msg;
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
  }
  if (server.hasArg("DISCONNECT")) {                              //adicionar o argumento disconect quando chegar a pagina save
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    if (server.arg("USERNAME") == user_config &&  server.arg("PASSWORD") == pwd_config) {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      return;
    }
    msg = "Wrong username/password! try again.";
  }
  String content = "<html><body><form action='/login' method='POST'>";
  content += "<br>";
  content += "<center>User:<br><input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "<br>Password:<br><input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<br><br><input type='submit' name='SUBMIT' value='Login'></form>";
  content +=  "<br><h4>"+msg+"</h4>";
  server.send(200, "text/html", content);
}
////*****************FUNÇÃO DA PAGINA INICIAL/CONFIGURAÇÃO*********************************
void handle_setup_page(){
  String header;
  EEPROM.get(0, ssid_AP);
  EEPROM.get(20, password_AP);
  EEPROM.get(40, password_AP);
  EEPROM.get(60, brokerUrl);
  EEPROM.get(80, brokerPort);
  EEPROM.get(100, brokerID);
  EEPROM.get(120, brokerUser);
  EEPROM.get(140, brokerPWD);
  EEPROM.get(160, brokerTopic);
  EEPROM.get(180, user_config);
  EEPROM.get(200, pwd_config);
  EEPROM.get(220, ssid_config);
  EEPROM.get(200, pwd_config);
  EEPROM.end();
  if (!is_authentified()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }else {
  String html = "<html><head><title>Configuration Modulo IHM</title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";  
  html += "</head><body>";
  html += "<h1><center>Configuracao Broker and Acess Parametros</h1>";                     //1 center foi suficiennte
 
  html += "<p><center>SSID do AP</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";                       //config_salve
  html += "<input type=text name=ssid_AP placeholder='" + ssid_AP + "'/> ";
  
  html += "<p>Password do AP</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=password_AP placeholder='" + password_AP + "'/> ";     
 
  html += "<p>IP/URL Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerUrl placeholder='" + brokerUrl + "'/> ";    
  
  html += "<p>Porta do Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerPort placeholder='" + brokerPort + "'/> ";    

  html += "<p>ID da Placa</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerID placeholder='" + brokerID + "'/> ";    
  
  
  html += "<p>ID da Placa</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerUser placeholder='" + brokerUser + "'/> ";    
  
  html += "<p>Senha de Acesso ao Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerPWD placeholder='" + brokerPWD + "'/> ";    

  html += "<p>Usúario de Acesso</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=Usuario Para Acesso  placeholder='" + user_config + "'/> ";    
  
  html += "<p>Senha de Acesso</p>";                                                   //campo para obter periodo de envio da informação de leitura do sensor
  html += "<form method='POST' action='/'>";  
  html += "<input type=text name=pwd_config placeholder='" + pwd_config + "'/> ";
  
  html += "<p>SSID da Rede Wireless Para Acesso</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=ssid_config  placeholder='" + ssid_config + "'/> ";    
 
  html += "<p>Senha da Rede Wireless Para Acesso</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=password_config  placeholder='" + pwd_config + "'/> ";    
   
  html += "</p>";
  html += "<input type=submit name=botao value=Enviar /></p>";  
  html += "</form>"; 
  html += "</body></html>";
  server.send(200, "text/html", html);
  }
}
//*****************FUNÇÃO DA PAGINA DE SALVAMENTO DA CONFIGURAÇÃO************************  
  void handle_configuration_save(){
  String html =  "<html><head><title>Saved Settings</title>";
  html += "<style>body { background-color: #cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; }</style>";
  html += "</head><body>";
  html += "<h1><center>Saved Settings</h1>";
  html += "<h3>...Reset Modulo</h3>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  if(server.arg("ssid_AP") != ""){
     ssid_AP     = server.arg("ssid_AP");
  }                                                    //pega valor do html da input com nome = ssid_AP
  if(server.arg("password_AP") != ""){
    password_AP = server.arg("password_AP");                                        //pega valor do html da input com nome = password
  }
  if(server.arg("brokerUrl") != ""){
    brokerUrl   = server.arg("brokerUrl");
  }
  if(server.arg("brokerPort") != ""){
    brokerPort  = server.arg("brokerPort");
  }
  if(server.arg("brokerID") != ""){
    brokerID    = server.arg("BrokerID");
  }
  if(server.arg("brokerUser") != ""){
    brokerUser  = server.arg("brokerUser");
  }
  if(server.arg("brokerPWD") != ""){
    brokerPWD   = server.arg("brokerPWD");
  }
  if(server.arg("userConfig") != ""){
    user_config  = server.arg("user_config");
  }
  if(server.arg("pwdConfig") != ""){
    pwd_config   = server.arg("pwd_config");
  }
  if(server.arg("ssid_config") != ""){
    ssid_config  = server.arg("ssid_config");
  }
  if(server.arg("password_config") != ""){
    password_config   = server.arg("password_config");
  }
  EEPROM.put(0, ssid_AP);       
  EEPROM.put(20, password_AP);
  EEPROM.put(40, password_AP);
  EEPROM.put(60, brokerUrl);
  EEPROM.put(80, brokerPort);
  EEPROM.put(100, brokerID);
  EEPROM.put(120, brokerUser);
  EEPROM.put(140, brokerPWD);
  EEPROM.put(160, brokerTopic);
  EEPROM.put(180, user_config);
  EEPROM.put(200, pwd_config);
  EEPROM.put(220, ssid_config);
  EEPROM.put(200, pwd_config);
  EEPROM.commit();
  EEPROM.end();
  Serial.println(ssid_AP);
  configuration = false;
  WiFi.disconnect();
}

//********************FUNÇÃO QUE EXECUTA A PARTE DE CONFIGURAÇÃO***********************
    void loop_config(){
   Serial.println("Configuration is Started!!!");
        delay(2000);                                                        //2 segundos para filtrar ruido do botao
        WiFi.softAP(ssid_config.c_str(),password_config.c_str(),1,!configuration,2);        //cria a rede com o parametro para configuração
        Serial.println(WiFi.softAPIP());
        server.on("/", handle_setup_page);
        server.on("/login",handle_login);
        server.on("/config_save",handle_configuration_save);
        server.on("/inline", []() {server.send(200, "text/plain", "this works without need of authentification"); });
        const char * headerkeys[] = {"User-Agent", "Cookie"} ;
        size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);  
        server.collectHeaders(headerkeys, headerkeyssize);
        server.begin();
        server.sendHeader("Set-Cookie", "ESPSESSIONID=0");        //precisei forçar esta flag para rodar o login durente teste
        server.handleClient();
        while(configuration){
          server.handleClient();
        }
         WiFi.disconnect();
  }
//******************FUNÇÃO QUE CONECTA AO BROKER MQTT*********************************** 
void MQTT(){
  if(MQTTServer.connected() == 1 ){                                       //CONECTADO
    Serial.println("MQTT Conectado!");    
    Serial.print("Status: ");                                 //STATUS NAO OK
    switch (MQTTServer.state()) {
      case -4:
        Serial.println("MQTT_CONNECTION_TIMEOUT");
      break;
      case -3:
        Serial.println("MQTT_CONNECTION_LOST");
      break;
      case -2:
        Serial.println("MQTT_CONNECT_FAILED");
      break;
      case -1:
        Serial.println("MQTT_DISCONNECTED");
      break;
      case 0:
        Serial.println("MQTT_CONNECTED");
        return;
      break;
      case 1:
        Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
      break;
      case 2:
        Serial.println("MQTT_CONNECT_UNAVAILABLE");
      break;
      case 3:
        Serial.println("MQTT_CONNECTION_TIMEOUT");
      break;
      case 4:
        Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
      case 5:
        Serial.println("MQTT_CONNECT_UNAUTHORIZED");
      break;
      default:
        Serial.println("Erro Desconhecido!!");
      break;
      MQTTServer.disconnect();
      goto InitMQTT;
    }    
  }else if(MQTTServer.connected() == 0 ){                                 //DESCONECTADO
    InitMQTT:
    Serial.println("MQTT Desconectado!... Configuracao MQTT");
    Serial.println(brokerUrl);
    MQTTServer.setServer(brokerUrl.c_str(), atoi(brokerPort.c_str()));
    MQTTServer.setCallback(subscrive);
    long timerMQTT = millis();
    while(MQTTServer.connect(brokerID.c_str()) != 1){                     //WHILE DESCONECTADO
      delay(100);
      Serial.println(".");
      if((millis()-timerMQTT)>7000){
        Serial.println("Falha na Coneccao!");
        return;
      }                        
    }
//*****************************RECONECTA WI.FI*****************************************************
//se já está conectado a rede WI-FI, nada é feito. 
//Caso contrário, são efetuadas tentativas de conexão 
void Wi_Fi(){
    if (WiFi.status() == WL_CONNECTED){
      return;
    }
    delay(10);
    Serial.println("Configurando WiFi!"); 
    Serial.print(ssid_AP);
    Serial.println(password_AP);   
    WiFi.begin(ssid_AP.c_str(), password_AP.c_str());                  // Conecta na rede WI-FI    
    long timerWifi = millis();
    while (WiFi.status() != WL_CONNECTED){
      delay(100);
      Serial.println("..");
      if((millis()-timerWifi)>10000){
        return;
      }
    }  
    Serial.print("Conectado com sucesso na rede: ");
    Serial.println(ssid_AP);
    Serial.print(" IP obtido: ");
    Serial.println(WiFi.localIP());
    return;
}
void subscrive(char* topic, byte* payload, unsigned int length){
  char resposta[length];
  Serial.println("Resceiver msg!!");
  String topico;
  String msg;
  String aux;
  int valor;

  for(int i = 0; i <= length; i++){                  //Transforma o topico String Para manipulação
     resposta[i] = (char)topic[i];
     topico += resposta[i];
     }
  Serial.println(topico);
  for(int i = 0; i <= length; i++){                  //Transforma a mensagem recebida em String Para manipulação
     resposta[i] = (char)payload[i];
     msg += resposta[i];
     }
   
}

