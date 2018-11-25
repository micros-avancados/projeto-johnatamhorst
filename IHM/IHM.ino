//**************************IHM**********************************************************
//AUTOR: Johnatam Renan Horst
//CURSO: Engenharia de Controle e Automação
//DATA: 06/09/2018
//PROJETO: 
//DESCRISAO: Modulo IHM do Projeto
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
//Wifi Acess Point/Roteador da rede wifi
String ssid_AP     = "wifi_Johnatan";
String password_AP = "99434266";
//MQTT 
String brokerUrl   = "192.168.0.107";        //URL do broker MQTT 
String brokerPort  = "1883";                 //Porta do Broker MQTT
String brokerID    = "IHM";
String brokerUser  =  "IHM";                 //Usuário Broker MQTT
String brokerPWD   = "IHM";                  //Senha Broker MQTT
String topicSubscrive = "IHM";                //Topico de subscrive da IHM
String brokerTopic = "as";                  //Topico
String topicSubscriveControle = "CONTROLE";      //Topico de cubscrive do sensor
String topicSubscriveSensor = "SENSOR";      //Topico de cubscrive do sensor
//Acess Point ssid e pwd
const char* ssid_config = "Configuration Module";     //ssid REDE WIFI para configuração
const char* password_config = "espadmin";             //senha REDE WIFI para configuração
//Login page
String userConfig = "admin";
String pwdConfig  = "admin";
//Objetos
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);
const byte ROWS = 4;                                  //linha do teclado Matricial
const byte COLS = 3;                                  //Colunas do teclado matricial
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'E','0','M',}
};
byte rowPins[ROWS] = {D0,D1,D2,D3}; 
byte colPins[COLS] = {10,D6,D4}; 
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//Prototipo das funções utilizadas
void atualizaTela(int setPoint, int nivelTanque, String tempoRestante, String tempoTotal ,int litrosTanque );
void loop_config();
void subscrive(char* topic, byte* payload, unsigned int length);
void handle_configuration_save();
void handle_setup_page();
void handle_login();
bool is_authentified();
void configuration_ISR();
void Wi_Fi();
void initMQTT();
//descomentar depois void answerBroker(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
char tecla;
bool configuration = false;
//Variaveis de Controle
int setPoint = 0;                       //Preset de SetPoint 
String tempoRestante = "00:00:00";      //Preset de 0 tempo;
String tempoTotal = "00:01:00";         //Preset de tempo total de 1hora
bool sensorHigh = false;                //Preset false no sensor de nivel High
bool sensorLow = false;                 //Preset false no sensor de nivel Low
int litrosTanque = 50;                  //Preset de 50 litros total do tanque
int nivelTanque = 50;                    //Preset de 0% de nivel no tanque
#define topicoSubscrive "IHM"
//funcao setup
void setup(){
    Serial.begin(9600);
    keypad.addEventListener(keypadEvent); // Add an event listener for this keypad
    tft.init();
    tft.setRotation(2);    
    atualizaTela( setPoint, nivelTanque, tempoRestante, tempoTotal ,litrosTanque );

    EEPROM.begin(512);
    if(analogRead(A0) > 200){      ///esquema pois tive q resetar modulo para conectar ao broker depois de confgurado
      EEPROM.put(0, ssid_AP);       //se ageitar config exluir este botao e esta logica
      EEPROM.put(20, password_AP);
      EEPROM.put(40, password_AP);
      EEPROM.put(60, brokerUrl);
      EEPROM.put(80, brokerPort);
      EEPROM.put(100, brokerID);
      EEPROM.put(120, brokerUser);
      EEPROM.put(140, brokerPWD);
      EEPROM.put(160, brokerTopic);
      EEPROM.put(180, userConfig);
      EEPROM.put(200, pwdConfig);
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
      EEPROM.get(180, userConfig);
      EEPROM.get(200, pwdConfig);
      EEPROM.end();
      }
}

void loop(){
    if(tecla=='M'){
      MQTTServer.disconnect();
      WiFi.disconnect(true);
      configuration = true;
      loop_config();
      tecla='0';         
    }else{
      Wi_Fi();
      MQTT();
      MQTTServer.loop();           
    }
    niveisTela(nivelTanque);
    char key = keypad.getKey();
    tecla=key;
    if (key) {
        Serial.print(key);
        //MQTTServer.publish(topicSubscriveSensor.c_str(), String(key).c_str() );    
    }
}

//********************FUNÇÃO QUE EXECUTA A PARTE DE CONFIGURAÇÃO***********************
void loop_config(){
    delay(2000);                                                        //2 segundos para filtrar ruido do botao
    WiFi.softAP(ssid_config,password_config,1,!configuration,2);        //cria a rede com o parametro para configuração
    Serial.println(WiFi.softAPIP());
    server.on("/", handle_setup_page);
    server.on("/login",handle_login);
    server.on("/config_save",handle_configuration_save);
    server.on("/inline", []() {server.send(200, "text/plain", "this works without need of authentification"); });
    const char * headerkeys[] = {"User-Agent", "Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);  
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    Serial.println("Server Inicializado");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");        //precisei forçar esta flag para rodar o login durente teste
    server.handleClient();
    while(configuration){
      server.handleClient();
    }
    WiFi.disconnect();
  //ESP.restart();  
  }                                             
//*******************FUNÇÃO Q RECONECTA AO BROKER ***************************************
//reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
// em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
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
    Serial.println(topicoSubscrive);
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
    MQTTServer.subscribe(topicSubscriveControle.c_str(),0);
    if(MQTTServer.subscribe(topicSubscriveSensor.c_str(),0)){
      Serial.println("SUBSCRIVE ok!!");
    }else{
      Serial.println("ERRO SUBSCRIVE");
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

//*********************SUBCRIVER MODE***********************
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
    
  if(topico == topicSubscriveControle){                     //Mensagem do Modulo de Controle
     for(int i = 0; i <= 7; i++){                   //Transforma a mensagem recebida em String Para manipulação
     resposta[i] = (char)payload[i];
     msg += resposta[i];
     }     
     tempoRestante = msg;                           //armagena o tempo restante, somente 1 informação   
     for(int i = 8; i <= length; i++){               //Transforma a mensagem recebida em String Para manipulação
     resposta[i] = (char)payload[i];
     msg += resposta[i];
     }
     tempoTotal = msg;                              //armagena o tempo total, somente 1 informação                                
    //######Salva tempo Restante
    //######Salva tempo Total
  }
  
  if(topico == topicSubscriveSensor){               //Mensagem do Modulo Sensor
    for(int i = 0; i <= 2; i++){                     //Obtem somente informação de nivel, posição(1,2,3)                
      aux += (char)msg[i];
    }
    Serial.println(atoi(aux.c_str()));
    nivelTanque = atoi(aux.c_str());
    sensorLow = bool(msg[4]);
    sensorHigh = bool(msg[5]);
    for(int i = 6; i <= length; i++){                  //Transforma a mensagem recebida em String Para manipulação
      resposta[i] = (char)payload[i];
      aux += resposta[i];
    }
    litrosTanque = atoi(aux.c_str());
    
    //######Salva nivel
    //######Salva sensor nivel Low
    //######Salva sensor nivel HIGH
  }  
}
////*****************CONTROLA O NIVEL DA TELA*********************************
void niveisTela(int nivelTanque){
  tft.fillEllipse(170, 180, 30, 10, TFT_BLUE);
  int nivel = map(nivelTanque,0,100,175,30);         //nivel do Tanque vem em Porcentagem  
    for(int i = 175; i > nivel; i--){                  //for para atualizar nivel no tubo                          
      tft.fillEllipse(170, i, 40, 10, TFT_BLUE);
    }
    for(int i = nivel; i > 50; i--){                          
    tft.fillEllipse(170, i, 40, 10, TFT_WHITE);
    }
    tft.fillEllipse(170, 26, 50, 20, TFT_BLACK);
    tft.fillEllipse(170, 26, 30, 10, TFT_WHITE);
  
  
  
}

////*****************CONTROLA A TELA*********************************
void atualizaTela(int setPoint, int nivelTanque, String tempoRestante, String tempoTotal ,int litrosTanque ){

  //###
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
    tft.fillEllipse(170, 26, 50, 20, TFT_BLACK);
    tft.fillEllipse(170, 26, 30, 10, TFT_WHITE);
    tft.fillRect(120, 26, 10, 150, TFT_BLACK);
    tft.fillRect(211, 26, 10,150, TFT_BLACK);    
    tft.fillEllipse(170, 180, 50, 20, TFT_BLACK);
    tft.fillEllipse(170, 180, 30, 10, TFT_WHITE);
    
    niveisTela(nivelTanque);
    
    tft.setTextSize(2);
    tft.setCursor(10,10);
    tft.println("SET-POINT");
    tft.setCursor(30,30);
    tft.println(setPoint);                                  //Posição do valor de SetPoint
    
    tft.setCursor(10,80);
    tft.println("Litros");
    tft.setCursor(30,100);
    tft.println((litrosTanque*nivelTanque)/100);            //Litros no Tanque no Painel
    
    tft.setCursor(10,150);
    tft.println("Tp Total:");
    tft.setCursor(10,170);
    tft.println(tempoTotal);
   
    tft.setCursor(10,220);
    tft.println("Tempo Restante:");
    tft.setCursor(10,240);
    tft.println(tempoRestante);                               //Imprime o Tempo Total para encher
    
  //###  
    
    tft.fillRect(10, 270, 220, 30, TFT_BLACK);                //retangulo de porcentagem
    tft.fillRect(12, 272, 216, 26, TFT_WHITE); 
  
  }
////*****************escrever*********************************
void keypadEvent(KeypadEvent key){ 
    switch (keypad.getState()){        
    case PRESSED:
          Serial.println("Pressed");
        break;
    case RELEASED:
            Serial.println("Realeased");
        break;
    case HOLD:
            Serial.println("Hold");
            tecla=key;
            //configuration = true;
        break;
    }
}
//**************FUNÇÃO QUE VERIFICA AUTENTIFICAÇÃO DE LOGIN******************************
//retorna true e false conforme o cookie estiver com o ID correto
bool is_authentified() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("ESPSESSIONID=1") != -1) { 
      return true;
    }
  }
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
    if (server.arg("USERNAME") == userConfig &&  server.arg("PASSWORD") == pwdConfig) {
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
 // WiFi.disconnect(true);
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
  EEPROM.get(180, userConfig);
  EEPROM.get(200, pwdConfig);
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
  html += "<h1><center>Configuration Broker and Acess Parameters</h1>";                     //1 center foi suficiennte
 
  html += "<p><center>SSID</p>";                                                      //campo para obter ssid da rede wifi com acesso a internet
  html += "<center><form method='POST' action='/config_save'>";                       //config_salve
  html += "<input type=text name=ssid_AP placeholder='" + ssid_AP + "'/> ";
  
  html += "<p>Password</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=password_AP placeholder='" + password_AP + "'/> ";     
 
  html += "<p>IP/URL Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerUrl placeholder='" + brokerUrl + "'/> ";    
  
  html += "<p>Porta do Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerPort placeholder='" + brokerPort + "'/> ";    

  html += "<p>User Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerID placeholder='" + brokerID + "'/> ";    
  
  
  html += "<p>User Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerUser placeholder='" + brokerUser + "'/> ";    
  
  html += "<p>Senha Broker</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerPWD placeholder='" + brokerPWD + "'/> ";    
  
  html += "<p>Topico</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=brokerTopic placeholder='" + brokerTopic + "'/> ";    
  
  html += "<p>Acess User</p>";                                                          //campo para obter password da rede wifi com acesso a internet
  html += "<form method='POST' action='/'>";
  html += "<input type=text name=userConfig placeholder='" + userConfig + "'/> ";    
  
  html += "<p>Acess PassWord</p>";                                                   //campo para obter periodo de envio da informação de leitura do sensor
  html += "<form method='POST' action='/'>";  
  html += "<input type=text name=pwdConfig placeholder='" + pwdConfig + "'/> ";
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
  Serial.println(password_AP);
  Serial.println(ssid_AP);
  if(server.arg("brokerUrl") != ""){
    brokerUrl   = server.arg("brokerUrl");
  }
  if(server.arg("brokerPort") != ""){
    brokerPort  = server.arg("brokerPort");
  }
  if(server.arg("BrokerID") != ""){
    brokerID    = server.arg("BrokerID");
  }
  if(server.arg("brokerUser") != ""){
    brokerUser  = server.arg("brokerUser");
  }
  if(server.arg("brokerPWD") != ""){
    brokerPWD   = server.arg("brokerPWD");
  }
  if(server.arg("brokerTopic") != ""){
    brokerTopic = server.arg("brokerTopic");
  }
  if(server.arg("userConfig") != ""){
    userConfig  = server.arg("userConfig");
  }
  if(server.arg("pwdConfig") != ""){
    pwdConfig   = server.arg("pwdConfig");
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
  EEPROM.put(180, userConfig);
  EEPROM.put(200, pwdConfig);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("REDEEEEE");
  Serial.println(ssid_AP);
  Serial.println(pwdConfig);
  configuration = false;
  WiFi.disconnect();
}

