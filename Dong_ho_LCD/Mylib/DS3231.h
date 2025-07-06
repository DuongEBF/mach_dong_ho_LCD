#ifndef DS3231_H
#define DS3231_H


#include "stdint.h"
#include "main.h"
//extern I2C_HandleTypeDef hi2c1; //day la cach 1
static I2C_HandleTypeDef* ds_i2c1; //cach 2
typedef struct
{
	int8_t second;
	int8_t minute;
	int8_t hour;
	int8_t day;
	int8_t date;
	int8_t month;
	int8_t year;
}Date_time_t;
#define RTC_ADDR (0x68<<1)
#define ADDR_RES 0x00
void rtc_write_time(Date_time_t* dt);
void rtc_read_time(Date_time_t* dt);
uint8_t rtc_read_temp(Date_time_t* dt);
void DS3231_Init(I2C_HandleTypeDef* i2c1); //cach 2
uint8_t rtc_read_day_of_week(Date_time_t* dt);
void ds3231_write_ram(uint8_t address, uint8_t data);
uint8_t ds3231_read_ram(uint8_t address);
#endif //DS3231_H