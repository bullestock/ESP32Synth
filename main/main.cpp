#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void startAsmSynth(void);

extern "C" void app_main(void){
    ESP_LOGI("main","Starting AsmSynth example");
    startAsmSynth();
    // Keep the main task alive
    while(true){ vTaskDelay(pdMS_TO_TICKS(10000)); }
}
