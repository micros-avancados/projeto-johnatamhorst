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
#include <Ticker.h>
#define sensorHigh D1
#define sensorLow D2
#define nivelTanque A0
#define buttonConf D7
#define resetParametros D5
//Wifi Acess Point/Roteador da rede wifi
String ssid_AP     = "wifi_Johnatan";
String password_AP = "99434266";
//MQTT 
String brokerUrl   = "192.168.0.190";                                 //URL do broker MQTT 
String brokerPort  = "1883";                                          //Porta do Broker MQTT
String brokerID    = "SENSOR";
String brokerUser  =  "";                                       //Usuário Broker MQTT
String brokerPWD   = "";                                        //Senha Broker MQTT                                          //Topico
String topicPubControleSensorHigh = "CONTROLE/SENSOR_HIGH";     //Topico de pubscrive do sensor nivel Alto do Tanque
String topicPubControleSensorLow = "CONTROLE/SENSOR_LOW";       //Topico de pubscrive do sensor  nivel Baixo do Tanque
String topicPubControleSensorNivel = "CONTROLE/SENSOR_NIVEL";   //Topico de pubscrive do sensor de nivel do Tanque
String topicPubControleLitrosTanque = "CONTROLE/LITROS_TANQUE"; //Topico de pubscrive a quantidade de litros no Tanque
//Acess Point ssid e pwd
String user_config = "admin";
String pwd_config  = "admin";
String ssid_config = "Configuration Module";     //ssid REDE WIFI para configuração
String password_config = "espadmin";             //senha REDE WIFI para configuração
//Objetos
WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);
Ticker timerPub;
//Var de Controle
bool configuration = false;
bool varSensorHigh = false;           //Preset false nivelTanque high do tanque
bool varSensorLow = false;            //Preset False nivelTanque Low do Tanque
int varNivelTanque = 25;              //Preset de nivel = 0.0
int varLitrosTanque = 70;             //Presety de 50 litros  no tanque
bool pubOk = false;
int aux = 0;
int aux2 = 0;
String msg;
void setup(){
    Serial.begin(9600);
    pinMode(sensorHigh,INPUT_PULLUP);
    pinMode(sensorLow,INPUT_PULLUP);
    pinMode(buttonConf,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonConf),confInterrupt,RISING);
    EEPROM.begin(512);
    //timerPub.attach(1,Pub);
    if(digitalRead(resetParametros) == true){      ///esquema pois tive q resetar modulo para conectar ao broker depois de confgurado
      EEPROM.put(0, ssid_AP);       //se ageitar config exluir este botao e esta logica
      EEPROM.put(20, password_AP);
      EEPROM.put(40, password_AP);
      EEPROM.put(60, brokerUrl);
      EEPROM.put(80, brokerPort);
      EEPROM.put(100, brokerID);
      EEPROM.put(120, brokerUser);
      EEPROM.put(140, brokerPWD);
      EEPROM.put(180, user_config);
      EEPROM.put(200, pwd_config);
      EEPROM.put(220, ssid_config);
      EEPROM.put(240, pwd_config);
      EEPROM.put(260, varLitrosTanque);
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
      EEPROM.get(180, user_config);
      EEPROM.get(200, pwd_config);
      EEPROM.get(220, ssid_config);
      EEPROM.get(240, pwd_config);
      EEPROM.get(260, varLitrosTanque);
      EEPROM.end();
      }
      Wi_Fi();
      MQTT();
      MQTTServer.publish(topicPubControleLitrosTanque.c_str(), String(varLitrosTanque).c_str(),false );
      //delay(500);
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
      varSensorLow = !digitalRead(sensorHigh);
      varSensorHigh = !digitalRead(sensorLow);
      varNivelTanque = map(analogRead(nivelTanque),0,1024,0,100);

      aux++;
      if(aux == 10){ //publicar somente a cada 10´s
      MQTTServer.publish(topicPubControleSensorHigh.c_str(), String(varSensorHigh).c_str(),false );
      //delay(500);
      MQTTServer.publish(topicPubControleSensorLow.c_str(), String(varSensorLow).c_str() ,false);
      //delay(500);
      aux = 0;
      }  
          
      MQTTServer.publish(topicPubControleSensorNivel.c_str(), String(varNivelTanque).c_str() ,0);
      delay(1000);
//      MQTTServer.publish(topicPubControleLitrosTanque.c_str(), String(varLitrosTanque).c_str(),false );
//      delay(500);
//      Serial.print("Nivel: ");
//      Serial.println(varNivelTanque);
     
      
    } 
    
}

////*****************ITERRUPÇÂO PARA CONFIGUTAÇÂO****************************************
void confInterrupt(){
  configuration = true; 
  }

//**************FUNÇÃO QUE VERIFICA AUTENTIFICAÇÃO DE LOGIN******************************
//retorna true e false conforme o cookie estiver com o ID correto
bool is_authentified() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {           
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}
////*****************PAGINA HTML PARA LOGIN***********************************************
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

  html += "<p><center>Total de Litros Tanque</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";                       //config_salve
  html += "<input type=text name=varLitrosTanque placeholder='" + String(varLitrosTanque) + "'/> ";
 
  html += "<p><center>SSID so AP</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
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
  html += "<input type=text name=user_config  placeholder='" + user_config + "'/> ";    
  
  html += "<p>Senha de Acesso</p>";                                                   //campo para obter periodo de envio da informação de leitura do sensor
  html += "<form method='POST' action='/'>";  
  html += "<input type=text name=pwd_config placeholder='" + pwd_config + "'/> ";
  
  html += "<p>SSID da Rede Wireless Para Acesso</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=ssid_config  placeholder='" + ssid_config + "'/> ";    
 
  html += "<p>Senha da Rede Wireless Para Acesso</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=password_config='" + password_config + "'/> ";    
   
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
  if(server.arg("varLitrosTanque") != ""){
     varLitrosTanque  = atoi(server.arg("ssid_AP").c_str());
  }  
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
  EEPROM.put(180, user_config);
  EEPROM.put(200, pwd_config);
  EEPROM.put(220, ssid_config);
  EEPROM.put(240, pwd_config);  
  EEPROM.put(260, varLitrosTanque);
  EEPROM.commit();
  EEPROM.end();
  configuration = false;
  WiFi.disconnect();
}

//********************FUNÇÃO QUE EXECUTA A PARTE DE CONFIGURAÇÃO*************************
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
//******************FUNÇÃO QUE CONECTA AO BROKER MQTT************************************* 
void MQTT(){
  if(MQTTServer.connected() == 1 ){                                       //CONECTADO
    //Serial.println("MQTT Conectado!");    
    //Serial.print("Status: ");                                 //STATUS NAO OK
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
        //Serial.println("MQTT_CONNECTED");
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
    //Serial.println(brokerUrl);
    MQTTServer.setServer(brokerUrl.c_str(), atoi(brokerPort.c_str()));
    //MQTTServer.setCallback(subscrive);
    long timerMQTT = millis();
    while(MQTTServer.connect(brokerID.c_str()) != 1){                     //WHILE DESCONECTADO
      delay(100);
      Serial.println(".");
      if((millis()-timerMQTT)>7000){
        Serial.println("Falha na Coneccao!");
        return;
      }                        
    }
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
    WiFi.begin(ssid_AP.c_str(), password_AP.c_str());                  // Conecta na rede WI-FI    
    long timerWifi = millis();
    while (WiFi.status() != WL_CONNECTED){
      delay(100);
      Serial.println("..");
      if((millis()-timerWifi)>10000){
        return;
      }
    }  
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(" IP obtido: ");
    Serial.println(WiFi.localIP());
    return;
}


