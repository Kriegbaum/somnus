#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "flac_decoder.h"
#include "audio_type_def.h"
#include "dram_list.h"
#include "sdcard_scan.h"

static const char *PLAYER_TAG = "MUSIC_PLAYER";

class MusicPlayer
{
    public:

    MusicPlayer()
    {
        ESP_LOGI(PLAYER_TAG, "Creating player instance...");
    }

    esp_err_t init(esp_periph_set_handle_t &periph_settings, audio_board_handle_t &audio_board_handle) 
    {
        ESP_LOGI(PLAYER_TAG, "Initializing SD card...");
        audio_board_sdcard_init(periph_settings, SD_MODE_SPI);
        
        ESP_LOGI(PLAYER_TAG, "Initializing playback queue...");
        dram_list_create(&playback_queue_handle);
        const char *supported_types[2] = {"mp3", "flac"};
        sdcard_scan(save_queue_callback, "/sdcard", 0, supported_types, 2, playback_queue_handle);
        dram_list_show(playback_queue_handle);

        /*
        ESP_LOGI(PLAYER_TAG, "Initializing codec chip...");
        audio_hal_ctrl_codec(audio_board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

        ESP_LOGI(PLAYER_TAG, "Initializing pipeline...");
        pipeline = audio_pipeline_init(&pipeline_cfg);
        mem_assert(pipeline);

        ESP_LOGI(PLAYER_TAG, "Creating fatfs streamer...");
        fatfs_cfg.type = AUDIO_STREAM_READER;
        fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

        ESP_LOGI(PLAYER_TAG, "Creating I2S Stream...");
        i2s_cfg.type = AUDIO_STREAM_WRITER;
        i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    
        ESP_LOGI(PLAYER_TAG, "Creating mp3 decoder...");
        mp3_decoder = mp3_decoder_init(&mp3_cfg);

        ESP_LOGI(PLAYER_TAG, "Creating flac decoder...");
        flac_decoder = flac_decoder_init(&flac_dec_cfg);

        ESP_LOGI(PLAYER_TAG, "Attaching pipeline elements, [sdcard]->fatfs_stream->decoder->i2s_stream->[codec_chip]");
        audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
        audio_pipeline_register(pipeline, flac_decoder, "flac_decoder");
        audio_pipeline_register(pipeline, mp3_decoder, "mp3_decoder");
        audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
        audio_pipeline_link(pipeline, &flac_pipe_tag[0], 3);

        dram_list_next(playback_queue_handle, 1, &current_uri);
        ESP_LOGI(PLAYER_TAG, "Setting uri to %s", current_uri);
        audio_element_set_uri(fatfs_stream_reader, current_uri);

        ESP_LOGI(PLAYER_TAG, "Set up event listener");
        evt = audio_event_iface_init(&evt_cfg);

        ESP_LOGI(PLAYER_TAG, "Register listener to pipeline");
        audio_pipeline_set_listener(pipeline, evt);
        
        ESP_LOGI(PLAYER_TAG, "Start playback...");
        set_pipeline_codec_from_uri(current_uri);
        audio_pipeline_run(pipeline);

        audio_element_report_codec_fmt(fatfs_stream_reader);
        */
        return ESP_OK;
    }

    esp_err_t set_format_flac()
    {
        ESP_LOGI(PLAYER_TAG, "Switching to flac decoder...");
        audio_pipeline_pause(pipeline);
        audio_pipeline_breakup_elements(pipeline, mp3_decoder);
        audio_pipeline_relink(pipeline, &flac_pipe_tag[0], 3);
        //audio_pipeline_reset_ringbuffer(pipeline);
        //audio_pipeline_reset_elements(pipeline);
        audio_pipeline_set_listener(pipeline, evt);
        audio_pipeline_run(pipeline);
        audio_pipeline_resume(pipeline);
        current_codec = ESP_CODEC_TYPE_FLAC;
        return ESP_OK;
    }

    esp_err_t set_format_mp3()
    {
        ESP_LOGI(PLAYER_TAG, "Switching to mp3 decoder...");
        audio_pipeline_pause(pipeline);
        audio_pipeline_breakup_elements(pipeline, flac_decoder);
        audio_pipeline_relink(pipeline, &mp3_pipe_tag[0], 3);
        //audio_pipeline_reset_ringbuffer(pipeline);
        //audio_pipeline_reset_elements(pipeline);
        audio_pipeline_set_listener(pipeline, evt);
        audio_pipeline_run(pipeline);
        audio_pipeline_resume(pipeline);
        current_codec = ESP_CODEC_TYPE_MP3;
        return ESP_OK;
    }
    
    esp_err_t tick()
    {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(PLAYER_TAG, "Event interface error : %d", ret);
            return ret;
        }

        // We recieve a REPORT_MUSIC_INFO command here, indicating a new track has started
        // In this case, we need to update the i2s stream with its properties
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && 
            msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(flac_decoder, &music_info);
            ESP_LOGI(PLAYER_TAG, "Received new music info for %s, sample_rates=%d, bits=%d, ch=%d",
                     current_uri, music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
        }

        // If the i2s stream is finsihed, we are done playing this track and should advance to next
        if (msg.source == (void *) i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
        {
            audio_element_state_t i2s_state = audio_element_get_state(i2s_stream_writer);
            if (i2s_state == AEL_STATE_FINISHED)
            {
                ESP_LOGI(PLAYER_TAG, "Finished playback for %s", current_uri);
                play_next();
            }
        }
        return ESP_OK;
    }

    esp_codec_type_t get_uri_codec(char* &uri)
    {   
        // Get dot and everything after in filename
        char *extension = strrchr(uri, '.');
        // We didn't find an extension at all
        if(extension == NULL) {return ESP_CODEC_TYPE_UNKNOW;}
        // Strip the dot out and get just the extension
        extension ++;
        if(!strcasecmp(extension, get_codec_ext(ESP_CODEC_TYPE_MP3)))
        {
            return ESP_CODEC_TYPE_MP3;
        }
        else if(!strcasecmp(extension, get_codec_ext(ESP_CODEC_TYPE_FLAC)))
        {
            return ESP_CODEC_TYPE_FLAC;
        }
        else
        {
            return ESP_CODEC_TYPE_UNKNOW;
        }
    }

    void set_pipeline_codec_from_uri(char* &uri)
    {
        esp_codec_type_t codec_type = get_uri_codec(uri);
        if (codec_type == current_codec)
        {
            return;
        }
        else if (codec_type == ESP_CODEC_TYPE_FLAC)
        {
            set_format_flac();
        }
        else if (codec_type == ESP_CODEC_TYPE_MP3)
        {
            set_format_mp3();
        }
    }
           
    void play_next()
    {
        ESP_LOGI(PLAYER_TAG, "Playing next song!");
        dram_list_next(playback_queue_handle, 1, &current_uri);
        ESP_LOGI(PLAYER_TAG, "Next song is %s", current_uri);
        audio_element_set_uri(fatfs_stream_reader, current_uri);
        set_pipeline_codec_from_uri(current_uri);
        audio_pipeline_reset_ringbuffer(pipeline);
        audio_pipeline_reset_elements(pipeline);
        audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
        audio_pipeline_run(pipeline);
    }

    private:
    audio_element_handle_t fatfs_stream_reader, i2s_stream_writer, flac_decoder, mp3_decoder;
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();

    audio_pipeline_handle_t pipeline;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();

    audio_event_iface_handle_t evt;
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();

    const char *mp3_extension = ".mp3";
    const char *mp3_pipe_tag[3] = {"file", "mp3_decoder", "i2s"};
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();

    const char *flac_extension = ".flac";
    const char *flac_pipe_tag[3] = {"file", "flac_decoder", "i2s"};
    flac_decoder_cfg_t flac_dec_cfg = DEFAULT_FLAC_DECODER_CONFIG();

    char *current_uri =  NULL;
    int current_codec = NULL;   

    playlist_operator_handle_t playback_queue_handle = NULL;

    static void save_queue_callback(void *user_data, char *url)
    {
        playlist_operator_handle_t sdcard_handle = (playlist_operator_handle_t) user_data;
        esp_err_t ret = dram_list_save(sdcard_handle, url);
        if (ret != ESP_OK)
        {
            ESP_LOGE(PLAYER_TAG, "Failed to add item to playback queue...");
        }
    }
};

