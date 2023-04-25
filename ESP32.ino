#include <WebServer.h>
#include <WiFi.h>        //wifi library
#include <Adafruit_BMP280.h> //BMP280 library
#include <LiquidCrystal_I2C.h> //LCD library
#include <TinyGPS++.h> //GPS library
#include <SoftwareSerial.h> //Serial connection GPIO adjust

/****** HTTP CONNECTION SETUP ******/
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
WiFiServer server(80); // Set web server port number to 80
String header; // Variable to store the HTTP request
unsigned long currentTime = millis(); 
unsigned long previousTime = 0; 
const long timeoutTime = 2000; // Define timeout time in milliseconds

/****** BMP280_SENSOR ******/
Adafruit_BMP280 bmp;  // I2C Interface
// for more accurate https://barometricpressure.app/cities
//  insert your location sea level pressure (milibar)
float SEALEVELPRESSURE_HPA = 1013.25; //default value


/****** I2C_LCD ******/
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); 
int lcdColumns = 20;
int lcdRows = 4;


/****** GLOBAL_STRING_DISPLAY ******/
String strLon = "not available";
String strLat = "not available";
String strAlt = "not available";
String strTemp, strPressure;
String numSats = "0";
String sym="/";


/****** NEO-6M_GPS_MODULE******/
static const int RXPin = 16, TXPin = 17; //UART2 pin configuration
static const uint32_t GPSBaud = 9600; //GPS baud rate 
TinyGPSPlus gps; //create TinyGPSPlus object
SoftwareSerial ss(RXPin, TXPin);  // The serial connection object to the GPS device

void setup() {
  Serial.begin(9600); //start UART1 serial commnuication 
  lcd.init();// initialize LCD
  lcd.backlight();
  delay(1000);
  lcd.clear();

  //Connect to Wi-Fi network with given SSID and password
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting");
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  lcd.clear();
  lcd.setCursor(0, 0);  
  lcd.print("Connected:");
  lcd.setCursor(0, 1);  
  lcd.print(WiFi.localIP());
  server.begin();
  delay(2000);

  //BMP280 Initialization
  if (!bmp.begin(0x76)) {
    lcd.print("BMP280 not found.");
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  delay(3000);
  lcd.clear();
  lcd.print("BMP280 started.");
  delay(2000);
  lcd.clear();

  // GPS Initialization
  ss.begin(GPSBaud);
  lcd.clear();
  lcd.print("Booting GPS.");
  delay(5000);
  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPS++ with an attached GPS module"));
  Serial.print(F("Testing TinyGPS++ library v. "));
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();

}

void loop() {

  while (ss.available() > 0) {
    if (gps.encode(ss.read())) {  //funnel characters from GPS module using encode()
      strLat = String(gps.location.lat(), 6);
      strLon = String(gps.location.lng(), 6);
      numSats = String(gps.satellites.value());
      strTemp = bmp.readTemperature();
      strPressure = bmp.readPressure() / 100.0F;
      strAlt = bmp.readAltitude(SEALEVELPRESSURE_HPA);
      displayUART(); //send data to serial monitor
      displayLCD(); //display Temperature,Pressure and Altitude on LCD
      webSever(); //create websever
      delay(10000);
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }
}

void webSever() { 
  WiFiClient client = server.available();

  if (client) {  // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");                                             // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #0043af; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");

            // Web Page Heading
            client.println("</style></head><body><h1>ESP32 with BMP280 && NEO-6M GPS</h1>");
            client.println("<table><tr><th>MEASUREMENT</th><th>VALUE</th></tr>");
            client.println("<tr><td>Date</td><td><span class=\"sensor\">");
            String date= gps.date.month() + sym + gps.date.day() + sym + gps.date.year();
            client.println(date);
            client.println("</span></td></tr>");
            client.println("<tr><td>Temp. Celsius</td><td><span class=\"sensor\">");
            client.println(strTemp);
            client.println(" *C</span></td></tr>");
            client.println("<tr><td>Temp. Fahrenheit</td><td><span class=\"sensor\">");
            client.println(1.8 * bmp.readTemperature() + 32);
            client.println(" *F</span></td></tr>");
            client.println("<tr><td>Pressure</td><td><span class=\"sensor\">");
            client.println(strPressure);
            client.println(" hPa</span></td></tr>");
            client.println("<tr><td>Approx. Altitude</td><td><span class=\"sensor\">");
            client.println(strAlt);
            client.println(" m</span></td></tr>");
            client.println("<tr><td>Latitude</td><td><span class=\"sensor\">");
            client.println(strLat);
            client.println("</span></td></tr>");
            client.println("<tr><td>Longitude</td><td><span class=\"sensor\">");
            client.println(strLon);
            client.println("</span></td></tr>");
            client.println("<tr><td>Tracked Satellites </td><td><span class=\"sensor\">");
            client.println(numSats);
            client.println("</span></td></tr>");
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void displayLCD() {
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("Temperature ");
  lcd.setCursor(12,0);
  lcd.print(strTemp); lcd.print(" *C");
  lcd.setCursor(0,1);
  lcd.print("Pressure ");
  lcd.setCursor(9,1);
  lcd.print(strPressure); lcd.print(" hPa");
  lcd.setCursor(0,2);
  lcd.print("Altitude ");
  lcd.setCursor(9,2);
  lcd.print(strAlt); lcd.print(" m");
  lcd.setCursor(0,3);
  lcd.print("Sats ");
  lcd.setCursor(5,3);
  lcd.print(numSats);
  delay(10);
}


void displayUART() {
  Serial.println();
  Serial.print(F("Temperature = "));
  Serial.print(strTemp); //fetch and print Temperature
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(strPressure); ////fetch and display Pressure in hPa
  Serial.println(" hPa");

  Serial.print(F("Approx altitude = "));
  Serial.print(strAlt); //fetch and display Altitude in meters
  Serial.println(" m");
  Serial.println();

  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(strLat);
    Serial.print(F(","));
    Serial.println(strLon);

  } else {
    Serial.println(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.println(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  } else {
    Serial.print(F("INVALID"));
  }
}