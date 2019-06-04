void sendRequest(const String data) {
  Serial.print("connecting to ");
  Serial.println(DOOR_STATE_SERVICE_HOST);

  WiFiClient client;
  const int httpPort = 4000;
  if (!client.connect(DOOR_STATE_SERVICE_HOST, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting path: ");
  Serial.println(DOOR_STATE_SERVICE_PATH);

  String request = String("POST ") + "/api" + " HTTP/1.1\r\n" +
    "Host: " + DOOR_STATE_SERVICE_HOST + "\r\n" +
    "Content-Type: application/json\r\n" +
    "event: BATCH_EVENTS_FROM_WEBHOOK\r\n" +
    "id: " + DOOR_STATE_SERVICE_ID + "\r\n" +
    "password: " + DOOR_STATE_SERVICE_PASSCODE + "\r\n" +
    "webhookbody: " + data + "\n";
  Serial.print(request);
  client.print(request);

  delay(500);

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print("Server response: ");
    Serial.println(line);
  }

  Serial.println();
  Serial.println("closing connection");
}
