#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

class ConnectionManager;

class Comms {
public:
  void begin(ConnectionManager& cm);

  bool publishBoot(const char* payload);
  bool publishLog(const char* payload);
  bool publishEvents(const char* payload);
  bool publishResp(const char* payload);
  bool publishStatus(const char* payload);
  bool publishEventJson(const char* json, bool retained = false);


  // Home Assistant Discovery + State
  bool publishHAAvailability(const char* payload, bool retained = true);
  bool publishHAConfig(bool retained = true);
  bool publishHAState(float tempC, float humPct, time_t epoch);

private:
  ConnectionManager* _cm = nullptr;
  PubSubClient* _mqtt = nullptr;

  bool publishRaw(const char* topic, const char* payload, bool retained = false);
};
