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
String topicSubvarStart = "CONTROLE/START"; 
//BOMBA OUVE
String topicSubSensorVazao = "CONTROLE/SENSOR_VAZAO";
//SENSOR OUVE
String topicSubSensorHigh = "CONTROLE/SENSOR_HIGH"; 
String topicSubSensorLow = "CONTROLE/SENSOR_LOW";
String topicSubSensorNivel = "CONTROLE/SENSOR_NIVEL";
String topicSubLitrosTanque = "CONTROLE/LITROS_TANQUE";

//BOMBA FALA
String topicPubBombaBomba = "BOMBA/BOMBA";
String topicPubBombaValvula = "BOMBA/VALVULA";
//IHM FALA
String topicPubIHMLitrosTanque = "IHM/LITROS_TANQUE";
String topicPubIHMTempoTotal = "IHM/TEMPO_TOTAL";                  
String topicPubIHMTempoRestante = "IHM/TEMPO_RESTANTE";
String topicPubIHMStatus = "IHM/STATUS";
String topicPubIHMNivel = "IHM/NIVEL";

WiFiClient espClient;
PubSubClient MQTTServer(espClient);
ESP8266WebServer server(80);

//OUVE
unsigned int setPoint = 0;                       //Preset de SetPoint 
bool varStart = false;
//OUVE DO SENSOR
bool sensorHigh = false;                //Preset false no sensor de nivel High
bool sensorLow = false;                 //Preset false no sensor de nivel Low
unsigned int nivel = 0.0;
unsigned int litrosTanque = 50;         //Preset de 50 litros capacidade total do tanque
//OUVE DA BOMBA
float vazao = 0.0;

//FALA PRA IHM
String tempoRestante = "00:00:00";      //Preset de 0 tempo;
String tempoTotal = "00:01:00";         //Preset de tempo total de 1hora
unsigned int nivelTanque = 50;          //Preset de 0% de nivel no tanque
int statusTanque = 50;                  //Preset Inicial
//int litrosTanque = 50;                  //Preset de 50 litros total do tanque
//FALA PRA BOMBA
bool bomba = false;
bool valvula = false;
//var controle
unsigned int nivelControle = 0;
int timerLoop;
void setup(){
      timerLoop = millis();
    Serial.begin(9600);
//    Wi_Fi();
//    MQTT();
//    MQTTServer.publish(topicPubBombaBomba.c_str(), String(0).c_str() );     // bomba desligada
//    MQTTServer.publish(topicPubBombaValvula.c_str(), String(0).c_str() );   // valvula fechada
//      
}

void loop(){
   
      Wi_Fi();
      MQTT();
      MQTTServer.loop(); 
      //nivelControle = map(nivel,0,100,0,litrosTanque);
      nivelControle = (litrosTanque*nivel)/100;
      if(varStart && nivelControle >= setPoint && sensorLow && setPoint != 0){
        bomba = true;
        valvula = true;
      }else{
        bomba = false;
        valvula = false;
      }
    
}                                           
//*******************FUNÇÃO Q RECONECTA AO BROKER ***************************************
//reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
// em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void MQTT(){
  if(MQTTServer.connected() == 1 ){                                       //CONECTADO
//    if(MQTTServer.subscribe(topicSubscrive.c_str(),0)){
//      Serial.println("SUBSCRIVE ok!!");
//    }else{
//      Serial.println("ERRO SUBSCRIVE");
//    }                                //STATUS NAO OK
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
    //Serial.println(topicoSubscrive);
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
  //Serial.println(" ");
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
  if(topico.equals(topicSubSetpoint)){                     //Mensagem do Modulo de Controle
      setPoint = atoi(msg.c_str());
      //Serial.print("Setpoint: ");
      //Serial.println(setPoint);
  }
   if(topico.equals(topicSubvarStart)){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        varStart = false;
      }else{
        varStart = true;
      }
     // Serial.print("varStart: ");
      //Serial.println(varStart);
  }
   if(topico.equals(topicSubSensorVazao)){                     //Mensagem do Modulo de Controle
      vazao = atoi(msg.c_str());
      //Serial.print("Vazao: ");
      //Serial.println(vazao);
  }
   if(topico.equals(topicSubSensorHigh)){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        sensorHigh = false;
      }else{
        sensorHigh = true;
      }
     // Serial.print("Sensor High: ");
      //Serial.println(sensorHigh);
  }
   if(topico.equals(topicSubSensorLow)){                     //Mensagem do Modulo de Controle
      if(msg[0]=='0'){
        sensorLow = false;
      }else{
        sensorLow = true;
      }
     //Serial.print("SensorLow: ");
     //Serial.println(sensorLow);
  }

  if(topico.equals(topicSubSensorNivel)){                     //Mensagem do Modulo de Controle
      nivel = atoi(msg.c_str());
      Serial.print("Nivel subs: ");
      Serial.println(nivel);      
  }
  if(topico.equals(topicSubLitrosTanque)){                     //Mensagem do Modulo de Controle
      litrosTanque = atoi(msg.c_str());
     // Serial.print("Litros Tanque: ");
     /// Serial.println(litrosTanque);      
  }
  MQTTServer.publish(topicPubBombaBomba.c_str(), String(bomba).c_str() );     // bomba desligada
  MQTTServer.publish(topicPubBombaValvula.c_str(), String(valvula).c_str() );   // valvula fechada
  MQTTServer.publish(topicPubIHMNivel.c_str(), String(nivel).c_str() ); 
  MQTTServer.publish(topicPubIHMLitrosTanque.c_str(), String(litrosTanque).c_str() );
  
}




