//**************************IHM**********************************************************
//AUTOR: Johnatam Renan Horst
//CURSO: Engenharia de Controle e Automação
//DATA: 06/09/2018
//PROJETO: 
//DESCRISAO: Modulo CONTROLE do Projeto
//
//
//
//
//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>      
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Ticker.h>
String ssid_AP     = "wifi_Johnatan";
String password_AP = "99434266";
//MQTT 
String brokerUrl   = "192.168.0.190";        //URL do broker MQTT 
String brokerPort  = "1883";                 //Porta do Broker MQTT
String brokerID    = "CONTROLE";
String brokerUser  =  "";                                   //Usuário Broker MQTT
String brokerPWD   = "";                                    //Senha Broker MQTT                 
String topicSubscrive = "CONTROLE/#";                            //Topico de cubscrive do generico
//IHM OUVE
String topicSubSetpoint = "CONTROLE/SETPOINT";      //lembra se for no nodeMCU colocar p $ no final      
String topicSubvarStart = "CONTROLE/varStart"; 
//BOMBA OUVE
String topicSubSensorVazao = "CONTROLE/SENSOR_VAZAO";
//SENSOR OUVE
String topicSubSensorHigh = "CONTROLE/SENSOR_HIGH"; 
String topicSubSensorLow = "CONTROLE/SENSOR_LOW";
String topicSubSensorNivel = "CONTROLE/SENSOR_NIVEL";

//BOMBA FALA
String topicPubBombaBomba = "BOMBA/BOMBA";
String topicPubBombaValvula = "BOMBA/VALVULA";
//IHM FALA
String topicPubIHMTempoTotal = "IHM/TEMPO_TOTAL";                  
String topicPubIHMTempoRestante = "IHM/TEMPO_RESTANTE";
String topicPubIHMStatus = "IHM/STATUS";
String topicPubIHMNivel = "IHM/NIVEL";

WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);

//OUVE
bool setPoint = false;                       //Preset de SetPoint 
bool varStart = false;
//OUVE DO SENSOR
bool sensorHigh = false;                //Preset false no sensor de nivel High
bool sensorLow = false;                 //Preset false no sensor de nivel Low
unsigned int nivel = 0.0;
//OUVE DA BOMBA
float vazao = 0.0;

//FALA PRA IHM
String tempoRestante = "00:00:00";      //Preset de 0 tempo;
String tempoTotal = "00:01:00";         //Preset de tempo total de 1hora
unsigned int nivelTanque = 50;                   //Preset de 0% de nivel no tanque
int statusTanque = 50;
//FALA PRA BOMBA
//int nivelTanque = 50;                   //Preset de 0% de nivel no tanque
//int statusTanque = 50;

int litrosTanque = 50;                  //Preset de 50 litros total do tanque


//funcao setup
void setup(){
    Serial.begin(9600);
    
}

void loop(){
   
      Wi_Fi();
      MQTT();
      MQTTServer.loop(); 
      //MQTTServer.publish(topicSubscriveSensor.c_str(), String(key).c_str() );    
    
}                                           
//*******************FUNÇÃO Q RECONECTA AO BROKER ***************************************
//reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
// em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void MQTT(){
  if(MQTTServer.connected() == 1 ){                                       //CONECTADO                             //STATUS NAO OK
    if(MQTTServer.state()==0){
    //Serial.println("!");
    }else{
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
  Serial.println("Resceiver msg!!");
  String topico;
  String msg;
  String aux;
  int valor;
  int i=0;

  topico = String(topic);
  for(int i = 0; i < length; i++){                  //Transforma a mensagem recebida em String Para manipulação
     resposta[i] = (char)payload[i];
     msg += resposta[i];
     }
  if(topico == topicSubSetpoint){                     //Mensagem do Modulo de Controle
      setPoint = atoi(msg.c_str());
      Serial.print("Setpoint: ");
      Serial.println(setPoint);
  }
   if(topico == topicSubvarStart){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        varStart = false;
      }else{
       // varStart = true;
      }
      Serial.print("varStart: ");
      Serial.println(varStart);
  }
   if(topico == topicSubSensorVazao){                     //Mensagem do Modulo de Controle
      vazao = atoi(msg.c_str());
      Serial.print("Vazao: ");
      Serial.println(vazao);
  }
   if(topico.equals(topicSubSensorHigh)){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        sensorHigh = false;
      }else{
        sensorHigh = true;
      }
      Serial.print("Sensor High: ");
      Serial.println(sensorHigh);
  }
   if(topico == topicSubSensorLow){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        sensorLow = false;
      }else{
        sensorLow = true;
      }
      Serial.print("SensorLow: ");
      Serial.println(sensorLow);
  }
  if(topico == topicSubSensorNivel){                     //Mensagem do Modulo de Controle
      nivel = atoi(msg.c_str());
      Serial.print("Nivel: ");
      Serial.println(nivel);      
  }
}




