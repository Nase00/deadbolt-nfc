char passcode[64] = "";
char task[64] = "";

void handleTask(String task) {
  if (task == "lock") lock();
  if (task == "unlock") unlock();
  if (task == "toggle") toggle();
}

void handleRoot() {
  Serial.println("Enter handleRoot");
  String header;
  String content = "Successfully connected to device. ";

  if (server.hasHeader("passcode") && server.hasHeader("task")) {
    server.header("passcode").toCharArray(passcode, 64);
    server.header("task").toCharArray(task, 64);

    if (strcmp(passcode, SECRET_PASSCODE) == 0) {
      content += "Access granted.";
      server.send(200, "text/html", content);

      handleTask(task);
    } else {
      content += "Access denied.";
      server.send(401, "text/html", content);
    }
  } else {
    content += "Bad request.";
    server.send(500, "text/html", content);
  }

  Serial.println(content);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}
