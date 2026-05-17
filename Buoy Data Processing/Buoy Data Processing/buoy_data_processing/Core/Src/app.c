/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app.c
  * @brief   Application layer
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "main.h"

// Re-enable these once the corresponding modules exist:
// #include "imu.h"
// #include "fusion.h"
// #include "nav.h"
// #include "heave.h"

#include <stdio.h>
#include <string.h>

/* External peripheral handles -----------------------------------------------*/
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

/* Private defines -----------------------------------------------------------*/
#define BMP280_ADDR     (0x76 << 1)   // HAL uses 8-bit shifted address
#define BMP280_REG_ID   0xD0
#define BMP280_CHIP_ID  0x58

/* Private variables ---------------------------------------------------------*/
// static fusion_state_t     ori_est;
// static nav_accel_state_t  nav_accel;
// static heave_state_t      heave_state;

/* Private function prototypes -----------------------------------------------*/
static void uart_print(const char *msg);
static void i2c_scan(void);
static void bmp280_check_id(void);

/* ========================================================================= */
/* UART PRINT                                                                 */
/* ========================================================================= */

static void uart_print(const char *msg)
{
    HAL_UART_Transmit(&huart2,
                      (uint8_t*)msg,
                      strlen(msg),
                      HAL_MAX_DELAY);
}

/* ========================================================================= */
/* I2C SCANNER                                                                */
/* ========================================================================= */

static void i2c_scan(void)
{
    char msg[64];
    uint8_t found = 0;

    uart_print("\r\n");
    uart_print("=== I2C scan start ===\r\n");

    for (uint8_t addr = 1; addr < 128; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1,
                                  (uint16_t)(addr << 1),
                                  2,
                                  20) == HAL_OK)
        {
            snprintf(msg, sizeof(msg),
                     "Found device at 0x%02X\r\n", addr);
            uart_print(msg);
            found = 1;
        }
    }

    if (!found)
    {
        uart_print("No I2C devices found\r\n");
    }

    uart_print("=== I2C scan end ===\r\n");
}

/* ========================================================================= */
/* BMP280 CHIP ID CHECK                                                       */
/* ========================================================================= */

static void bmp280_check_id(void)
{
    char msg[64];
    uint8_t chip_id = 0;
    HAL_StatusTypeDef status;

    uart_print("\r\n=== BMP280 chip ID test ===\r\n");

    status = HAL_I2C_IsDeviceReady(&hi2c1, BMP280_ADDR, 3, 100);
    if (status != HAL_OK)
    {
        snprintf(msg, sizeof(msg),
                 "No ACK at 0x76 (HAL=%d)\r\n", (int)status);
        uart_print(msg);
        return;
    }
    uart_print("BMP280 ACK'd at 0x76\r\n");

    status = HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, BMP280_REG_ID,
                              I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100);
    if (status != HAL_OK)
    {
        snprintf(msg, sizeof(msg),
                 "ID read failed (HAL=%d)\r\n", (int)status);
        uart_print(msg);
        return;
    }

    snprintf(msg, sizeof(msg), "Chip ID = 0x%02X ", chip_id);
    uart_print(msg);

    if (chip_id == BMP280_CHIP_ID)
    {
        uart_print("(BMP280 confirmed)\r\n");
    }
    else
    {
        uart_print("(unexpected - expected 0x58)\r\n");
    }
}

/* ========================================================================= */
/* APP INIT                                                                   */
/* ========================================================================= */

void app_init(void)
{
    // imu_init();
    // fusion_init(&ori_est);
    // nav_init(&nav_accel);
    // heave_init(&heave_state);

    uart_print("\r\n");
    uart_print("=================================\r\n");
    uart_print(" buoy_data_processing startup\r\n");
    uart_print("=================================\r\n");

    HAL_Delay(200);

    i2c_scan();
    bmp280_check_id();
}

/* ========================================================================= */
/* APP STEP                                                                   */
/* ========================================================================= */

void app_step(void)
{
    static uint32_t last_tick = 0;

    if (HAL_GetTick() - last_tick >= 1000)
    {
        last_tick = HAL_GetTick();
        uart_print("System running...\r\n");
    }
}
