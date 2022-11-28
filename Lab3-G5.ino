#include <Stepper.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <ArduinoJson.h>

Stepper motor1(2048, 8, 10, 9, 11); // PASOS PARA UNA VUELTA COMPLETO
const int PIN_LUZ = 0;
int velocidad = 4; // En RPM (Valores de  1, 2 o 3  para el modelo 28BYJ-48) y 4 para cuando es de noche 
int potencia = 0;
int luz = 0;
float voltage;
float luxes;
float resistance;

const float LUX_CALC_SCALAR=12518931;
const float LUX_CALC_EXPONENT=-1.405;

int zona = 1; // Zona donde se está regando actualmente, tomará valores del 1 al 3
int apagado = 0; // Bandera para controlar el estado apagado/encendido
int giro = 512;
int vueltas = 1;
unsigned long tiempo = 0;
long tiempoUltimoCambio = 0;

const byte I2C_SLAVE_ADDR = 0x7F;
time_t fecha;
String mensaje;
//DynamicJsonDocument doc(1024);


void setup() 
{
  Serial.begin(9600);
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

const char ASK_FOR_LENGTH = 'L';
const char ASK_FOR_DATA = 'D';

char request = ' ';
char requestIndex = 0;

/*void SerializeObject(JsonArray sensors, JsonArray actuators)
{
  doc["controller_name"] = "Arduino-nano-5";
  fecha = now();  
  doc["date"] = fecha;
  doc["actuators"] = actuators;
  doc["sensors"] = sensors;
  serializeJson(doc, json);
}*/

void receiveEvent(int bytes)
{
  while (Wire.available())
  {
   request = (char)Wire.read();
  }
}
void requestEvent()
{
  if(request == ASK_FOR_LENGTH)
  {
    Wire.write(mensaje.length());
    char requestIndex = 0;
  }
  if(request == ASK_FOR_DATA)
  {
    if(requestIndex < (mensaje.length() / 32)) 
    {
      Wire.write(mensaje.c_str() + requestIndex * 32, 32);
      requestIndex ++;
    }
    else
    {
      Wire.write(mensaje.c_str() + requestIndex * 32, (mensaje.length() % 32));
      requestIndex = 0;
    }
  }
}


void loop()
{
  tiempo = millis();
  tiempoUltimoCambio = tiempo;
  regar(20000); // Riega zona 1 durante 20segs.
  zona++;
  vueltas=1;  
  delay(5000);  
  
  tiempoUltimoCambio = tiempo;
  regar(25000); // Riega zona 2 durante 25segs.
  zona++;
  vueltas=1;
  delay(5000);

  tiempoUltimoCambio = tiempo;
  regar(30000); // Riega zona 3 durante 30segs.
  
  apagarMotor();
  delay(20000); // Una vez completado el ciclo de zonas (cerca del mediodia), el motor volverá a regar 20 segs después
  zona = 1;
  vueltas=1;
  
  tiempoUltimoCambio = tiempo;
  regar(20000); // Riega zona 1 durante 20segs.
  zona++;
  vueltas=1;
  delay(5000);  
  
  tiempoUltimoCambio = tiempo;
  regar(25000); // Riega zona 2 durante 25segs.
  zona++;
  vueltas=1;
  delay(5000);
  
  tiempoUltimoCambio = tiempo;
  regar(30000); // Riega zona 3 durante 30segs.
  apagarMotor();
  zona = 1; 
  vueltas=1;
  delay(20000);  
}

void obtenerDatos()
{
  luz=random(0,1023);
  //luz = analogRead(PIN_LUZ);
	velocidad = map(luz, 0, 1023, 1, 4);
  voltage = luz * (5.0/1023) * 1000;
  resistance = 10000 * ( voltage / ( 5000.0 - voltage) );
  luxes = LUX_CALC_SCALAR * pow(resistance, LUX_CALC_EXPONENT);
}

void regar(int duracion) 
{
  while ( tiempo - tiempoUltimoCambio < duracion) // Controla que el motor gire durante el tiempo pasado por parametro 
  {                                                 // (al ser una duracion pequeña para la demo, puede pasarse unos 7 segs.)
    obtenerDatos();
    controlarMotor(); 
    tiempo=millis();
    delay(1000);
  };  
}

// La velocidad va a ser 4 cuando la fotoresistencia no detecte luz, es decir por la noche. 
// Va a ser 1 cuando la intensidad de la luz sea maxima, es decir en horario cercano al mediodia en condiciones ideales. 
// El motor debe permanecer apagado cuando la velocidad sea 4. 
void controlarMotor() 
{    
  if (velocidad < 4) { 
    /*motor1.setSpeed(velocidad); // El motor girará a la velocidad obtenida de mapear los datos recibidos por la fotoresistencia 
    Serial.print("regando zona: ");  
    Serial.println(zona);   
    Serial.print("velocidad de riego: ");
    Serial.print(velocidad);
    Serial.println(" rpms");
    Serial.print("cantidad de vueltas: ");
    Serial.println(vueltas);
    Serial.print("lectura de luz: ");    
    Serial.print(luxes);
    Serial.println(" lx");
    Serial.println("");
    motor1.step(giro); // El motor realizará la cantidad de pasos pasada por parametros (512, 1/4 de vuelta)
    */
    serializeObject(luxes, velocidad);

    apagado = 0;
    vueltas++;     
  }
	else if ( velocidad == 4 ) {
    if (apagado = 0) { // Si el motor está encendido se apagará
      apagarMotor();
    }; 
    tiempo = tiempo - 1000;
    Serial.println("Motor apagado");  
    Serial.print("lectura de luz: ");    
    Serial.println(luz); 
    Serial.println("");
  } 
}

void apagarMotor() 
{    
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  apagado = 1;
}

void serializeObject(int luz, int velocidad)
{
  //char json[300];
  String json; 
  StaticJsonDocument<192> doc; 
    
  doc["controller_name"] = "Arduino-nano-5";
  doc["date"] = "2022-27-11 20:28:00";
    
  // Armando el array actuators
  JsonArray arrActuator = doc.createNestedArray("actuators");
  StaticJsonDocument<52> act;
  JsonObject actuator = act.to<JsonObject>();
  actuator["type"]="stepper";
  actuator["current_value"] = velocidad;
  arrActuator.add(actuator);
    
  // Armando el array sensors
  JsonArray arrSensor = doc.createNestedArray("sensors");
  StaticJsonDocument<52> sen;
  JsonObject sensor = sen.to<JsonObject>();
  sensor["type"]="fotorresistor";
  sensor["current_value"] = luxes;
  arrSensor.add(sensor);  
    
  serializeJson(doc, json);
  Serial.print("Json: ");
  Serial.println(json); 
  mensaje = json;  
}
