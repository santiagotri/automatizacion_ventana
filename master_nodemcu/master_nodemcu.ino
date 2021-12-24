#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char *NOMBRE_RED = "NOMBRE_RED", //Ingresa aca el nombre de tu red wifi
            *CLAVE_RED = "CONTRASENA"; // Ingresa acá la contrasena de tu red wifi

const int PUERTO = 80,
          LED_D1 = 2;


// Tener un estado del LED
bool estadoDelLED = false;
int lecturasSensorLluvia = 0;
int sensor_lluvia = 0;
bool autocerrado_lluvia = true;
int estado_ventana = 0;
int estado_persianas = false;
bool esclavoOcupado = false;

//ESTOS PINS SE PUEDEN CAMBIAR DE ACUERDO A LOS CANALES DIGITALES USADOS
const int pinA = 16; //d0
const int pinB = 5; //d1
const int pinC = 4;//d2
const int pinD = 0;//d3
const int pinE = 14;//d5

const int pinF = 12;//d6
const int pinG = 13;//d7


ESP8266WebServer servidor(PUERTO);

//Esta es la interfaz en html, si se desea hacer un cambio es posible
String obtenerInterfaz()
{
  String rta = "<!DOCTYPE html>"
               ""
               "<html>"
               "<head>"
               ""
               "    <script type='text/javascript'>\n"

               "    async function enviarPost(parametro) {\n"
               "        var url = document.URL + parametro;\n"
               "        var xhr = new XMLHttpRequest();\n"
               "        xhr.open(\"POST\", url, true);\n"
               "        xhr.send(null);\n"
               "        await new Promise(r => setTimeout(r, 200));\n"
               "        location.reload();\n"
               "    }\n"
               "    \n"
               "    function cambioEstadoVentana () {\n"
               "            var button_text = document.getElementById('customRange').value;\n"
               "            if (button_text=='0') {\n"
               "    //window.location.href = '/cerrar_ventana';//code\n"
               "    enviarPost('cerrar_ventana');\n"
               "            }else if (button_text=='1') {\n"
               "    //window.location.href = '/abrir_ventana_25';//code\n"
               "    enviarPost('abrir_ventana_25');\n"
               "            }else if (button_text=='2') {\n"
               "    //window.location.href = '/abrir_ventana_50';//code\n"
               "    enviarPost('abrir_ventana_50');\n"
               "            }else if (button_text=='3') {\n"
               "    //window.location.href = '/abrir_ventana_75';//code\n"
               "    enviarPost('abrir_ventana_75');\n"
               "            }if (button_text=='4') {\n"
               "    //window.location.href = '/abrir_ventana_100';//code\n"
               "    enviarPost('abrir_ventana_100');\n"
               "            }\n"
               "            console.log(button_text);\n"
               "    }\n"
               "    function cambioEstadoPersianas() {\n"
               "        var button_estado = document.getElementById('customSwitch1').checked;\n"
               "        if (button_estado) {\n"
               "            //window.location.href = '/abrir_persianas';//code  \n"

               "            enviarPost('abrir_persianas');  \n"

               "        }else if (!button_estado) {\n"
               "            //window.location.href = '/cerrar_persianas';  \n"
               "            enviarPost('cerrar_persianas');  \n"
               "        }\n"
               "        console.log('Estado persianas: '+button_estado);\n"
               "    }\n"
               "\n"
               "    function cambioEstadoAutocerrado(args) {\n"
               "        var button_estado = document.getElementById('customSwitch2').checked;\n"
               "        if (button_estado) {\n"
               "            //window.location.href = '/activar_autocerrado_lluvia';//code  \n"
               "            enviarPost('activar_autocerrado_lluvia');  \n"
               "        }else if (!button_estado) {\n"
               "            //window.location.href = '/desactivar_autocerrado_lluvia';//code  \n"
               "            enviarPost('desactivar_autocerrado_lluvia');  \n"
               "        }\n"
               "        console.log('Estado autocerrado: '+button_estado);\n"
               "    }\n"
               "\n"
               "\n    </script>"
               "    <meta charset='UTF-8'>"
               "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
               "    <link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css' integrity='sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T' crossorigin='anonymous' type='text/css'>"
               ""
               "    <title>Ventana Cuarto Santi</title>"
               "</head>"
               ""
               "<body>"
               "    <br>"
               ""
               "    <h1 class='display-1' align='center'>Ventana Cuarto</h1><br>";

  if (bloqueo_sensor()) {
    rta += "<div align='center' class='p-3 mb-2 bg-warning text-dark'>";
    rta +=      "        La ventana ha sido cerrada por detección de lluvia";
    rta +=     "    </div>";
  } else if (sensor_lluvia != 0) {
    rta += " <div align='center' class='p-3 mb-2 bg-danger text-white'>";
    rta +=     "        Se detecta lluvia y el autocerrado está desactivado.";
    rta +=     "    </div>";
  }

  if (digitalRead(pinG) == LOW && digitalRead(pinF) == LOW) {
    rta += " <div align='center' class='p-3 mb-2 bg-danger text-white'>";
    rta +=     "        Existió un error de comunicación con el esclavo. (ERROR_03)";
    rta +=     "    </div>";
  }
  else if (esclavoOcupado) {
    rta += "<div align='center' class='p-3 mb-2 bg-warning text-dark'>";
    rta +=      "        No se ha podido ejecutar la acción debido a que hay una petición en proceso.";
    rta +=     "    </div>";
    esclavoOcupado = false;
  }

  rta += "    <br>";
  rta +=     "    <div class='row'>";
  rta +=     "        <div align='center' class='col'>";
  rta +=     "            <h4>Hora actual</h4>";
  rta +=    "            <div style='text-align:center;padding:1em 0;'>";
  rta +=     "                <iframe src='https://www.zeitverschiebung.net/clock-widget-iframe-v2?language=es&amp;size=small&amp;timezone=America%2FBogota' width='100%' height='90' frameborder='0' seamless=''></iframe>";
  rta +=     "            </div><br>";
  rta +=     "            <br>";
  if (sensor_lluvia == 0)
  {
    rta += "<img src='https://image.flaticon.com/icons/png/512/1828/1828640.png' width='50' height='50' class='img-fluid' alt='Responsive image'><br>";
    rta +=      "            <br>";
    rta +=      "";
    rta +=     "            <p class='lead'>No se detecta lluvia<br>";
    rta +=     "            mediante el sensor</p><br>";
  } else if (sensor_lluvia == 1) {
    rta += "<img src='https://image.flaticon.com/icons/png/512/595/595067.png' width='50' height='50' class='img-fluid' alt='Responsive image'><br>";
    rta +=     "            <br>";
    rta +=     "";
    rta +=     "            <p class='lead'>Se detecta llovizna<br>";
    rta +=     "            mediante el sensor</p><br>";
  } else {
    rta += "<img src='https://image.flaticon.com/icons/png/512/595/595067.png' width='50' height='50' class='img-fluid' alt='Responsive image'><br>";
    rta +=      "            <br>";
    rta +=     "";
    rta +=     "            <p class='lead'>Se detecta <a class='text-danger'>fuerte lluvia</a><br>";
    rta +=     "            mediante el sensor</p><br>";
  }
  
  rta += "<br> <p> <small class='text-muted'>(Lectura sensor ";
  rta += lecturasSensorLluvia;
  rta += "/1024)</small></p>";

  rta += " </div>";
  rta +=    "        <div align='center' class='col-5' style='min-width: 400px;'>";
  rta +=    "            <h4>Ventana</h4>";
  rta +=    "<p class='lead'>La ventana se encuentra ";

  rta += darEstado_ventana();

  rta += "</p><br>";
  rta +=      "";
  rta +=      "            <div class='row' style='max-width: 390px;'>";
  rta +=     "                <div align='left' class='col'>";
  rta +=      "                    Cerrada";
  rta +=      "                </div>";
  rta +=      "";
  rta +=      "                <div align='left' class='col'>";
  rta +=      "                    25%";
  rta +=      "                </div>";
  rta +=     "";
  rta +=     "                <div align='center' class='col'>";
  rta +=     "                    50%";
  rta +=     "                </div>";
  rta +=     "";
  rta +=     "                <div align='right' class='col'>";
  rta +=     "                    75%";
  rta +=    "                </div>";
  rta +=    "";
  rta +=     "                <div align='right' class='col'>";
  rta +=     "                    Abierta";
  rta +=     "                </div>";
  rta +=      "            </div><input type='range' style='max-width: 350px;' class='custom-range' value='";

  rta += estado_ventana;

  rta += "' min='0' max='4' id='customRange'";
  if (bloqueo_sensor()) rta += " disabled";
  rta += "><br>";
  rta +=     "            <br>";
  rta +=    "            <button class='btn btn-primary' type='submit' onclick='cambioEstadoVentana()'";
  if (bloqueo_sensor()) rta += " disabled";
  rta += ">Actualizar</button><br>";
  rta +=     "            <br>";
  rta +=      "            <br>";
  rta +=     "";
  rta +=     "            <h4>Persianas</h4>";
  rta +=     "";
  rta +=     "            <p class='lead'>Las persianas se encuentran ";

  rta += darEstado_persianas();

  rta += "</p><br>";
  rta +=    "";
  rta +=     "            <div class='custom-control custom-switch'>";
  rta +=     "                <input type='checkbox' class='custom-control-input' id='customSwitch1' onclick='cambioEstadoPersianas()'";

  if (estado_persianas) rta += " checked";

  rta += "> <label class='custom-control-label' for='customSwitch1'>";

  rta += ((estado_persianas) ? "Abiertas" : "Cerradas");

  rta += "</label>";
  rta += "            </div><br><br>";
  rta += "        </div><br>";
  rta += "<div align='center' class='col'>";
  rta +=       "            <h4>Configuración</h4><br>";
  rta +=       "";
  rta +=       "            <p class='lead'><strong>SSID:</strong> ";

  rta += NOMBRE_RED;

  rta += "</p><br>";
  rta +=      "";
  rta +=       "            <p class='lead'><strong>IP:</strong> ";

  rta += "192.168.0.149";

  rta += "</p><br>";
  rta +=      "";
  rta +=      "            <p class='lead'><strong>Estado red:</strong> ";

  rta += "Conectada";

  rta += "</p><br>";
  rta +=      "";
  rta +=      "            <p class='lead'><strong>MAC:</strong> ";

  rta += "98:CD:AC:31:F8:33";

  rta += "</p><br>";
  rta +=      "";
  rta +=      "            <div class='custom-control custom-switch'>";
  rta +=      "                <input type='checkbox' class='custom-control-input' id='customSwitch2' onclick='cambioEstadoAutocerrado()'";

  if (autocerrado_lluvia) rta += " checked";

  rta += "> <label class='custom-control-label' for='customSwitch2'>Autocerrado al detectar lluvia</label>";

  if (!autocerrado_lluvia) rta += "<br><small class='text-muted'>Precaución: para evitar daños se recomienda dejar encendido</small>";

  rta +=      "            </div>";
  rta +=      "        </div>";
  rta +=      "    </div><br>";
  rta +=      "    <br>";
  rta +=      "";
  rta +=      "    <p style='text-align:center'><small class='text-muted'>Creado y diseñado por</small> <a href='https://instagram.com/santiagotri'>@Santiagotri</a></p>";
  rta +=      "</body>";
  rta +=      "</html>";
  rta +=     "";

  return rta;
}

//RUTAS

void rutaRaiz()
{
  mostrarInterfazHTML();
}

void mostrarInterfazHTML()
{
  servidor.send(200, "text/html", obtenerInterfaz());
}

void responderEsclavoOcupado()
{
  servidor.send(405, "text/html", "esclavo_ocupado");
}

void responderVentanaBloqueadaPorLluvia()
{
  servidor.send(405, "text/html", "ventana bloqueada por lluvia");
}

void ruta_cerrar_ventana()
{
  ruta_cerrar_ventana_sin_interfaz();
  //mostrarInterfazHTML();
  if (esclavoOcupado) {
    responderEsclavoOcupado();
  } else {
    ruta_estado_ventana();
  }
}

void ruta_cerrar_ventana_sin_interfaz()
{
  if (enviar_secuencia_esclavo(false, true, true, false)) estado_ventana = 0;
  else esclavoOcupado = true;
}

void ruta_cerrar_ventana_extraordinario() {
  if (enviar_secuencia_esclavo(false, true, true, true)) estado_ventana = 0;
  else esclavoOcupado = true;
}

boolean enviar_secuencia_esclavo(boolean A, boolean B, boolean C, boolean D) {
  if (esclavo_escuchando()) {

    if (A) digitalWrite(pinA, HIGH);
    else digitalWrite(pinA, LOW);

    if (B) digitalWrite(pinB, HIGH);
    else digitalWrite(pinB, LOW);

    if (C) digitalWrite(pinC, HIGH);
    else digitalWrite(pinC, LOW);

    if (D) digitalWrite(pinD, HIGH);
    else digitalWrite(pinD, LOW);

    digitalWrite (pinE, HIGH);


    while (digitalRead(pinG) == LOW && digitalRead(pinF) == HIGH) {
      digitalWrite(LED_D1, LOW);
      delay(10);
      digitalWrite(LED_D1, HIGH);
      delay(10);
    }

    digitalWrite (pinE, LOW);

    return true;
  }
  return false;
}

boolean esclavo_escuchando () {
  if (digitalRead(pinF) == HIGH && digitalRead(pinG) == LOW) return true;
  return false;
}

void ruta_abrir_ventana_25()
{
  if (bloqueo_sensor()) {
    ruta_cerrar_ventana_sin_interfaz();
    responderVentanaBloqueadaPorLluvia();
  } else {
    if (enviar_secuencia_esclavo(true, true, false, false) || estado_ventana == 1) {
      estado_ventana = 1;
      ruta_estado_ventana();
    }
    else {
      esclavoOcupado = true;
      responderEsclavoOcupado();
    }
    //mostrarInterfazHTML();
  }
}

void ruta_abrir_ventana_50()
{
  if (bloqueo_sensor()) {
    ruta_cerrar_ventana_sin_interfaz();
    responderVentanaBloqueadaPorLluvia();
  } else {
    if (enviar_secuencia_esclavo(true, true, false, true) || estado_ventana == 2) {
      estado_ventana = 2;
      ruta_estado_ventana();
    }
    else {
      esclavoOcupado = true;
      responderEsclavoOcupado();
    }
    //mostrarInterfazHTML();
  }
}
void ruta_abrir_ventana_75()
{
  if (bloqueo_sensor()) {
    ruta_cerrar_ventana_sin_interfaz();
    responderVentanaBloqueadaPorLluvia();
  } else {
    if (enviar_secuencia_esclavo(true, true, true, false) || estado_ventana == 3) {
      estado_ventana = 3;
      ruta_estado_ventana();
    }
    else {
      esclavoOcupado = true;
      responderEsclavoOcupado();
    }
    //mostrarInterfazHTML();
  }
}
void ruta_abrir_ventana_100()
{
  if (bloqueo_sensor()) {
    ruta_cerrar_ventana_sin_interfaz();
    responderVentanaBloqueadaPorLluvia();
  } else {
    if (enviar_secuencia_esclavo(true, true, true, true) || estado_ventana == 4) {
      estado_ventana = 4;
      ruta_estado_ventana();
    }
    else {
      esclavoOcupado = true;
      responderEsclavoOcupado();
    }
    //mostrarInterfazHTML();
  }
}

void ruta_abrir_persianas()
{
  if (enviar_secuencia_esclavo(true, false, true, true)) {
    estado_persianas = true;
    ruta_estado_persianas();
  }
  else {
    esclavoOcupado = true;
    responderEsclavoOcupado();
  }
  //mostrarInterfazHTML();
}

void ruta_cerrar_persianas()
{
  if (enviar_secuencia_esclavo(false, false, false, true)) {
    estado_persianas = false;
    ruta_estado_persianas();
  }
  else {
    esclavoOcupado = true;
    responderEsclavoOcupado();
  }
  //mostrarInterfazHTML();
}

void ruta_activar_autocerrado_lluvia()
{
  if (sensor_lluvia != 0) ruta_cerrar_ventana_sin_interfaz();
  autocerrado_lluvia = true;
  //mostrarInterfazHTML();
  ruta_estado_autocerrado_lluvia();
}

void ruta_desactivar_autocerrado_lluvia()
{
  autocerrado_lluvia = false;
  //mostrarInterfazHTML();
  ruta_estado_autocerrado_lluvia();
}

void ruta_estado_ventana() {
  servidor.send(200, "text/plain", darEstado_ventana());
}

void ruta_estado_persianas() {
  servidor.send(200, "text/plain", darEstado_persianas());
}

void ruta_estado_autocerrado_lluvia() {
  String rta = (autocerrado_lluvia) ? "Activado" : "Desactivado";
  servidor.send(200, "text/plain", rta);
}

void ruta_estado_lluvia() {
  String rta = "ERROR_02";
  if (sensor_lluvia == 0) rta = "No se detecta lluvia mediante el sensor";
  else if (sensor_lluvia == 1) rta = "Se detecta llovizna mediante el sensor";
  else if (sensor_lluvia == 2) rta = "Se detecta lluvia fuerte mediante el sensor";
  servidor.send(200, "text/plain", rta);
}

void rutaNoEncontrada()
{
  servidor.send(404, "text/plain", "Not found 404");
}



//METODOS DE CONSULTA

bool bloqueo_sensor() {
  return ((sensor_lluvia != 0 && autocerrado_lluvia) ? true : false);
}


String darEstado_ventana() {
  if (estado_ventana == 0 && bloqueo_sensor()) return "cerrada por detección de lluvia";
  else if(estado_ventana==0) return "cerrada";
  else if (estado_ventana == 1) return "abierta al 25%";
  else if (estado_ventana == 2) return "abierta al 50%";
  else if (estado_ventana == 3) return "abierta al 75%";
  else if (estado_ventana == 4) return "completamente abierta";
  else {
    return "ERROR_01";
  }
}

String darEstado_persianas() {
  return ((estado_persianas) ? "abiertas" : "cerradas");
}

//SETUP Y LOOP

void setup()
{

  Serial.begin(9600);
  // Configuración del LED
  pinMode(LED_D1, OUTPUT);

  pinMode(pinA, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);
  pinMode(pinD, OUTPUT);
  pinMode(pinE, OUTPUT);

  pinMode(pinF, INPUT);
  pinMode(pinG, INPUT);

  // Set your Static IP address
  IPAddress local_IP(192, 168, 0, 149);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);


  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }else{
    Serial.println("STA Configurado!");
  }

  //Conectarse a WIFI
  //WiFi.mode(WIFI_STA);
  WiFi.begin(NOMBRE_RED, CLAVE_RED);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_D1, HIGH);
    delay(500); //aqui debemos esperar unos instantes
    digitalWrite(LED_D1, LOW);
    delay(500);
    Serial.println("Intentando conectar con wifi...");
  }

  Serial.println("Conexión exitosa a WiFi");
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());


  // Configuramos la ruta y la función que responderá a la solicitud de dicha ruta
  servidor.on("/", rutaRaiz);
  servidor.on("/cerrar_ventana", ruta_cerrar_ventana);
  servidor.on("/abrir_ventana_25", ruta_abrir_ventana_25);
  servidor.on("/abrir_ventana_50", ruta_abrir_ventana_50);
  servidor.on("/abrir_ventana_75", ruta_abrir_ventana_75);
  servidor.on("/abrir_ventana_100", ruta_abrir_ventana_100);
  servidor.on("/abrir_persianas", ruta_abrir_persianas);
  servidor.on("/cerrar_persianas", ruta_cerrar_persianas);
  servidor.on("/activar_autocerrado_lluvia", ruta_activar_autocerrado_lluvia);
  servidor.on("/desactivar_autocerrado_lluvia", ruta_desactivar_autocerrado_lluvia);
  servidor.on("/estado_ventana", ruta_estado_ventana);
  servidor.on("/estado_persianas", ruta_estado_persianas);
  servidor.on("/estado_autocerrado_lluvia", ruta_estado_autocerrado_lluvia);
  servidor.on("/estado_lluvia", ruta_estado_lluvia);

  servidor.onNotFound(rutaNoEncontrada);
  // Empezar a escuchar
  servidor.begin();
}

void loop()
{
  for (int i = 0; i < 100000; i++) servidor.handleClient();
  lecturasSensorLluvia = analogRead(0);
  if (lecturasSensorLluvia >= 1020) sensor_lluvia = 0;
  else if (lecturasSensorLluvia >= 1000) sensor_lluvia = 1;
  else if (lecturasSensorLluvia >= 50) sensor_lluvia = 2;
  else {
    sensor_lluvia = 3;
  }
  if (bloqueo_sensor() && estado_ventana != 0) ruta_cerrar_ventana_sin_interfaz();
  Serial.println(lecturasSensorLluvia);
}
