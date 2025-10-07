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

void LilyGoWatch::SetWakeupFlag()
{
    Watch.PowerManage.SetWakeupFlag();
    Watch.current_menu = MAIN_FACE;
    Watch.RedrawScreen = true;
}
