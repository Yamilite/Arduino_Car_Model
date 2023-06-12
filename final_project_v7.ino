#include <WiFi.h>
#include <WebServer.h>
/* Установите здесь свои SSID и пароль */
const char* ssid = "Car";  
const char* password = "01234567";  
/* Настройки IP адреса */
IPAddress local_ip(192,168,2,1);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);
WebServer server(80);
bool Autopilot = LOW;
bool Mechanik = LOW;

#include <Ultrasonic.h>
#include <ESP32Servo.h>

#define IN1_PIN 18
#define IN2_PIN 19
#define IN3_PIN 26
#define IN4_PIN 27

#define trig_pin 33
#define echo_pin 32

Servo myservo;

Ultrasonic ultrasonic(33, 32);

void forward_motors() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
}

void backward_motors() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
}

void right_motors() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}

void left_motors() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
}

void stop_motors() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}

void setup() {
  Serial.begin(9600);

  myservo.attach(13);

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  server.on("/forward", handle_forward);
  server.on("/backward", handle_backward);
  server.on("/right", handle_right);
  server.on("/left", handle_left);
  server.on("/stop", handle_stop);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handle_led1off() {
  Autopilot = LOW; // установка флага автопилота в выключенное состояние
  stop_motors(); // остановка двигателей
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik)); // отправка ответа клиенту
}

void handle_OnConnect() {
  Autopilot = LOW;
  Mechanik = LOW;
  stop_motors(); // остановка двигателей при подключении к серверу
  Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik)); 
}

void handle_led1on() {
  Autopilot = HIGH; // установка флага автопилота во включенное состояние
  Serial.println("GPIO4 Status: ON");

  // создание нового потока выполнения для работы автопилота
  xTaskCreatePinnedToCore(
    run_autopilot, // функция, которую нужно выполнить в новом потоке
    "Autopilot Task", // имя потока
    10000, // размер стека потока
    NULL, // параметры передаваемые в функцию run_autopilot()
    1, // приоритет потока
    NULL, // переменная для хранения дескриптора потока
    0 // номер ядра процессора, на котором будет выполняться поток
  );

  server.send(200, "text/html", SendHTML(Autopilot,Mechanik)); // отправка ответа клиенту
}


void run_autopilot(void *params) {
  int distance;
  int left_distance;
  int right_distance;

  while (Autopilot == HIGH) { // цикл работы автопилота
    distance = ultrasonic.read(); // чтение данных с датчика расстояния

    if (distance > 30) { // если расстояние до препятствия больше 20 см
      forward_motors(); // продолжаем движение вперед
    } else { // если расстояние до препятствия меньше или равно 20 см
      stop_motors(); // останавливаем двигатели
      delay(1000); // ждем одну секунду

      myservo.write(90); // поворачиваем сервопривод в центральное положение
      delay(500); // ждем полсекунды

      left_distance = ultrasonic.read(); // считываем расстояние до препятствия слева
      myservo.write(0); // поворачиваем сервопривод налево
      delay(500); // ждем полсекунды
      myservo.write(180);
      delay(500);
      right_distance = ultrasonic.read(); // считываем расстояние до препятствия справа
      myservo.write(90); // поворачиваем сервопривод в центральное положение

      if (left_distance >right_distance) { // если расстояние до препятствия слева больше, чем справа
right_motors(); // поворачиваем налево
delay(500); // ждем одну секунду
} else { // если расстояние до препятствия справа больше или равно, чем слева
left_motors(); // поворачиваем направо
delay(500); // ждем одну секунду
}stop_motors(); // останавливаемся перед продолжением движения
delay(1000);
}}

vTaskDelete(NULL); // завершение потока выполнения
}



void handle_led2on() {
  Autopilot = LOW;
  Mechanik = HIGH;
  Serial.println("GPIO5 Status: ON");
  server.send(200, "text/html", SendHTML(Autopilot,true)); 
}
void handle_led2off() {
  Mechanik = LOW;
  Serial.println("GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(Autopilot,false)); 
}

void handle_forward() {
  forward_motors();
  Serial.println("Forward");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik));
}

void handle_backward() {
  backward_motors();
  Serial.println("Backward");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik));
}

void handle_right() {
  right_motors();
  Serial.println("Right");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik));
}

void handle_left() {
  left_motors();
  Serial.println("Left");
  server.send(200, "text/html", SendHTML(Autopilot,Mechanik));
}

void handle_stop() {
stop_motors();
Serial.println("Stop");
server.send(200, "text/html", SendHTML(Autopilot,Mechanik));
}

void handle_NotFound(){
server.send(404, "text/plain", "Not found");
}

String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Управление светодиодами</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background-color: #212529; color: #ffffff;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #f8f9fa;margin: 50px auto 30px;} h3 {color: #dee2e6;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #007bff;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #28a745;}\n";
  ptr +=".button-on:active {background-color: #218838;}\n";
  ptr +=".button-off {background-color: #dc3545;}\n";
  ptr +=".button-off:active {background-color: #c82333;}\n";
  ptr +=".button-stop {display: block;width: 80px;height: 80px;background-color: #dc3545;border: none;color: white;padding: 13px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 40px;}\n";
  ptr +=".button-stop:hover {background-color: #c82333;}\n";
  ptr +="p {font-size: 14px;color: #dee2e6;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";

  if(led1stat) {
    ptr +="<p>Автопилот: ВКЛ.</p><a class=\"button button-off\" href=\"/led1off\">ВЫКЛ.</a>\n";
  } else {
    ptr +="<p>Автопилот: ВЫКЛ.</p><a class=\"button button-on\" href=\"/led1on\">ВКЛ.</a>\n";
  }
  if(led2stat) {
    ptr +="<p>Ручное управление: ВКЛ.</p><a class=\"button button-off\" href=\"/led2off\">ВЫКЛ.</a>\n";
  } else {
    ptr +="<p>Ручное управление: ВЫКЛ.</p><a class=\"button button-on\" href=\"/led2on\">ВКЛ.</a>\n";
  }

  // New buttons for directions and stop
  ptr +="<div style=\"display:flex; justify-content:center; margin-top:50px\">";
  ptr +="<a class=\"button button-on\" href=\"/forward\">Вперед</a>\n";
  ptr +="</div>";
  ptr +="<div style=\"display:flex; justify-content:center; margin-top:20px\">";
  ptr +="<a class=\"button button-on\" href=\"/left\">Влево</a>\n";
  ptr +="<a class=\"button button-stop\" href=\"/stop\" style=\"margin-left:20px\">Стоп</a>\n";
  ptr +="<a class=\"button button-on\" href=\"/right\" style=\"margin-left:20px\">Вправо</a>\n";
  ptr +="</div>";
  ptr +="<div style=\"display:flex; justify-content:center; margin-top:20px\">";
  ptr +="<a class=\"button button-on\" href=\"/backward\">Назад</a>\n";
  ptr +="</div>";

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}