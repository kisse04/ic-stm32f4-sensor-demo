#include "bmp280.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal_i2c.h"

#define BMP280_ADDR_7BIT      (0x76)
#define BMP280_ADDR           (BMP280_ADDR_7BIT << 1)   // HAL needs 8-bit address

#define BMP280_REG_ID         (0xD0)
#define BMP280_REG_RESET      (0xE0)
#define BMP280_REG_STATUS     (0xF3)
#define BMP280_REG_CTRL_MEAS  (0xF4)
#define BMP280_REG_CONFIG     (0xF5)

#define BMP280_REG_TEMP_MSB   (0xFA)   // 0xFA..0xFC
#define BMP280_REG_CALIB00    (0x88)

static I2C_HandleTypeDef *s_hi2c = NULL;

// temperature calibration
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;
static int32_t  t_fine;

static int read_bytes(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return (HAL_I2C_Mem_Read(s_hi2c, BMP280_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK) ? 0 : -1;
}

static int write_byte(uint8_t reg, uint8_t val)
{
    return (HAL_I2C_Mem_Write(s_hi2c, BMP280_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100) == HAL_OK) ? 0 : -1;
}

static int read_u8(uint8_t reg, uint8_t *val)
{
    return read_bytes(reg, val, 1);
}
/*
void BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;

    // (optional) soft reset
    (void)write_byte(BMP280_REG_RESET, 0xB6);
    HAL_Delay(10);

    // Optional: check chip id = 0x58
    // uint8_t id=0; read_u8(BMP280_REG_ID,&id);

    // IMPORTANT: set sleep mode before writing config (datasheet says writes in normal may be ignored)
    // ctrl_meas: mode = sleep (00), keep osrs as 0 for now
    (void)write_byte(BMP280_REG_CTRL_MEAS, 0x00);

    // read calibration (24 bytes; we only use first 6 for temperature)
    uint8_t calib[24];
    if (read_bytes(BMP280_REG_CALIB00, calib, sizeof(calib)) == 0) {
        dig_T1 = (uint16_t)((uint16_t)calib[1] << 8 | calib[0]);
        dig_T2 = (int16_t)((uint16_t)calib[3] << 8 | calib[2]);
        dig_T3 = (int16_t)((uint16_t)calib[5] << 8 | calib[4]);
    }

    // config: t_sb[7:5], filter[4:2], spi3w_en[0]
    // 對 forced mode 其實 t_sb 不影響；這裡保留你的設定也OK
    (void)write_byte(BMP280_REG_CONFIG, (uint8_t)(0x04 << 5)); // t_sb=500ms, filter=off

    // 先把 ctrl_meas 設成「溫度 oversampling x1、壓力 skipped、sleep」
    // 之後每次讀溫度用 forced 觸發一次（見 BMP280_TriggerAndWait）
    //原始方案
    //(void)write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01 << 5) | (0x00 << 2) | 0x00));


    //debug 方案A 
    //ctrl_meas: osrs_t=1, osrs_p=1(隨便給1，不要skip), mode=normal(11)
    //(void)write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01<<5) | (0x01<<2) | 0x03));

    //debug 方案B 
    // ctrl_meas: osrs_t=1, osrs_p=1(建議別skip), mode=sleep(00)
    write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01<<5) | (0x01<<2) | 0x00));

    
    //debug start
    //uint8_t id = 0;
    //read_u8(BMP280_REG_ID, &id);
    //printf("BMP/BME ID=0x%02X\r\n", id);
    //debug end


}
*/

//debug start
void BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;

    (void)write_byte(BMP280_REG_RESET, 0xB6);
    HAL_Delay(10);

    uint8_t id=0;
    read_u8(BMP280_REG_ID, &id);
    printf("BMP/BME ID=0x%02X\r\n", id);

    // 先進 sleep，避免 config 寫入被忽略
    (void)write_byte(BMP280_REG_CTRL_MEAS, 0x00);

    // 讀校正參數（同你原本）
    uint8_t calib[24];
    if (read_bytes(BMP280_REG_CALIB00, calib, sizeof(calib)) == 0) {
        dig_T1 = (uint16_t)((uint16_t)calib[1] << 8 | calib[0]);
        dig_T2 = (int16_t)((uint16_t)calib[3] << 8 | calib[2]);
        dig_T3 = (int16_t)((uint16_t)calib[5] << 8 | calib[4]);
    }

    // config：t_sb=62.5ms(001), filter off
    (void)write_byte(BMP280_REG_CONFIG, (uint8_t)(0x01 << 5));

    // ctrl_meas：osrs_t=1, osrs_p=1, normal mode(11)
    (void)write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01<<5) | (0x01<<2) | 0x03));

    // 等第一次量測完成（避免第一次讀到 0）
    uint8_t st=0;
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 200) {
        if (read_u8(BMP280_REG_STATUS, &st) != 0) break;
        if ((st & (1U<<3)) == 0 && (st & 1U) == 0) { // measuring=0 & im_update=0
            // 再多等一點點讓 output 穩定
            HAL_Delay(5);
            break;
        }
        HAL_Delay(1);
    }
}


//debug end



static int BMP280_TriggerAndWait(uint32_t timeout_ms)
{
    // 先確保 sleep
    if (write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01 << 5) | (0x00 << 2) | 0x00)) != 0)
        return -1;

    // 再切 forced（觸發一次量測）
    if (write_byte(BMP280_REG_CTRL_MEAS, (uint8_t)((0x01 << 5) | (0x00 << 2) | 0x01)) != 0)
        return -1;

    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms) {
        uint8_t st = 0;
        if (read_u8(BMP280_REG_STATUS, &st) != 0) return -1;

        // measuring(bit3)=0 且 im_update(bit0)=0 才算完成
        if ((st & (1U << 3)) == 0 && (st & 1U) == 0) return 0;

        HAL_Delay(1);
    }

    return -2;
}

/*
int32_t BMP280_ReadTemperature(void)
{
    if (s_hi2c == NULL) return 0;

    if (BMP280_TriggerAndWait(50) != 0)
        return 0;

    //dubug start
    //debug part 1
    uint8_t cm = 0;
    read_u8(BMP280_REG_CTRL_MEAS, &cm);
    printf("ctrl_meas=0x%02X\r\n", cm);
    //debug end


    // IMPORTANT: burst read ALL data registers 0xF7..0xFC (8 bytes)
    uint8_t d[6];
    if (read_bytes(0xF7, d, 6) != 0)
        return 0;

    //debug part 2
    //uint8_t d[8];
    //if (read_bytes(0xF7, d, 8) != 0)
    //    return 0;

    printf("raw8: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
           d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]);

    //int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)d[5] >> 4);
    //debug end



    // temp bytes are at 0xFA..0xFC => d[3], d[4], d[5]
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)d[5] >> 4);

    //debug part 3
    printf("adc_T=%ld\r\n", adc_T);
    //debug end


    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8; // 0.01 C
}

*/

int32_t BMP280_ReadTemperature(void)
{

uint8_t cm=0, st=0;
read_u8(BMP280_REG_CTRL_MEAS, &cm);
read_u8(BMP280_REG_STATUS, &st);
printf("cm=0x%02X st=0x%02X\r\n", cm, st);

    uint8_t d[6];
    if (read_bytes(0xF7, d, 6) != 0) return 0;

    printf("raw6: %02X %02X %02X %02X %02X %02X\r\n",
       d[0], d[1], d[2], d[3], d[4], d[5]);

    uint8_t t3[3];
    read_bytes(0xFA, t3, 3);
    printf("temp3: %02X %02X %02X\r\n", t3[0], t3[1], t3[2]);
    

    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)d[5] >> 4);

    // 如果讀到 0（你現在的問題），做一次 retry（很多時序問題直接被吃掉）
    if (adc_T == 0) {
        HAL_Delay(5);
        if (read_bytes(0xF7, d, 6) != 0) return 0;
        adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)d[5] >> 4);
    }

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;

    return (t_fine * 5 + 128) >> 8; // 0.01 C
}




/*
int32_t BMP280_ReadTemperature(void)
{
    uint8_t d[8];
    if (read_bytes(0xF7, d, 8) != 0) return 0;

    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)d[5] >> 4);

    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8; // 0.01C
}*/