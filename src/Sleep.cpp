#include "LilyGoWatch.h"

void LilyGoWatch::HandleSleepActions()
{
    PowerManage.HandleSleepActions(DisplayBrightness, (Watch.Globals.wifi_mode == WIFI_MODE_NULL));
}

void LilyGoWatch::WakeUp()
{
    PowerManage.WakeUpFromSleep(DisplayBrightness);

    Watch.RedrawScreen = true;
    Watch.current_menu = MAIN_FACE;
    Watch.UpdateScreen();
    Serial.println("Wake Up");
}

void LilyGoWatch::GoToSleep()
{
    PowerManage.GoToSleep((Watch.Globals.wifi_mode == WIFI_MODE_NULL));
}

void IRAM_ATTR LilyGoWatch::SetWakeupFlag()
{
    // ISR context: direct volatile writes only, no RTOS calls
    Watch.PowerManage.ShouldWakeUp = true;
    Watch.PowerManage.idle_time = 0;
    Watch.current_menu = MAIN_FACE;
    Watch.RedrawScreen = true;
}
