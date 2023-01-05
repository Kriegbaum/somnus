#include "input_key_service.h"
#include "esp_peripherals.h"
#include "board.h"

static const char *BUTTONS_TAG = "BUTTON_INPUTS";

class ButtonInputs
{
    public:

    ButtonInputs()
    {
        ESP_LOGI(BUTTONS_TAG, "Creating button input instance");
    }

    esp_err_t init(esp_periph_set_handle_t &periph_settings, audio_board_handle_t &audio_board_handle)
    {
        ESP_LOGI(BUTTONS_TAG, "Initializing buttons...");
        audio_board_key_init(periph_settings);
        input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
        input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
        input_cfg.handle = periph_settings;
        periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
        input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
        periph_service_set_callback(input_ser, input_key_service_callback, (void *)audio_board_handle);
        return ESP_OK;
    }

    static esp_err_t input_key_service_callback(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
    {
        ESP_LOGI(BUTTONS_TAG, "[ * ] key id %d %d", (int)evt->data, evt->type);
        return ESP_OK;
    }

};

