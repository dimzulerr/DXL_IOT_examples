#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include <SPI.h>

#define UDP_PORT 8888
#define CHIPSELECT 4

IPAddress ip(192, 168, 42, 177);
static byte mac[] = {
  ip[0], ip[1], ip[2], ip[3], ip[0], ip[1]
};



EthernetServer server(80);
EthernetUDP udp;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
String udpIn;
String httpOut = "empty";

void setup() {
  Serial.begin(115200);
  //Инициализация интернет - соединений
  Ethernet.begin(mac, ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  server.begin();

  udp.begin(UDP_PORT);
  Serial.println("----------------------");
  Serial.println("UDP\HTTP connection is at: ");
  Serial.println(Ethernet.localIP());
  Serial.println("----------------------");
  //Инициализация SD
  if (!SD.begin(CHIPSELECT)) {
    Serial.println("Card failed, or not present");
    while (1)
      ;
  }
  Serial.println("Card initialized.");
  File log = SD.open("log.txt", FILE_WRITE);
  if (log) {
    log.print("Initialized with IP ");
    log.println(ip);
    log.close();
  }
}

void loop() {
  // Отрисовка данных, полученных по UDP, в веб интерфейс
    EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 1");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
            client.println(httpOut);
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  // Отслеживание UDP пакетов
  if (udp.parsePacket()) {
    IPAddress remote = udp.remoteIP();
    udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);

    httpOut = (String)remote[0] + "." + (String)remote[1] + "." + (String)remote[2] + "." + (String)remote[3] + " : " + packetBuffer;
    udpIn = "MSG from " + (String)remote[0] + "." + (String)remote[1] + "." + (String)remote[2] + "." + (String)remote[3] + " : " + packetBuffer;

    Serial.println(udpIn);
    udp.beginPacket(remote, UDP_PORT);
    char out[udpIn.length() + 1];
    udpIn.toCharArray(out, udpIn.length() + 1);
    udp.write(out);
    udp.endPacket();
    // Логирование полученного пакета на SD
    File log = SD.open("log.txt", FILE_WRITE);
    if (log) {
      log.println(httpOut);
      log.close();
    }
    ~log;
  } else {
    for (byte i = 0; i < 24; i++) {
      packetBuffer[i] = 0;
    }
  }
}
