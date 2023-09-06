#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"

#define conexion_internet 13
int waitingTime = 90000; //Wait 30 seconds to google response.

//-------------------VARIABLES GLOBALES--------------------------
int contconexion = 0;
unsigned long previousMillis = 0;

char ssid[50];      
char pass[50];

const char *ssidConf = "esp32cam";
const char *passConf = "12345678";

const uint32_t TiempoEsperaWifi = 5000;

unsigned long TiempoActual = 0;
unsigned long TiempoAnterior = 0;
const long TiempoCancelacion = 500;

int banconection = 0; // Bandera que indicará si esta conectado
int bandera_envio = 0;

//-----------CODIGO HTML PARA PAGINAS WEB ---------------
String pagina_p1 = "<!DOCTYPE html>"
  "<html>"
    "<head>"
      "<meta charset='UTF-8'>"
      "<meta http-equiv='X-UA-Compatible' content='IE=edge'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>ESP32-CAM (Manometro)</title>"
    "</head>"
    "<style>"
      "html{"
        "width: 100%;"
        "height: 100vh;"
        "overflow:hidden;"
      "}"

      ".flex_column{"
        "display: flex;"
        "flex-direction: column;"
        "align-items: center;"
      "}"

      "body{"
        "width: 100%;"
        "height: 100%;"
        "padding: 0;"
        "margin: 0;"
        "background-color: rgb(233, 233, 233);"
      "}"

      "footer{"
        "background-color: black;"
        "width: 100%;"
        "height: fit-content;"
      "}"

      "h1, h2, h3, h4{"
        "font-family: 'Segoe UI', Tahoma, Verdana, sans-serif;"
        "margin: 0;"
        "margin-top: 5px;"
        "margin-bottom: 10px;"
        "text-align: center;"
      "}"

      "input, p{"
        "width: 90%;"
        "font-family: 'Segoe UI', Tahoma, Verdana, sans-serif;"
      "}"

      ".contenido{"
        "background-color: rgb(233, 233, 233);"
        "width: 95%;"
        "height: 100%;"
        "overflow: auto;"
      "}"

      ".apartado{"
        "background-color: rgb(245, 245, 245);"
        "box-shadow: 3px 3px 9px black;"
        "margin: 0 5px;"
        "margin-bottom: 15px;"
        "padding: 0 15px;"
        "width: fit-content;"
      "}"

      ".boton{"
        "font-weight: bolder;"
        "font-size: 1em;"
        "cursor: pointer;"
        "border: 1px solid;"
        "padding: 10px;"
        "background-color: rgb(223, 240, 252);"
      "}"

      ".boton:hover{"
        "box-shadow: 3px 3px 3px gray;"
      "}"

      "footer h4{"
        "color: white;"
      "}"

      ".fondo{"
          "display: none;"
          "position: fixed;"
          "top:0;"
          "margin: 0;"
          "z-index: 1;"
          "background-color: black;"
          "width: 100%;"
          "height: 100vh;"
      "}"

      ".fondo h2{"
        "color:white;"
        "margin: auto;"
      "}"
    "</style>"
    "<body class = 'flex_column'>"
      "<div class = 'contenido flex_column'><br><br>";

String paginafin = "</div><br>"
      "<div id = 'mensj' class = 'fondo flex_column'>"
        "<h2 style = 'text-align: center;'> Espere un momento... </h2><br>"
      "</div>"
      "<script>"
        "function mensj_a(){"
          "let mnsj = document.getElementById('mensj');"
          "mnsj.style.display = 'flex';"
        "};"
      "</script>"
    "<footer class = 'flex_column'><br><br>"
      "<h4>Website developed (2023)</h4><br><br>"
    "</footer>"
  "</body>"
"</html>";

// Para almacenar datos del cliente (Configuración)
String contenido;


//--------------------------------------------------------------
WebServer server(80);

//-------------------PAGINA DE CONFIGURACION--------------------
void paginaconf() {
  server.send(200, "text/html", pagina_p1 + contenido + paginafin); 
}

void paginainicio() {
  server.send(200, "text/html", pagina_p1 + contenido + paginafin); 
}

//--------------------MODO_CONFIGURACION------------------------
void modoconf() {
  delay(100);
  digitalWrite(conexion_internet, HIGH);
  delay(100);
  digitalWrite(conexion_internet, LOW);
  delay(100);
  digitalWrite(conexion_internet, HIGH);
  delay(100);
  digitalWrite(conexion_internet, LOW);

  contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
        "<form action='guardar_conf' method='get' class = 'apartado flex_column'>"
          "<h3> Configuración de Punto de Acceso: </h3>"

          "<h3 style= 'width: 90%; text-align: left;'> * Ingresar SSID: </h3>" 
          "<input class='input1' name='ssid' type='text' autocomplete='off'>"

          "<h3 style= 'width: 90%; text-align: left;'> * Ingresar PASSWORD: </h3>"  
          "<input class='input1' name='pass' type = 'password' autocomplete='off'>"

          "<input class='boton' type='submit' value = 'GUARDAR CONFIGURACIÓN'/><br>"
        "</form>"
      "</div><br>"

      "<a href='escanear'>"
        "<button class='boton'> ESCANEAR REDES DISPONIBLES </button>"
      "</a>";

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP();

  server.on("/", paginaconf);               // Página de configuración
  server.on("/guardar_conf", guardar_conf); // Graba en la eeprom la configuración
  server.on("/escanear", escanear);         // Escanean las redes WIFI disponibles
  

  if (!MDNS.begin("esp32cam")) {
    while(1){
      delay(1000);
    }
  }
  MDNS.addService("http", "tcp", 80);
  server.begin();

  while(true){
      server.handleClient();
  }
}

//--------------------- GUARDAR CONFIGURACION -------------------------
void guardar_conf() {
  //Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0,server.arg("ssid"));
  //Serial.println(server.arg("pass"));
  grabar(50,server.arg("pass"));

  contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
  "<h3> Configuración guardada... </h3><br>"
  "<h3> Reinicie el ESP32-CAM </h3>"
  "</div><br>";

  server.send(200, "text/html", pagina_p1 + contenido + paginafin); 
}

//---------------- Función para GRABAR en la EEPROM -------------------
void grabar(int addr, String a) {
  int tamano = a.length(); 
  char inchar[50]; 

  a.toCharArray(inchar, tamano+1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr+i, inchar[i]);
  }

  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr+i, 255);
  }

  EEPROM.commit();
}

//----------------- Función para LEER la EEPROM ------------------------
String leer(int addr) {
   byte lectura;
   String strlectura;
   for (int i = addr; i < addr+50; i++) {
      lectura = EEPROM.read(i);
      if (lectura != 255) {
        strlectura += (char)lectura;
      }
   }
   return strlectura;
}

//--------------------------- ESCANEAR REDES ----------------------------
void escanear() {  
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas

  if (n == 0) { //si no encuentra ninguna red
    contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
        "<form action='guardar_conf' method='get' class = 'apartado flex_column'>"
          "<h3> Configuración de Punto de Acceso: </h3>"
          "<h3 style= 'width: 90%; text-align: left;'> * Ingresar SSID: </h3>" 
          "<input class='input1' name='ssid' type='text' autocomplete='off'>"

          "<h3 style= 'width: 90%; text-align: left;'> * Ingresar PASSWORD: </h3>"  
          "<input class='input1' name='pass' type = 'password' autocomplete='off'><br>"
          
          "<input class='boton' type='submit' value = 'GUARDAR CONFIGURACIÓN'/><br>"
        "</form>"
      "</div><br>"

      "<div style= 'width: 100%'  class = 'flex_column' class = 'aartado flex_column'>"
          "<h3> No han sido encontradas redes disponibles </h3>"
          "<a href='escanear'>"
            "<button class='boton'> ESCANEAR REDES DISPONIBLES </button>"
          "</a>"
      "</div>";
  }else{
    contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
            "<form action='guardar_conf' method='get' class = 'apartado flex_column'>"
              "<h3> Configuración de Punto de Acceso: </h3>"
              "<h3 style= 'width: 90%; text-align: left;'> * Ingresar SSID: </h3>" 
              "<input class='input1' name='ssid' type='text' autocomplete='off'>"

              "<h3 style= 'width: 90%; text-align: left;'> * Ingresar PASSWORD: </h3>"  
              "<input class='input1' name='pass' type = 'password' autocomplete='off'><br>"
              
              "<input class='boton' type='submit' value = 'GUARDAR CONFIGURACIÓN'/><br>"
            "</form>"
          "</div><br>"

          "<div style= 'width: 100%'  class = 'flex_column' class = 'aartado flex_column'>"
              "<h3> Lista de redes encontradas: </h3>";


    for (int i = 0; i < n; ++i){
      // agrega al STRING "contenido" la información de las redes encontradas
      contenido = (contenido) + "<p><b>" + String(i + 1) + ") " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") </b><br><b>Canal:</b> " + WiFi.channel(i) + " <br><b>Tipo de cifrado:</b> " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }

    contenido = (contenido) + "<a href='escanear'>"
          "<button class='boton'> ESCANEAR REDES DISPONIBLES </button>"
        "</a>"
    "</div>";

    paginaconf();
  }
}


//------------------------SETUP WIFI-----------------------------
int setup_wifi() {
  // Conexión WIFI
  WiFi.mode(WIFI_STA); //para que no inicie el SoftAP en el modo normal
  //WiFi.begin("H@WI_521-B13R", "0Brg3@#._P1nXy?@");
  //WiFi.begin("INFINITUM7F36", "bXMBS345TR");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED and contconexion < 50) { //Cuenta hasta 50 si no se puede conectar lo cancela
    //Indicando que esta buscando si existe una conexión establecida entre ESP32-CAM y el WIFI
    ++contconexion;
    delay(250);
    Serial.print(".");

    digitalWrite(conexion_internet, HIGH);
    delay(250);
    digitalWrite(conexion_internet, LOW);
  }
  Serial.println();
  if (contconexion < 50) {
      Serial.println("Conexión OK");
      digitalWrite(conexion_internet, HIGH); // LED INDICANDO QUE HAY CONEXIÓN CON WIFI (ENCENDIDO)
      return 1;
  } else { 
      contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
          "<h3 style= 'width: 90%'> Error en la conexión </h3>"    
          "<h3 style= 'width: 90%'> Intente volver a configurar el ESP32-cam </h3>"  
      "</div><br>";

      digitalWrite(conexion_internet, LOW); // LED INDICANDO QUE NO HAY CONEXIÓN CON WIFI (APAGADO)
      return 0;
  }
}

// Envio de foto a drive


//------------------------SETUP-----------------------------
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Inicia Serial
  Serial.begin(115200);
  EEPROM.begin(512);

  pinMode(conexion_internet, OUTPUT); // LED INDICADOR DE CONEXIÓN CON WIFI
  pinMode(14, INPUT);                 // BOTÓN DE CONFIGURACIÓN
  
  // Si desea configurar el SSID Y PASS de Wifi - Debe dejar oprimido el botón al energizar el ESP32-CAM
  if(digitalRead(14) == 0) { 
    modoconf();
  }

  // Al energizar al ESP32-CAM siempre realiza la lectura de la EEPROM
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);
 
  delay(1000);

  // Estableciendo conexión con el wifi
  banconection = setup_wifi();
}

//--------------------------LOOP--------------------------------
void loop(){
  Serial.println(ssid);
  Serial.println(pass);

  if(banconection == 1){
    contenido = "<div style= 'width: 100%'  class = 'flex_column'>"
        "<h3> Conexión establecida: </h3>"
        "<h4 style= 'width: 90%; text-align: left;'> * Conectado a SSID: " + String(ssid) + " </h4><br><br>" 
        "<div style= 'width: 100%'  class = 'flex_column'>"
        "<input type='button' onclick='location.href='https://www.youtube.com/';' value='Ir a YouTube' />"
      "</div><br>";
    "</div><br>";

    server.on("/", paginainicio); //esta es la pagina de inicio
    server.begin();

    if (!MDNS.begin("esp32cam")) {
      while (1) {
        delay(1000);
      }
      Serial.print("Error en MDNS");
    }
    MDNS.addService("http", "tcp", 80);

    while(true){
      server.handleClient();
    }
  }else{
    while(true){
      //Indicando que esta buscando si existe una conexión establecida entre ESP32-CAM y el WIFI
      Serial.print("x");
      digitalWrite(conexion_internet, HIGH);
      delay(250);
      digitalWrite(conexion_internet, LOW);
      delay(250);
    }
  }
}