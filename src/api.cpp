#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "secrets.h"
#include "api.h"

using namespace std;

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

    if (code != HTTP_CODE_OK) {
        Serial.printf("Token request failed: code=%d body=%s\n", code, response.c_str());
        return "";
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return "";
    }

    String token = doc["access_token"].as<String>();
    unsigned long expiresIn = doc["expires_in"].as<unsigned long>();
    tokenFetchedAt = millis();
    tokenLifetimeMs = expiresIn * 1000;
    accessToken = token;
    return token;
}

String getAccessToken() {
    if (millis() - tokenFetchedAt + 60000 > tokenLifetimeMs) {
        fetchToken();
    }
    return accessToken;
}

vector<Flight> getFlights(const double currentLat, const double currentLong) {
    double radius = 1.0; 
    double lamin = currentLat - radius;
    double lamax = currentLat + radius;
    double lomin = currentLong - radius;
    double lomax = currentLong + radius;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String url = "https://opensky-network.org/api/states/all";
    url += "?lamin=" + String(lamin, 4);
    url += "&lomin=" + String(lomin, 4);
    url += "&lamax=" + String(lamax, 4);
    url += "&lomax=" + String(lomax, 4);

    String authToken = getAccessToken();
    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + authToken);

    int code = http.GET();
    String response = http.getString();
    http.end();

    if (code != HTTP_CODE_OK) {
        Serial.printf("Get flights request failed: code=%d body=%s\n", code, response.c_str());
        return {};
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return {};
    }

    vector<Flight> flights;
    for (JsonVariant state : doc["states"].as<JsonArray>()) {
        Flight f;
        f.icao24 = state[0].as<String>();

        f.callsign = state[1].as<String>();
        f.callsign.trim();

        if (state[5].isNull() || state[6].isNull()) {
            continue;
        }
        f.lon = state[5].as<float>();
        f.lat = state[6].as<float>();

        f.altitude = state[7].as<float>();
        f.velocity = state[9].as<float>();
        f.heading = state[10].as<float>();
        f.onGround = state[8].as<bool>();
        flights.push_back(f);
    }

    return flights;
}