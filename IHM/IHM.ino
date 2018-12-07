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
#include <Ticker.h>
#include <Arduino.h>
//Wifi Acess Point/Roteador da rede wifi
String ssid_AP     = "wifi_Johnatan";
String password_AP = "99434266";
//MQTT 
String brokerUrl   = "192.168.0.190";        //URL do broker MQTT 
String brokerPort  = "1883";                 //Porta do Broker MQTT
String brokerID    = "IHM";
String brokerUser  =  "";                                   //Usuário Broker MQTT
String brokerPWD   = "";                                    //Senha Broker MQTT                 
String topicSubscrive = "IHM/#";                            //Topico de cubscrive do generico
String topicPubControleSetpoint = "CONTROLE/SETPOINT";      //lembra se for no nodeMCU colocar p $ no final      
String topicPubControleStart = "CONTROLE/START"; 
String topicSubTempoTotal = "IHM/TEMPO_TOTAL";                  
String topicSubTempoRestante = "IHM/TEMPO_RESTANTE";
String topicSubStatus = "IHM/STATUS";
String topicSubNivel = "IHM/NIVEL"; 
String topicSubLitrosTanque = "IHM/LITROS_TANQUE";
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
Ticker timeTela;
ESP8266WebServer server(80);
const byte ROWS = 4;                                  //linha do teclado Matricial
const byte COLS = 3;                                  //Colunas do teclado matricial
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'P','0','M',}
};
byte rowPins[ROWS] = {D6,10,D4,D3}; 
byte colPins[COLS] = {D0,D1,D2}; 
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

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
int nivelTanque = 50;                   //Preset de 0% de nivel no tanque
int statusTanque = 50;
bool varStart = false;
int nivelControle;
#define topicoSubscrive "IHM"
//funcao setup
void setup(){
    Serial.begin(9600);
    keypad.addEventListener(keypadEvent); // Add an event listener for this keypad
    tft.init();
    tft.setRotation(2);    
    atualizaTela( setPoint, nivelTanque, tempoRestante, tempoTotal ,litrosTanque );
    timeTela.attach(2,interruptTela);
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
      tecla='n';         
    }if (tecla == 'P'){
      timeTela.detach();
      setPoint = telaSetPoint();      
      if(setPoint >= nivelControle){
        setPoint = nivelControle;
      }
      Serial.print("NivelControle: ");
      Serial.println(nivelControle);
      //fazer teste de maximo limite do tanque aki 
      timeTela.attach(2,interruptTela);
      atualizaTela( setPoint, nivelTanque, tempoRestante, tempoTotal ,litrosTanque );
      tecla='n';
    }if (tecla == '1'){
      varStart = true;
      tecla='n';
    }if (tecla == '0'){
      varStart = false;
      tecla='n';
    }else{
      Wi_Fi();
      MQTT();
      MQTTServer.loop();           
    }
     nivelControle = (litrosTanque*nivelTanque)/100;
    //delay(1000);
    char key = keypad.getKey();
    tecla=key;
    if (key) {
        Serial.println(key);  
        Serial.println(varStart); 
    }
}
void interruptTela(){
  atualizaTela( setPoint, nivelTanque, tempoRestante, tempoTotal ,litrosTanque );
  
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
    //MQTTServer.subscribe(topicSubscriveControle.c_str(),0);
    if(MQTTServer.subscribe(topicSubscrive.c_str(),0)){
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
    //Serial.print(ssid_AP);
    //Serial.println(password_AP);    
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
    //Serial.println(ssid_AP);
    Serial.print(" IP obtido: ");
    Serial.println(WiFi.localIP());
    return;
}

//*********************SUBCRIVER MODE***********************
void subscrive(char* topic, byte* payload, unsigned int length){
  char resposta[length];
  //Serial.println("");
  String topico;
  String msg;
  String aux;
  int valor;
  int i=0;
 topico = String(topic);
  for(int i = 0; i <= length; i++){                  //Transforma a mensagem recebida em String Para manipulação
     resposta[i] = (char)payload[i];
     msg += resposta[i];
     }
    
  if(topico.equals(topicSubNivel)){                     //Mensagem do Modulo de Controle
      nivelTanque = atoi(msg.c_str());
      Serial.print("Nivel Tanque: ");
      Serial.println(nivelTanque);
  }
   if(topico.equals(topicSubStatus)){                     //Mensagem do Modulo de Controle
      statusTanque = atoi(msg.c_str());
      Serial.print("Status Tanque: ");
      Serial.println(statusTanque);
  }
   if(topico.equals(topicSubTempoTotal)){                     //Mensagem do Modulo de Controle
      tempoTotal = msg;
      Serial.print("Tempo Total: ");
      Serial.println(tempoTotal);
  }
   if(topico.equals(topicSubTempoRestante)){                     //Mensagem do Modulo de Controle
      tempoRestante = msg;
      Serial.print("Tempo Restante: ");
      Serial.println(tempoRestante);
  }
   if(topico.equals(topicSubLitrosTanque)){                     //Mensagem do Modulo de Controle
      litrosTanque = atoi(msg.c_str());
      //Serial.print("litros Tanque: ");
      //Serial.println(litrosTanque);
  }
  MQTTServer.publish(topicPubControleSetpoint.c_str(), String(setPoint).c_str() ); 
  MQTTServer.publish(topicPubControleStart.c_str(), String(varStart).c_str() ); 
}
////*****************CONTROLA O NIVEL DA TELA*********************************
void nivelTela(int nivelTanque){

  //resolver problema com o 100
  if(nivelTanque >= 100){
      nivelTanque = 99;
  }else if(nivelTanque<0){
      nivelTanque=0;
  }
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
void statusTela(int statusTanque, bool start){
  
  if(statusTanque >= 100){
      statusTanque = 99;
  }else if(statusTanque<0){
      statusTanque=0;
  }
  if(start){
    statusTanque = map(statusTanque,0,100,12,216);
    tft.fillRect(12, 272, statusTanque, 26, TFT_BLUE);
    tft.fillRect(12 + statusTanque, 272 , 216 - statusTanque, 26, TFT_WHITE);
  }else{
    tft.fillRect(10, 270, 220, 30, TFT_BLACK);  
  }
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
    
    
    nivelTela(nivelTanque);
    statusTela(statusTanque,true);
    tft.setTextSize(2);
    tft.setCursor(10,10);
    tft.println("SET-POINT");
    tft.setCursor(30,30);
    tft.println(setPoint);                                  //Posição do valor de SetPoint
    
    tft.setCursor(10,80);
    tft.println("Litros");
    tft.setCursor(30,100);
    tft.println(litrosTanque);            //Litros no Tanque no Painel
    
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
int telaSetPoint(){
    int auxTecla = 1;
    int teclaS;
    char keyS = keypad.getKey();
    int setPointS = 0;
    tecla = 'n';
    while(true){
      tft.fillScreen(TFT_GREEN);
      tft.setTextColor(TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(50,100);
      tft.println("SET-POINT");
      tft.setCursor(50,150);
      tft.println(setPointS);
      //delay(1000);
      //Serial.println(keypad.getKeys());
      while(!keypad.getKeys()){
        Serial.println(".");
        delay(200);
        }
      keyS = tecla;
      if( keyS != 'P' && keyS != 'M'){
        if(((int)keyS-48) > 0 && ((int)keyS-48) < 10 ){
          teclaS = ((int)keyS - 48);
          setPointS *= auxTecla;
          setPointS += teclaS;
          auxTecla *= 10; 
        }else if(keyS == '0'){
          setPointS *= 10;
          }
      }else if(keyS = 'P'){
        return setPointS;
      }
      tecla = 'n';     
    }
  }
void keypadEvent(KeypadEvent key){ 
    switch (keypad.getState()){        
    case PRESSED:
          //Serial.println("Pressed");
          tecla=key;
        break;
    case RELEASED:
            //Serial.println("Realeased");
        break;
    case HOLD:
            //Serial.println("Hold");
            if(key == 'M'){
            tecla=key;
            }
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
  EEPROM.put(180, userConfig);
  EEPROM.put(200, pwdConfig);
  EEPROM.commit();
  EEPROM.end();
  configuration = false;
  WiFi.disconnect();
}

