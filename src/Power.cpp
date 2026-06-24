#include "LilyGoWatch.h"

Power::Power()
{
    ShouldWakeUp = false;
    Asleep = false;
    idle_time = 0;
    sleep_timeout = SLEEP_TIMEOUT;
}

void Power::HandleSleepActions(uint8_t setBrightness, bool enterLightSleep)
{
    if (ShouldSleep())
        GoToSleep(enterLightSleep);
    else if (ShouldWakeUp)
        WakeUpFromSleep(setBrightness);
}

void Power::WakeUpFromSleep(uint8_t setBrightness)
{
#ifdef ACCELEROMETER
    Watch.sensor.getIrqStatus();
    Watch.sensor.readIrqStatus();
#endif

    Watch.SetDisplayBrightnessPercent(setBrightness);

    Watch.TouchDrvFT6X36::reset();
    Watch.TouchDrvFT6X36::setPowerMode(TouchDrvFT6X36::PMODE_ACTIVE);

    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        ShouldWakeUp = false;
        Asleep = false;
        Watch.clearIrqStatus();
        idle_time = 0;
        xSemaphoreGive(Power_Acces_Mutex);
    }

    Watch.UpdateWatchScreen();

    Serial.println("Wake Up");
}

void Power::GoToSleep(bool enterLightSleep)
{
    Serial.println("Go To Sleep");
    Watch.clearIrqStatus();
    Watch.SetDisplayBrightnessPercent(0);

    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        ShouldWakeUp = false;
        Asleep = true;
        idle_time = 0;
        xSemaphoreGive(Power_Acces_Mutex);
    }

    if (enterLightSleep)
        esp_light_sleep_start();
}

void Power::SetAlarm(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    RTC_DateTime dateTime = Watch.getDateTime();

    RTC_Alarm alarm;
    alarm.hour = hours;
    alarm.minute = minutes;
    alarm.second = seconds;

    if ((hours < dateTime.hour) ||
        (hours == dateTime.hour && minutes < dateTime.minute) ||
        (hours == dateTime.hour && minutes == dateTime.minute && seconds <= dateTime.second))
    {
        alarm.week = (dateTime.week + 1) % 7;
    }
    else
    {
        alarm.week = dateTime.week;
    }

    // Předání alarmu někam dál (pokud bude potřeba)
}

bool Power::ShouldSleep()
{
    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        bool result = idle_time >= sleep_timeout;
        xSemaphoreGive(Power_Acces_Mutex);
        return result;
    }
    return false;
}

void Power::SetWakeupFlag()
{
    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        ShouldWakeUp = true;
        idle_time = 0;
        xSemaphoreGive(Power_Acces_Mutex);
    }
}

WiFiDriver *LilyGoWatch::getWifiDriverRef()
{
    return &wifi_;
}

void Power::SetSleepTimeout(uint32_t timeout)
{
    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        sleep_timeout = timeout;
        xSemaphoreGive(Power_Acces_Mutex);
    }
}

void Power::ResetIdleTime()
{
    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        idle_time = 0;
        xSemaphoreGive(Power_Acces_Mutex);
    }
}

void Power::IncrementIdleTime()
{
    if (xSemaphoreTake(Power_Acces_Mutex, portMAX_DELAY))
    {
        idle_time++;
        xSemaphoreGive(Power_Acces_Mutex);
    }
}

bool Power::IsAsleep() const
{
    return Asleep;
}