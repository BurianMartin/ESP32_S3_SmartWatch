#include "LilyGoWatch.h"

bool LilyGoWatch::InitMicrophone()
{
    static i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = MIC_I2S_SAMPLE_RATE,
        .bits_per_sample = MIC_I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 512,
        .use_apll = true};

    static i2s_pin_config_t i2s_cfg = {0};
    i2s_cfg.bck_io_num = I2S_PIN_NO_CHANGE;
    i2s_cfg.ws_io_num = BOARD_MIC_CLOCK;
    i2s_cfg.data_out_num = I2S_PIN_NO_CHANGE;
    i2s_cfg.data_in_num = BOARD_MIC_DATA;
    i2s_cfg.mck_io_num = I2S_PIN_NO_CHANGE;

    if (i2s_driver_install(MIC_I2S_PORT, &i2s_config, 0, NULL) != ESP_OK)
        return false;

    if (i2s_set_pin(MIC_I2S_PORT, &i2s_cfg) != ESP_OK)
        return false;

    return true;
}

void LilyGoWatch::SetUpAudio()
{
    if (xSemaphoreTake(Watch.AudioPlayback.Audio_Acces_Mutex, portMAX_DELAY))
    {
        AudioPlayback.out = new AudioOutputI2S(1, AudioOutputI2S::EXTERNAL_I2S);
        AudioPlayback.out->SetPinout(BOARD_DAC_IIS_BCK, BOARD_DAC_IIS_WS, BOARD_DAC_IIS_DOUT);
        AudioPlayback.out->SetGain(0.2);

        AudioPlayback.file_fs = new AudioFileSourceLittleFS();
        AudioPlayback.wav = new AudioGeneratorWAV();
        xSemaphoreGive(Watch.AudioPlayback.Audio_Acces_Mutex);
    }
}

bool LilyGoWatch::ReadMicrophone(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    return i2s_read(MIC_I2S_PORT, dest, size, bytes_read, ticks_to_wait) == ESP_OK;
}

void LilyGoWatch::PDM_Record(const char *song_name, uint32_t duration)
{
    if (!CreateFileWAV(song_name, duration, 1, MIC_I2S_SAMPLE_RATE, MIC_I2S_BITS_PER_SAMPLE))
    {
        Serial.println("Error during wav header creation");
        return;
    }

    Serial.println("Create wav file success!");

    const size_t BUFFER_SIZE = 500;
    uint8_t *buf = (uint8_t *)malloc(BUFFER_SIZE);
    if (!buf)
    {
        Serial.println("Failed to alloc memory");
        return;
    }

    File audio_file = LittleFS.open(song_name, FILE_APPEND);
    if (!audio_file)
    {
        Serial.println("Failed to create file");
        return;
    }

    uint32_t data_size = MIC_I2S_SAMPLE_RATE * MIC_I2S_BITS_PER_SAMPLE * duration / 8;

    uint32_t counter = 0;
    size_t bytes_written;
    Serial.println("Recording started");
    int percentage = 0;

    while (counter != data_size)
    {
        percentage = ((float)counter / (float)data_size) * 100;
        Serial.print(percentage);
        Serial.println("%");

        if (counter > data_size)
        {
            Serial.println("File is corrupted. data_size must be multiple of BUFFER_SIZE. Please modify BUFFER_SIZE");
            break;
        }

        if (!ReadMicrophone(buf, BUFFER_SIZE, &bytes_written))
            Serial.println("readMicrophone() error");

        if (bytes_written != BUFFER_SIZE)
            Serial.println("Bytes written error");

        audio_file.write(buf, BUFFER_SIZE);

        counter += BUFFER_SIZE;
    }
    Serial.println("Recording finished");

    audio_file.close();
    free(buf);
}

void LilyGoWatch::Record(void *parameter)
{
    Watch.EnsureAudioReady();
    Serial.println("Recording");
    Watch.PDM_Record(DEFAULT_RECORD_FILENAME, 8);
    vTaskDelete(NULL);
}

void LilyGoWatch::PlayRecordingFromStorage(void *parameter)
{
    Watch.EnsureAudioReady();
    if (xSemaphoreTake(Watch.AudioPlayback.Audio_Acces_Mutex, portMAX_DELAY))
    {
        Watch.AudioPlayback.out->SetGain(1);

        if (!Watch.AudioPlayback.file_fs->open(DEFAULT_RECORD_FILENAME))
        {
            Serial.println("Failed to open file!");
            xSemaphoreGive(Watch.AudioPlayback.Audio_Acces_Mutex);
            vTaskDelete(NULL);
            return;
        }

        Watch.AudioPlayback.wav->begin(Watch.AudioPlayback.file_fs, Watch.AudioPlayback.out);

        Serial.println("Running wav player!");

        while (Watch.AudioPlayback.wav->isRunning())
        {
            if (!Watch.AudioPlayback.wav->loop())
            {
                Serial.println("Playback stopped.");
                Watch.AudioPlayback.wav->stop();
                break;
            }
            Watch.PowerManage.ResetIdleTime();
            vTaskDelay(10);
        }

        Watch.AudioPlayback.wav->stop();
        xSemaphoreGive(Watch.AudioPlayback.Audio_Acces_Mutex);
    }

    vTaskDelete(NULL);
}
