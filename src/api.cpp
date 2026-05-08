#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "api.h"

static String accessToken = "";
static unsigned long tokenFetchedAt = 0;
static unsigned long tokenLifetimeMs = 0;

String fetchToken() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, "https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = "grant_type=client_credentials";
    body += "&client_id=" + String(OPENSKY_CLIENT_ID);
    body += "&client_secret=" + String(OPENSKY_CLIENT_SECRET);

    int code = http.POST(body);
    String response = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return "";
    }

    String token = doc["access_token"].as<String>();
    return token;
}

String getAccessToken() {
    if (millis() - tokenFetchedAt > tokenLifetimeMs - 60000) {
        fetchToken();
    }
    return accessToken;
}