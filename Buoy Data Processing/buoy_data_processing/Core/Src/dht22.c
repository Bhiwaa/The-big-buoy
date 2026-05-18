#include "dht22.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

static GPIO_TypeDef *dht22_gpio = NULL;
static uint16_t dht22_pin = 0;
static uint32_t last_read_time = 0;

static void dht22_delay_us(uint32_t us) {
    uint32_t cycles = us * 4;
    while (cycles--) { __NOP(); }
}

static uint8_t dht22_read_pin(void) {
    return HAL_GPIO_ReadPin(dht22_gpio, dht22_pin);
}
static void pin_output_low(void)
{
    /* Drive PA0 low */
    GPIOA->BRR = GPIO_PIN_0;
    /* Set PA0 as output: MODER bits [1:0] = 01 */
    GPIOA->MODER = (GPIOA->MODER & ~(3U)) | (1U);
    /* Push-pull: OTYPER bit 0 = 0 */
    GPIOA->OTYPER &= ~(1U);
    /* High speed: OSPEEDR bits [1:0] = 11 */
    GPIOA->OSPEEDR |= (3U);
    /* No pull: PUPDR bits [1:0] = 00 */
    GPIOA->PUPDR &= ~(3U);
}

static void pin_input(void)
{
    /* Set PA0 as input: MODER bits [1:0] = 00 */
    GPIOA->MODER &= ~(3U);
    /* Pull-up: PUPDR bits [1:0] = 01 */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U)) | (1U);
}
void DHT22_Init(GPIO_TypeDef *gpio_port, uint16_t gpio_pin) {
    dht22_gpio = gpio_port;
    dht22_pin = gpio_pin;
    last_read_time = 0;
    pin_input();
}

uint8_t DHT22_Read(DHT22_Data_t *data) {
    uint8_t bits[5] = {0};
    uint32_t timeout;
    uint8_t i, j;

    if (data == NULL) return 0;

    if (HAL_GetTick() - last_read_time < 2000) return 0;

    /* Send start signal */
    pin_output_low();
    HAL_Delay(2);
    pin_input();
    dht22_delay_us(30);

    /* Wait for sensor response low */
    timeout = 200;
    while (dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) return 0;

    /* Wait for sensor response high */
    timeout = 200;
    while (!dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) return 0;

    /* Wait for sensor to pull low before data */
    timeout = 200;
    while (dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) return 0;

    /* Read 40 bits */
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            /* Wait for line to go high */
            timeout = 100;
            while (!dht22_read_pin() && timeout--) dht22_delay_us(1);
            if (timeout == 0) return 0;

            /* Measure high time */
            uint32_t high_time = 0;
            while (dht22_read_pin() && high_time < 100) {
                high_time++;
                dht22_delay_us(1);
            }

            bits[i] <<= 1;
            if (high_time > 10) bits[i] |= 1;
        }
    }

    /* Verify checksum */
    if (((bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF) != bits[4]) {
        data->valid = 0;
        return 0;
    }

    /* Parse */
    uint16_t raw_hum = ((uint16_t)bits[0] << 8) | bits[1];
    uint16_t raw_temp = ((uint16_t)bits[2] << 8) | bits[3];
    int16_t temp_signed = (raw_temp & 0x8000) ? -(raw_temp & 0x7FFF) : raw_temp;

    data->humidity    = raw_hum * 0.1f;
    data->temperature = temp_signed * 0.1f;
    data->valid = 1;

    last_read_time = HAL_GetTick();
    return 1;
}

void DHT22_Print(const DHT22_Data_t *data) {
    char buf[64];
    if (data == NULL) return;
    if (data->valid)
        snprintf(buf, sizeof(buf), "DHT22: T=%.1f C | RH=%.1f%%\r\n",
                 data->temperature, data->humidity);
  //  else
       // snprintf(buf, sizeof(buf), "DHT22: Read failed\r\n");
   // HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);


}

void DHT22_Debug(void) {
    char buf[32];
    uint32_t timeout;
    uint32_t times[40] = {0};

    /* Send start signal */
    pin_output_low();
    HAL_Delay(2);
    pin_input();
    dht22_delay_us(30);

    /* Wait for response */
    timeout = 200;
    while (dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"No response\r\n", 13, 100);
        return;
    }
    timeout = 200;
    while (!dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Stuck low\r\n", 11, 100);
        return;
    }
    timeout = 200;
    while (dht22_read_pin() && timeout--) dht22_delay_us(1);
    if (timeout == 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Stuck high\r\n", 12, 100);
        return;
    }

    /* Read 40 bit timings */
    for (int i = 0; i < 40; i++) {
        timeout = 200;
        while (!dht22_read_pin() && timeout--) dht22_delay_us(1);
        uint32_t t = 0;
        while (dht22_read_pin() && t < 200) { t++; dht22_delay_us(1); }
        times[i] = t;
    }

    /* Print all 40 timings */
    HAL_UART_Transmit(&huart2, (uint8_t*)"Bit times:\r\n", 12, 100);
    for (int i = 0; i < 40; i++) {
        snprintf(buf, sizeof(buf), "%2d: %3lu\r\n", i, times[i]);
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
    }
}
