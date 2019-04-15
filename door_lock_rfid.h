/*
 * ESPhome RC522 RFID plugin with access control
 * 
 * Created on: 17.03.2019
 *
 * Copyright (c) 2019 Jakub Pachciarek. All rights reserved.
 * 
 */

#include "esphome.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <SPI.h>
#include <MFRC522.h>

using namespace esphome;
using namespace std;

class RfidSensorsComponent : public Component {
 private:
  MFRC522 rfid;

  typedef struct {
      String id;
      String description;
  } user;

  vector<user> users_list;

  unsigned long last_tag_read_milis = 0;

  void send_notification(String title, String message, String notification_id) {
    vector<api::KeyValuePair> data;
    data.push_back(api::KeyValuePair("title", title.c_str()));
    data.push_back(api::KeyValuePair("message", message.c_str()));
    data.push_back(api::KeyValuePair("notification_id", notification_id.c_str()));

    api::ServiceCallResponse resp;
    resp.set_service("persistent_notification.create");
    resp.set_data(data);
    api::global_api_server->send_service_call(resp);
  }

  void import_users_list(Stream* f) {
    ESP_LOGD("rfid", "Parsing users list...");

    users_list.clear();

    while (f->available()) {
      String line;
      char tmp = 0;
      while (f->available() && tmp != '\n'){
        tmp = f->read();
        line += tmp;
      }

      line.trim();

      size_t split_pos = line.indexOf('=');

      if (split_pos == 11) { //Naive ID check: XX:XX:XX:XX=DESCRIPTION
        user row;
        row.id = line.substring(0, split_pos);
        row.description = line.substring(split_pos + 1);
        users_list.push_back(row);
      }
    }
  }

  bool load_users_list() {
    ESP_LOGD("rfid", "Loading users list from file...");
    File f = SPIFFS.open("/rfid_users_list", "r");

    if (!f) {
      ESP_LOGE("rfid", "Failed to open users list file!");
      return false;
    }

    import_users_list(&f);

    f.close();

    return true;
  }

  bool save_users_list() {
    ESP_LOGD("rfid", "Saving users list to file...");
    File f = SPIFFS.open("/rfid_users_list", "w");

    if (!f) {
      ESP_LOGE("rfid", "Failed to open users list file!");
      return false;
    }

    for (auto &user : users_list) {
      f.print(user.id + "=" + user.description + '\n');
    }

    f.close();

    return true;
  }

  void handle_tag_read() {
    if (last_tag_read_milis != 0 && (last_tag_read_milis + 5000) > millis()) return;

    valid_tag_sensor->publish_state(false);
    invalid_tag_sensor->publish_state(false);

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

    user current_tag;
    bool known_tag = false;

    char tag_id_buffer[12];
    sprintf(tag_id_buffer, "%02X:%02X:%02X:%02X", rfid.uid.uidByte[0], rfid.uid.uidByte[1], rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    current_tag.id = tag_id_buffer;
    current_tag.description = "UNKNOWN";

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    for (size_t i = 0; i < users_list.size(); i++) {
      if (users_list[i].id == current_tag.id) {
        current_tag = users_list[i];
        known_tag = true;
        break;
      }
    }

    ESP_LOGD("rfid", "Tag with id %s found!", current_tag.id.c_str());
    ESP_LOGD("rfid", "Setting sensor states...");

    last_tag_read_milis = millis();

    valid_tag_sensor->publish_state(known_tag);
    invalid_tag_sensor->publish_state(!known_tag);
    last_tag_id_sensor->publish_state(current_tag.id.c_str());
    last_tag_description_sensor->publish_state(current_tag.description.c_str());
  }

 public:
  binary_sensor::BinarySensor *valid_tag_sensor = new binary_sensor::BinarySensor();
  binary_sensor::BinarySensor *invalid_tag_sensor = new binary_sensor::BinarySensor();
  text_sensor::TextSensor *last_tag_id_sensor = new text_sensor::TextSensor();
  text_sensor::TextSensor *last_tag_description_sensor = new text_sensor::TextSensor();

  void send_users_list_notification() {
    String message;
    for (auto &user : users_list) {
      message += user.id + " - " + user.description + '\n';
    }

    send_notification((App.get_name() + ": users list").c_str(), message, (App.get_name() + "_users_list_print").c_str());
  }

  void sync_users_list_from_server(string users_server_url) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (users_server_url.length() == 0) return;

    String title = (App.get_name() + ": users list update").c_str();
    String id = (App.get_name() + "_users_list_update").c_str();

    ESP_LOGD("rfid", "Fetching users list from server...");

    HTTPClient http;
    http.begin(users_server_url.c_str());
    int httpCode = http.GET();

    if (httpCode == 200) {
      import_users_list(http.getStreamPtr());
    } else {
      ESP_LOGE("rfid", "Invalid server response. Server unavailable?");
      send_notification(title, "Invalid server response. Server unavailable?", id);
      return;
    }

    http.end();

    save_users_list();
    send_users_list_notification();
  }

  void setup() override {
    SPIFFS.begin();
    SPI.begin();
    rfid.PCD_Init(D8, D3);

    load_users_list();
  }

  void loop() override {
    handle_tag_read();
  }
};