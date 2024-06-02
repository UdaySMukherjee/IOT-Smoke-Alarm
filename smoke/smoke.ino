#include <WiFi.h>
#include <WiFiClientSecure.h>

// Replace with your network credentials
const char* ssid = "Nbs Shadow";
const char* password = "Nbsshadow";

// Replace with your email credentials
const char* smtpServer = "smtp.gmail.com";
const int smtpPort = 465;
const char* emailSenderAccount = "usmtechlab@gmail.com";
const char* emailSenderPassword = "usmtechlab@01";
const char* emailRecipient = "udaysankar2003@gmail.com";

const int mq2Pin = 34; // Analog pin for MQ2 sensor
const int relayPin = 5; // Digital pin for Relay module
const int buzzerPin = 4; // Digital pin for Buzzer

int gasThreshold = 400; // Threshold value for gas detection

WiFiClientSecure client;

// Function declarations
void sendEmailNotification(String subject, String message);
String encodeBase64(String input);
bool sendSMTPCommand(String command, int expectedResponseCode);
bool waitForResponse(int expectedResponseCode, int timeout = 10000);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  client.setInsecure(); // Disable certificate verification

  pinMode(mq2Pin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(relayPin, LOW);
  digitalWrite(buzzerPin, LOW);
}

void sendEmailNotification(String subject, String message) {
  if (WiFi.status() == WL_CONNECTED) {
    client.connect(smtpServer, smtpPort);

    if (!client.connected()) {
      Serial.println("Failed to connect to SMTP server");
      return;
    }

    // Initializing the SMTP connection
    if (!sendSMTPCommand("EHLO smtp.gmail.com", 250)) return;
    if (!sendSMTPCommand("AUTH LOGIN", 334)) return;
    if (!sendSMTPCommand(encodeBase64(emailSenderAccount), 334)) return;
    if (!sendSMTPCommand(encodeBase64(emailSenderPassword), 235)) return;

    // Sending the email
    if (!sendSMTPCommand("MAIL FROM: <" + String(emailSenderAccount) + ">", 250)) return;
    if (!sendSMTPCommand("RCPT TO: <" + String(emailRecipient) + ">", 250)) return;
    if (!sendSMTPCommand("DATA", 354)) return;
    client.println("Subject: " + subject);
    client.println("To: " + String(emailRecipient));
    client.println("From: " + String(emailSenderAccount));
    client.println();
    client.println(message);
    client.println(".");
    if (!waitForResponse(250)) return;
    sendSMTPCommand("QUIT", 221);

    Serial.println("Email sent successfully");
  } else {
    Serial.println("WiFi Disconnected");
  }
}

String encodeBase64(String input) {
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String encoded;
  int i = 0;
  int j = 0;
  int in_len = input.length();
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(input.c_str() + j++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; i < 4; i++)
        encoded += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; j < i + 1; j++)
      encoded += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      encoded += '=';
  }

  return encoded;
}

bool sendSMTPCommand(String command, int expectedResponseCode) {
  client.println(command);
  return waitForResponse(expectedResponseCode);
}

bool waitForResponse(int expectedResponseCode, int timeout) {
  long start = millis();
  while (!client.available()) {
    if (millis() - start >= timeout) {
      Serial.println("SMTP server didn't respond in time");
      return false;
    }
  }
  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line); // Print the server response for debugging
    if (line.startsWith(String(expectedResponseCode))) {
      return true;
    } else if (line.startsWith("5") || line.startsWith("4")) {
      Serial.println("SMTP server returned an error: " + line);
      return false;
    }
  }
  return false;
}

void loop() {
  int gasLevel = analogRead(mq2Pin);
  Serial.print("Gas Level: ");
  Serial.println(gasLevel);

  if (gasLevel > gasThreshold) {
    // Gas leak detected
    digitalWrite(relayPin, HIGH); // Turn on exhaust fan
    digitalWrite(buzzerPin, HIGH); // Turn on buzzer
    sendEmailNotification("Smoke Detected", "Alert! A smoke has been detected.");
    delay(60000); // Wait for a minute before sending another email
  } else {
    // No gas leak
    digitalWrite(relayPin, LOW); // Turn off exhaust fan
    digitalWrite(buzzerPin, LOW); // Turn off buzzer
  }

  delay(1000); // Read sensor every second
}
