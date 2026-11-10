/**
 * @file main.c
 * @brief ESP32 ADC Reading and LDR Lux Calibration (Week 03 Lab)
 * @author Phuphasook (ID: 68030232)
 * @date 2026-11-10
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "LDR_CALIBRATION";

// Hardware Configurations
#define ADC_CHANNEL     ADC1_CHANNEL_6   // GPIO 34 (ADC1 Channel 6)
#define ADC_ATTEN       ADC_ATTEN_DB_11  // Attenuation 11dB (Input Range: ~0 - 3.3V)
#define ADC_WIDTH       ADC_WIDTH_BIT_12 // 12-bit ADC (Resolution: 0 - 4095)
#define NUM_SAMPLES     32               // Multisample Averaging (Oversampling for Noise Reduction)

// ADC Calibration Characteristics handle
static esp_adc_cal_characteristics_t adc_chars;

/**
 * @brief Convert raw ADC reading to Lux using 2nd-degree Polynomial calibration model:
 *        Lux = 0.000132 * ADC^2 - 0.1278 * ADC + 60.11
 *        (Achieved R^2 = 0.9906 from experimental calibration dataset)
 * 
 * @param raw_adc Raw 12-bit ADC value (0 - 4095)
 * @return float Calibrated illuminance in Lux
 */
float adc_to_lux(int raw_adc)
{
    const float a = 0.000132f;
    const float b = -0.1278f;
    const float c = 60.11f;

    float lux = (a * raw_adc * raw_adc) + (b * raw_adc) + c;

    // Physical constraint: Illuminance cannot be negative
    if (lux < 0.0f) {
        lux = 0.0f;
    }
    return lux;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  ESP32 LDR Calibration System (Week 03 Lab)     ");
    ESP_LOGI(TAG, "  Student ID: 68030232 - Phuphasook              ");
    ESP_LOGI(TAG, "=================================================");

    // 1. Configure ADC resolution and channel attenuation
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    // 2. Characterize ADC with eFuse / reference voltage (1100 mV)
    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN,
        ADC_WIDTH,
        1100,
        &adc_chars
    );

    ESP_LOGI(TAG, "ADC Initialized on GPIO34. Starting measurements...\n");

    while (1) {
        // 3. Multisample Averaging (Signal Integrity / Noise Filter)
        uint32_t adc_accum = 0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            adc_accum += adc1_get_raw(ADC_CHANNEL);
        }
        int raw_adc = (int)(adc_accum / NUM_SAMPLES);

        // 4. Convert raw ADC to calibrated voltage (mV)
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(raw_adc, &adc_chars);

        // 5. Convert raw ADC to Lux via Calibration Curve
        float lux = adc_to_lux(raw_adc);

        // 6. Print formatted results to Serial Monitor
        printf("ADC Raw: %4d | Voltage: %4d mV | Calibrated Lux: %7.2f Lux\n",
               raw_adc, voltage_mv, lux);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
