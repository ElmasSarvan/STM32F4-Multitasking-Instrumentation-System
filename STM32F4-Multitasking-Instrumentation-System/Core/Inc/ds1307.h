/*******************************************************************************
* Dosya          : ds1307.h
* Amaç           : Yazılımsal I2C ile DS1307 RTC Kontrolü (Hocanın Orijinal Şablonu)
* Bağlantılar    : SCL -> PB6  |  SDA -> PB7
*******************************************************************************/

#ifndef __DS1307_H
#define __DS1307_H

#include "main.h"

extern void delay_us (uint16_t delay);
extern uint8_t sn, dak, saat, gun, haftagun, ay, yil;

static GPIO_TypeDef* GPIO_RTC = GPIOB;

#define SCL GPIO_PIN_6
#define SDA GPIO_PIN_7

#define I2C_DELAY 10

static inline void i2cStart(void)
{
    GPIO_RTC->BSRR=SDA;
    GPIO_RTC->BSRR=SCL;
    delay_us(I2C_DELAY);
    GPIO_RTC->BSRR=SDA<<16;
    delay_us(I2C_DELAY);
    GPIO_RTC->BSRR=SCL<<16;
    delay_us(I2C_DELAY);
}

static inline void i2cStop(void)
{
    GPIO_RTC->BSRR=SDA<<16;
    GPIO_RTC->BSRR=SCL<<16;
    delay_us(I2C_DELAY);
    GPIO_RTC->BSRR=SCL;
    delay_us(I2C_DELAY);
    GPIO_RTC->BSRR=SDA;
    delay_us(I2C_DELAY);
}

static inline uint8_t i2cwrite(uint8_t veri)
{
    uint8_t ack, j;
    for (j=0; j<8; j++) {
        if((veri & 0x80)==0x80)
            GPIO_RTC->BSRR=SDA;
        else
            GPIO_RTC->BSRR=SDA<<16;

        GPIO_RTC->BSRR=SCL;
        delay_us(I2C_DELAY);
        GPIO_RTC->BSRR=SCL<<16;
        delay_us(I2C_DELAY);

        veri=veri<<1;
    }

    GPIO_RTC->BSRR=SCL;
    delay_us(I2C_DELAY);
    ack=(GPIO_RTC->IDR & SDA)>>7;
    GPIO_RTC->BSRR=SCL<<16;
    delay_us(I2C_DELAY);
    return ack;
}

static inline uint8_t i2cread(uint8_t ack)
{
   uint8_t veri = 0;
   for (uint8_t i = 0; i < 8; i++) {
        veri <<= 1;
        delay_us(I2C_DELAY);
        GPIO_RTC->BSRR=SCL;
        if((GPIO_RTC->IDR & SDA) !=0) veri |= 1;
        GPIO_RTC->BSRR=SCL<<16;
    }

   if(ack==0) GPIO_RTC->BSRR=SDA<<16;
   else GPIO_RTC->BSRR=SDA;
   delay_us(I2C_DELAY);
   GPIO_RTC->BSRR=SCL;
   delay_us(I2C_DELAY);
   GPIO_RTC->BSRR=SCL<<16;
   delay_us(I2C_DELAY);
   GPIO_RTC->BSRR=SDA;
   return veri;
}

static inline uint8_t decimaltobcd(uint8_t dec) {
    return (dec % 10 + ((dec / 10) << 4));
}

static inline uint8_t bcdtodecimal(uint8_t bcd) {
    return (((bcd & 0xf0) >> 4) * 10) + (bcd & 0x0f);
}

static inline void SetDateTime_ds1307(void)
{
     uint8_t s_bcd = decimaltobcd(sn);
     uint8_t d_bcd = decimaltobcd(dak);
     uint8_t st_bcd = decimaltobcd(saat);
     uint8_t h_bcd = decimaltobcd(haftagun);
     uint8_t g_bcd = decimaltobcd(gun);
     uint8_t a_bcd = decimaltobcd(ay);
     uint8_t y_bcd = decimaltobcd(yil);

    i2cStart();
    i2cwrite(0xD0);
    i2cwrite(0x00);
    i2cwrite(s_bcd);
    i2cwrite(d_bcd);
    i2cwrite(st_bcd);
    i2cwrite(h_bcd);
    i2cwrite(g_bcd);
    i2cwrite(a_bcd);
    i2cwrite(y_bcd);
    i2cStop();
}

static inline void GetDateTime_ds1307(void)
{
    i2cStart();
    i2cwrite(0xD0);
    i2cwrite(0x00);
    i2cStart();
    i2cwrite(0xD1);

    sn=i2cread(0);
    dak=i2cread(0);
    saat=i2cread(0);
    haftagun=i2cread(0);
    gun=i2cread(0);
    ay=i2cread(0);
    yil=i2cread(1);
    i2cStop();

    sn=bcdtodecimal(sn);
    dak=bcdtodecimal(dak);
    saat=bcdtodecimal(saat);
    haftagun=bcdtodecimal(haftagun);
    gun=bcdtodecimal(gun);
    ay=bcdtodecimal(ay);
    yil=bcdtodecimal(yil);
}
#endif
