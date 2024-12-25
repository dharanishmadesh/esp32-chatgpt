#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "ssid";
const char* password = "password";

// Replace with your OpenAI API Key
const char* api_key = "key";

// HTML page stored in program memory
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ChatGPT with ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #89f7fe, #66a6ff);
      color: #333;
      text-align: center;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }
    .container {
      background: white;
      border-radius: 10px;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
      padding: 30px;
      max-width: 400px;
      animation: fadeIn 1s ease-in-out;
    }
    @keyframes fadeIn {
      from {
        opacity: 0;
        transform: translateY(20px);
      }
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }
    h1 {
      color: #4a90e2;
    }
    input[type="text"] {
      width: 100%;
      padding: 10px;
      margin: 10px 0;
      border: 1px solid #ccc;
      border-radius: 5px;
    }
    input[type="submit"] {
      background: #4a90e2;
      color: white;
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      transition: background 0.3s;
    }
    input[type="submit"]:hover {
      background: #357ABD;
    }
    .response {
      margin-top: 20px;
      padding: 10px;
      background: #f1f1f1;
      border-radius: 5px;
      animation: slideIn 0.5s ease-in-out;
    }
    @keyframes slideIn {
      from {
        opacity: 0;
        transform: translateX(-20px);
      }
      to {
        opacity: 1;
        transform: translateX(0);
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Chat with ChatGPT</h1>
    <form method="POST" action="/">
      <input type="text" name="question" placeholder="Type your question..." required>
      <input type="submit" value="Ask GPT">
    </form>
  </div>
</body>
</html>
)rawliteral";

// Web server instance
WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available(); // Listen for incoming clients
  if (client) {
    Serial.println("New Client Connected.");
    String currentLine = "";
    String postData = "";
    bool isPost = false;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (isPost) {
          postData += c; // Collect POST data
        }
        if (c == '\n' && currentLine.length() == 0) {
          // End of HTTP headers
          if (isPost) {
            String question = getPostValue(postData, "question");
            if (question.length() > 0) {
              String response = sendChatGPTRequest(question);
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/html");
              client.println("Connection: close");
              client.println();
              client.println("<html><body>");
              client.println("<h1>ChatGPT Response</h1>");
              client.println("<p>" + response + "</p>");
              client.println("<a href='/'>Ask another question</a>");
              client.println("</body></html>");
              break;
            }
          } else {
            // Serve the HTML page
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println(html_page);
            break;
          }
        }
        if (c == '\n') {
          if (currentLine.startsWith("POST")) {
            isPost = true;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

// Extract POST data value
String getPostValue(String data, String key) {
  int start = data.indexOf(key + "=");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf('&', start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

// Send question to ChatGPT
String sendChatGPTRequest(String question) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.openai.com/v1/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + api_key);

    String payload = "{\"model\": \"gpt-3.5-turbo\", \"prompt\": \"" + question + "\", \"temperature\": 0, \"max_tokens\": 100}";
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
      String response = http.getString();
      http.end();
      return parseChatGPTResponse(response);
    } else {
      http.end();
      return "Error: Unable to reach OpenAI API";
    }
  }
  return "Error: Wi-Fi not connected";
}

// Parse ChatGPT response
String parseChatGPTResponse(String response) {
  int start = response.indexOf("\"text\":\"") + 8;
  int end = response.indexOf("\",", start);
  if (start != -1 && end != -1) {
    return response.substring(start, end);
  }
  return "Error: Unable to parse response";
}
