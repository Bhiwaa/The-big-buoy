#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

typedef struct {
    float humidity;
    float temperature;
    uint8_t valid;
} DHT22_Data_t;

void DHT22_Init(GPIO_TypeDef *gpio_port, uint16_t gpio_pin);
uint8_t DHT22_Read(DHT22_Data_t *data);
void DHT22_Print(const DHT22_Data_t *data);
void DHT22_Debug(void);

#endif /* DHT22_H */
