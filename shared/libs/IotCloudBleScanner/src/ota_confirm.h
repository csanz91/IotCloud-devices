#pragma once

#if defined(ARDUINO) && defined(ESP32)

#include <Arduino.h>
#include <esp_ota_ops.h>

// Rollback guard for OTA images.
//
// By default the Arduino core marks a new OTA image valid as soon as it boots,
// so an image that crashes later keeps crash-looping. Each project defines
// (in its own src/, so the linker can't skip it):
//
//     extern "C" bool verifyRollbackLater() { return true; }
//
// and calls ota_confirm_when(healthy) from loop(). The image stays "pending"
// until `healthy` is first true; if the device reboots before that, the
// bootloader goes back to the previous image. Needs a bootloader built with
// rollback support (Arduino core 3.x has it); otherwise this does nothing.
inline void ota_confirm_when(bool healthy)
{
    static bool done = false;
    if (done || !healthy)
        return;
    done = true;

    esp_ota_img_states_t state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        esp_ota_mark_app_valid_cancel_rollback();
        IOTCLOUD_LOG_PRINTLN(F("OTA: new image confirmed"));
    }
}

#endif // ARDUINO && ESP32
