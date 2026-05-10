#include "activity_manager.h"
#include "esp_log.h"
#include "scheduler.h"

static const char *TAG = "activity_manager";

void activity_manager_init(void)
{
    ESP_LOGI(TAG, "Activity manager started");
}

void activity_manager_execute(int id)
{
    ESP_LOGI(TAG, "Executing activity %d", id);
}

bool activity_manager_trigger(int id)
{
    event_t event = {
        .type = EVENT_ACTIVITY,
        .id = id,
        .data = NULL
    };
    return scheduler_post_event(&event);
}
