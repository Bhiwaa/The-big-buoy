/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app.c
  * @brief   Application layer
  ******************************************************************************
  */
/* USER CODE END Header */

#include "app.h"
#include "main.h"
#include "dht22.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

/* =========================================================================
 * DEFINES
 * ========================================================================= */
#define BMP280_ADDR         (0x76 << 1)
#define BMP280_REG_CALIB    0x88
#define BMP280_REG_ID       0xD0
#define BMP280_REG_CTRL     0xF4
#define BMP280_REG_CONFIG   0xF5
#define BMP280_REG_PRESS    0xF7
#define BMP280_CHIP_ID      0x58

#define BNO055_ADDR         (0x28 << 1)
#define BNO055_CHIP_ID      0xA0
#define BNO055_REG_ID       0x00
#define BNO055_OPR_MODE     0x3D
#define BNO055_EULER_H      0x1A
#define BNO055_QUAT_W       0x20

/* =========================================================================
 * TYPES
 * ========================================================================= */
typedef struct {
    uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
    uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3;
    int16_t  dig_P4; int16_t dig_P5; int16_t dig_P6;
    int16_t  dig_P7; int16_t dig_P8; int16_t dig_P9;
} bmp280_calib_t;

/* =========================================================================
 * PRIVATE VARIABLES
 * ========================================================================= */
static bmp280_calib_t bmp_cal;
static int32_t        t_fine;
static DHT22_Data_t   dht22_startup = {0};

/* =========================================================================
 * PRIVATE PROTOTYPES
 * ========================================================================= */
static void uart_print(const char *msg);
static void i2c_scan(void);
static void bmp280_check_id(void);
static void bmp280_read_calibration(void);
static void bmp280_configure(void);
static void bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press);
static float bmp280_comp_temp(int32_t adc_T);
static float bmp280_comp_press(int32_t adc_P);
static void bno055_init(void);
static void app_step_imu_baro(void);
static void app_step_dht22(void);

/* =========================================================================
 * UART
 * ========================================================================= */
static void uart_print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/* =========================================================================
 * I2C SCAN
 * ========================================================================= */
static void i2c_scan(void)
{
    char msg[64];
    uint8_t found = 0;
    uart_print("\r\n=== I2C scan start ===\r\n");
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 2, 20) == HAL_OK) {
            snprintf(msg, sizeof(msg), "Found device at 0x%02X\r\n", addr);
            uart_print(msg);
            found = 1;
        }
    }
    if (!found) uart_print("No I2C devices found\r\n");
    uart_print("=== I2C scan end ===\r\n");
}

/* =========================================================================
 * BMP280
 * ========================================================================= */
static void bmp280_check_id(void)
{
    char msg[64];
    uint8_t chip_id = 0;
    uart_print("\r\n=== BMP280 chip ID test ===\r\n");
    if (HAL_I2C_IsDeviceReady(&hi2c1, BMP280_ADDR, 3, 100) != HAL_OK) {
        uart_print("No ACK at 0x76\r\n"); return;
    }
    uart_print("BMP280 ACK'd\r\n");
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, BMP280_REG_ID,
                     I2C_MEMADD_SIZE_8BIT, &chip_id, 1, 100);
    snprintf(msg, sizeof(msg), "Chip ID = 0x%02X %s\r\n", chip_id,
             chip_id == BMP280_CHIP_ID ? "(confirmed)" : "(unexpected)");
    uart_print(msg);
}

static void bmp280_read_calibration(void)
{
    uint8_t calib[24];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, BMP280_REG_CALIB,
                     I2C_MEMADD_SIZE_8BIT, calib, 24, HAL_MAX_DELAY);
    bmp_cal.dig_T1 = (uint16_t)(calib[1]  << 8 | calib[0]);
    bmp_cal.dig_T2 = (int16_t) (calib[3]  << 8 | calib[2]);
    bmp_cal.dig_T3 = (int16_t) (calib[5]  << 8 | calib[4]);
    bmp_cal.dig_P1 = (uint16_t)(calib[7]  << 8 | calib[6]);
    bmp_cal.dig_P2 = (int16_t) (calib[9]  << 8 | calib[8]);
    bmp_cal.dig_P3 = (int16_t) (calib[11] << 8 | calib[10]);
    bmp_cal.dig_P4 = (int16_t) (calib[13] << 8 | calib[12]);
    bmp_cal.dig_P5 = (int16_t) (calib[15] << 8 | calib[14]);
    bmp_cal.dig_P6 = (int16_t) (calib[17] << 8 | calib[16]);
    bmp_cal.dig_P7 = (int16_t) (calib[19] << 8 | calib[18]);
    bmp_cal.dig_P8 = (int16_t) (calib[21] << 8 | calib[20]);
    bmp_cal.dig_P9 = (int16_t) (calib[23] << 8 | calib[22]);
    char msg[128];
    snprintf(msg, sizeof(msg), "Cal: T1=%u T2=%d T3=%d P1=%u P2=%d\r\n",
             bmp_cal.dig_T1, bmp_cal.dig_T2, bmp_cal.dig_T3,
             bmp_cal.dig_P1, bmp_cal.dig_P2);
    uart_print(msg);
    uart_print("BMP280 calibration loaded\r\n");
}

static void bmp280_configure(void)
{
    uint8_t reg;
    reg = 0x27; /* osrs_t=x1, osrs_p=x1, normal mode */
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR, BMP280_REG_CTRL,
                      I2C_MEMADD_SIZE_8BIT, &reg, 1, HAL_MAX_DELAY);
    reg = 0x10; /* t_sb=0.5ms, filter=x16 */
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR, BMP280_REG_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &reg, 1, HAL_MAX_DELAY);
    uart_print("BMP280 configured\r\n");
}

static void bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press)
{
    uint8_t data[6];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR, BMP280_REG_PRESS,
                     I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);
    *raw_press = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (data[2] >> 4);
    *raw_temp  = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | (data[5] >> 4);
}

static float bmp280_comp_temp(int32_t adc_T)
{
    float var1 = (((float)adc_T / 16384.0f) - ((float)bmp_cal.dig_T1 / 1024.0f))
                 * (float)bmp_cal.dig_T2;
    float var2 = ((((float)adc_T / 131072.0f) - ((float)bmp_cal.dig_T1 / 8192.0f)) *
                  (((float)adc_T / 131072.0f) - ((float)bmp_cal.dig_T1 / 8192.0f)))
                 * (float)bmp_cal.dig_T3;
    t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0f;
}

static float bmp280_comp_press(int32_t adc_P)
{
    float var1 = ((float)t_fine / 2.0f) - 64000.0f;
    float var2 = var1 * var1 * (float)bmp_cal.dig_P6 / 32768.0f;
    var2 = var2 + var1 * (float)bmp_cal.dig_P5 * 2.0f;
    var2 = (var2 / 4.0f) + ((float)bmp_cal.dig_P4 * 65536.0f);
    var1 = (((float)bmp_cal.dig_P3 * var1 * var1 / 524288.0f)
            + ((float)bmp_cal.dig_P2 * var1)) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * (float)bmp_cal.dig_P1;
    if (var1 == 0.0f) return 0.0f;
    float p = 1048576.0f - (float)adc_P;
    p = (p - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = (float)bmp_cal.dig_P9 * p * p / 2147483648.0f;
    var2 = p * (float)bmp_cal.dig_P8 / 32768.0f;
    return (p + (var1 + var2 + (float)bmp_cal.dig_P7) / 16.0f) / 100.0f;
}

/* =========================================================================
 * BNO055
 * ========================================================================= */
static void bno055_init(void)
{
    uint8_t id = 0;
    char msg[64];
    uart_print("\r\n=== BNO055 init ===\r\n");
    HAL_Delay(700);
    HAL_I2C_Mem_Read(&hi2c1, BNO055_ADDR, BNO055_REG_ID,
                     I2C_MEMADD_SIZE_8BIT, &id, 1, 100);
    snprintf(msg, sizeof(msg), "BNO055 ID: 0x%02X\r\n", id);
    uart_print(msg);
    if (id != BNO055_CHIP_ID) { uart_print("BNO055 not found!\r\n"); return; }
    uart_print("BNO055 detected\r\n");
    uint8_t mode = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, BNO055_ADDR, BNO055_OPR_MODE,
                      I2C_MEMADD_SIZE_8BIT, &mode, 1, 100);
    HAL_Delay(25);
    mode = 0x0C;
    HAL_I2C_Mem_Write(&hi2c1, BNO055_ADDR, BNO055_OPR_MODE,
                      I2C_MEMADD_SIZE_8BIT, &mode, 1, 100);
    HAL_Delay(500);   /* wait for fusion to start */
    uart_print("BNO055 set to NDOF mode\r\n");
}

/* =========================================================================
 * APP STEP HELPERS
 * ========================================================================= */
static void app_step_imu_baro(void)
{
    /* Read raw accelerometer, magnetometer, gyroscope */
    uint8_t acc_buf[6], mag_buf[6], gyr_buf[6];
    uint8_t euler_buf[6];

    HAL_I2C_Mem_Read(&hi2c1, BNO055_ADDR, 0x08,
                     I2C_MEMADD_SIZE_8BIT, acc_buf, 6, 100);
    HAL_I2C_Mem_Read(&hi2c1, BNO055_ADDR, 0x0E,
                     I2C_MEMADD_SIZE_8BIT, mag_buf, 6, 100);
    HAL_I2C_Mem_Read(&hi2c1, BNO055_ADDR, 0x14,
                     I2C_MEMADD_SIZE_8BIT, gyr_buf, 6, 100);
    HAL_I2C_Mem_Read(&hi2c1, BNO055_ADDR, BNO055_EULER_H,
                     I2C_MEMADD_SIZE_8BIT, euler_buf, 6, 100);

    /* Parse raw */
    int16_t ax = ((int16_t)acc_buf[1] << 8) | acc_buf[0];
    int16_t ay = ((int16_t)acc_buf[3] << 8) | acc_buf[2];
    int16_t az = ((int16_t)acc_buf[5] << 8) | acc_buf[4];

    int16_t mx = ((int16_t)mag_buf[1] << 8) | mag_buf[0];
    int16_t my = ((int16_t)mag_buf[3] << 8) | mag_buf[2];
    int16_t mz = ((int16_t)mag_buf[5] << 8) | mag_buf[4];

    int16_t gx = ((int16_t)gyr_buf[1] << 8) | gyr_buf[0];
    int16_t gy = ((int16_t)gyr_buf[3] << 8) | gyr_buf[2];
    int16_t gz = ((int16_t)gyr_buf[5] << 8) | gyr_buf[4];

    /* Parse fused Euler */
    uint16_t heading = ((uint16_t)euler_buf[1] << 8) | euler_buf[0];
    int16_t  roll    = ((int16_t) euler_buf[3] << 8) | euler_buf[2];
    int16_t  pitch   = ((int16_t) euler_buf[5] << 8) | euler_buf[4];

    /* CSV: timestamp, ax,ay,az, mx,my,mz, gx,gy,gz, heading,roll,pitch */
    char msg[256];
   snprintf(msg, sizeof(msg),
             "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",
             HAL_GetTick(),
             ax/100.0f, ay/100.0f, az/100.0f,
             mx/16.0f,  my/16.0f,  mz/16.0f,
             gx/16.0f,  gy/16.0f,  gz/16.0f,
             heading/16.0f, roll/16.0f, pitch/16.0f);
    uart_print(msg);
}

static void app_step_dht22(void)
{
    DHT22_Data_t data;
    char msg[64];
    if (DHT22_Read(&data)) {
        snprintf(msg, sizeof(msg), "DHT22: T=%.1f C | RH=%.1f%%\r\n",
                 data.temperature, data.humidity);
    } else {
        snprintf(msg, sizeof(msg), "DHT22: read failed\r\n");
    }
    uart_print(msg);
}
/* =========================================================================
 * APP INIT
 * ========================================================================= */
void app_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    uart_print("\r\n=================================\r\n");
    uart_print(" buoy_data_processing startup\r\n");
    uart_print("=================================\r\n");

    HAL_Delay(200);

    i2c_scan();
    bmp280_check_id();
    bmp280_configure();
    HAL_Delay(100);
    bmp280_read_calibration();
    bno055_init();

    DHT22_Init(GPIOA, GPIO_PIN_0);
    HAL_Delay(3000);
    DHT22_Read(&dht22_startup);
    char dht_msg[64];
    if (dht22_startup.valid)
        snprintf(dht_msg, sizeof(dht_msg), "DHT22: T=%.1f C | RH=%.1f%%\r\n",
                 dht22_startup.temperature, dht22_startup.humidity);
    else
        snprintf(dht_msg, sizeof(dht_msg), "DHT22: startup read failed\r\n");
    uart_print(dht_msg);
   uart_print("timestamp_ms,ax,ay,az,mx,my,mz,gx,gy,gz,heading,roll,pitch\r\n");
}

/* =========================================================================
 * APP STEP
 * ========================================================================= */
void app_step(void)
{
    static uint32_t imu_tick  = 0;
    static uint32_t dht22_tick = 0;
    uint32_t now = HAL_GetTick();

    if (now - imu_tick >= 100) {
        imu_tick = now;
        app_step_imu_baro();
    }

    if (now - dht22_tick >= 2500) {
        dht22_tick = now;
        app_step_dht22();
    }
}
