#include "DS3231.h"
static uint8_t Decimal2BCD(uint8_t num)
{
	return (num/10)<<4|(num%10);
}

static uint8_t BCD2Decimal(uint8_t num)
{
	return (num>>4)*10+(num&0x0F);
}
void DS3231_Init(I2C_HandleTypeDef* i2c1)
{
	ds_i2c1 = i2c1;
}
void rtc_write_time(Date_time_t* dt)
{
	uint8_t data[8];
	data[0] = ADDR_RES;
	data[1] = Decimal2BCD(dt->second);
	data[2] = Decimal2BCD(dt->minute);
	data[3] = Decimal2BCD(dt->hour);
	data[4] = Decimal2BCD(dt->day);
	data[5] = Decimal2BCD(dt->date);
	data[6] = Decimal2BCD(dt->month);
	data[7] = Decimal2BCD(dt->year);
	HAL_I2C_Master_Transmit(ds_i2c1,RTC_ADDR,data,8,100);
}
void rtc_read_time(Date_time_t* dt)
{
	uint8_t buff_Data[7];
	uint8_t add_res = ADDR_RES;
	HAL_I2C_Master_Transmit(ds_i2c1,RTC_ADDR,&add_res,1,100);
	HAL_I2C_Master_Receive(ds_i2c1,RTC_ADDR,buff_Data,7,100);
	dt->second = BCD2Decimal(buff_Data[0]);
	dt->minute = BCD2Decimal(buff_Data[1]);
	dt->hour =   BCD2Decimal(buff_Data[2]);
	dt->day =    BCD2Decimal(buff_Data[3]);
	dt->date =   BCD2Decimal(buff_Data[4]);
	dt->month =  BCD2Decimal(buff_Data[5]);
	dt->year =   BCD2Decimal(buff_Data[6]);
}

uint8_t rtc_read_temp(Date_time_t* dt)
{
	//uint16_t temp;
	uint8_t buff_Data_temp[2];
	uint8_t add_res = 0x11;
	HAL_I2C_Master_Transmit(ds_i2c1,RTC_ADDR,&add_res,1,100);
	HAL_I2C_Master_Receive(ds_i2c1,RTC_ADDR,buff_Data_temp,2,100);
	//temp = ((uint16_t)buff_Data_temp[0] << 2)|(buff_Data_temp[1] >> 6);
	return buff_Data_temp[0];
}

uint8_t rtc_read_day_of_week(Date_time_t* dt)
{
	uint16_t d = dt->date;
	uint16_t m = dt->month;
	uint16_t y = 2000 + dt->year;
	uint8_t weekday  = (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7;
	return weekday;
}