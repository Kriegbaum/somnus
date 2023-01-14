/* Somnum testing file */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_peripherals.h"
#include "board.h"
#include "buttons.hpp"
#include "player.hpp"

static const char *TAG =  "APP_MAIN";

extern "C" 
{
    void app_main();
}

void app_main()
{
    ESP_LOGI(TAG, "Booting up...");
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_DEBUG);

    // Set up peripheral manager
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // Set up board manager
    audio_board_handle_t board_handle = audio_board_init();

    ButtonInputs buttons = ButtonInputs();
    buttons.init(set, board_handle);

    MusicPlayer player = MusicPlayer();
    player.init(set, board_handle);

    while(1)
    {
        //player.tick();
    }
}

