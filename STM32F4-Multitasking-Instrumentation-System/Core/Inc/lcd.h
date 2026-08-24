/**********************************************
* Dosya          : lcd.h
* Amaç           : Hız Filtreli LCD Sürücüsü
************************************************/

#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include <stdio.h>

/* LCD KONTROL PİNLERİ (GPIOB) */
#define LCD_RS_PORT   GPIOB
#define LCD_EN_PORT   GPIOB
#define LCD_RS_PIN    GPIO_PIN_9
#define LCD_EN_PIN    GPIO_PIN_8

/* LCD DATA PİNLERİ PORT VE PİN HARİTASI */
#define LCD_D4_PORT   GPIOB
#define LCD_D4_PIN    GPIO_PIN_4

#define LCD_D5_PORT   GPIOB
#define LCD_D5_PIN    GPIO_PIN_5

#define LCD_D6_PORT   GPIOB
#define LCD_D6_PIN    GPIO_PIN_3

#define LCD_D7_PORT   GPIOA
#define LCD_D7_PIN    GPIO_PIN_8

// Mikro saniye gecikmesini işlemci hızına göre esnettik
static inline void delay_us_lcd(uint16_t n)
{
    volatile uint16_t j;
    while(n--)
    {
        for(j=0; j<35; j++); // Filtre artırıldı
    }
}

static inline void clock_lcd(void)
{
  HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_SET);
  delay_us_lcd(50);
  HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_RESET);
  delay_us_lcd(50);
}

static inline void yuklelcd(uint8_t deger)
{
  // LCD'nin komutu yakalaması için bekleme süresini 10ms yaptık (Kritik Hamle!)
  HAL_Delay(10);

  if((deger & 0x10)==0x10) HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4_PIN, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4_PIN, GPIO_PIN_RESET);

  if((deger & 0x20)==0x20) HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5_PIN, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5_PIN, GPIO_PIN_RESET);

  if((deger & 0x40)==0x40) HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6_PIN, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6_PIN, GPIO_PIN_RESET);

  if((deger & 0x80)==0x80) HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7_PIN, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7_PIN, GPIO_PIN_RESET);

  HAL_Delay(10);
}

static inline void sendCmdLcd(uint16_t cmd)
{
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
  yuklelcd(cmd);
  clock_lcd();
  yuklelcd(cmd<<4);
  clock_lcd();
  HAL_Delay(10);
}

static inline void sendDataLcd(uint16_t veri)
{
  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET);
  yuklelcd(veri);
  clock_lcd();
  yuklelcd(veri<<4);
  clock_lcd();
  HAL_Delay(5);
}

static inline void writeStr(uint16_t sat, uint16_t sut, char * str)
{
    uint16_t adr=0;
    if(sat==0) adr=0x80+sut;
    if(sat==1) adr=0xC0+sut;
    if(sat==2) adr=0x94+sut;
    if(sat==3) adr=0xD4+sut;
    sendCmdLcd(adr);
    while(*str!=0){
        sendDataLcd(*str++);
    }
}

static inline void clearLcd(void)
{
   sendCmdLcd(0x01);
   HAL_Delay(10);
}

static inline void lcdInit(void)
{
  HAL_Delay(100); // İlk uyanış süresi uzatıldı
  sendCmdLcd(0x02);
  HAL_Delay(20);
  sendCmdLcd(0x28);
  HAL_Delay(20);
  sendCmdLcd(0x06);
  HAL_Delay(20);
  sendCmdLcd(0x0C);
  HAL_Delay(20);
  sendCmdLcd(0x01);
  HAL_Delay(20);
}

static inline void displayFloat(uint16_t sat, uint16_t sut, float x)
{
    uint32_t sayix;
    uint16_t s3,s2,s1,s0;
    uint16_t adr=0;
    if(sat==0) adr=0x80+sut;
    if(sat==1) adr=0xC0+sut;

    sendCmdLcd(adr);
    if(x<0){x=-x;sendDataLcd('-');}

    sayix=x*100;
    s3=sayix/1000;
    sayix=sayix%1000;           s2=sayix/100;
    sayix=sayix%100;            s1=sayix/10;
    sayix=sayix%10;             s0=sayix;

    if(x>=0 && x<10) { sendDataLcd(s2+48); sendDataLcd('.'); sendDataLcd(s1+48); sendDataLcd(s0+48); }
    if(x>=10 && x<100) { sendDataLcd(s3+48); sendDataLcd(s2+48); sendDataLcd('.'); sendDataLcd(s1+48); sendDataLcd(s0+48); }
}

static inline void displayInt(uint16_t sat, uint16_t sut, int32_t x)
{
    uint32_t sayix;
    uint16_t s2,s1,s0;
    uint16_t adr=0;
    if(sat==0) adr=0x80+sut;
    if(sat==1) adr=0xC0+sut;

    sendCmdLcd(adr);
    if(x<0){x=-x;sendDataLcd('-');}

    sayix=x;
    s2=sayix/100;
    sayix=sayix%100;          s1=sayix/10;
    sayix=sayix%10;           s0=sayix;

    if(x<100) {sendDataLcd(s1+48); sendDataLcd(s0+48);}
    if(x>=100 && x<1000) {sendDataLcd(s2+48); sendDataLcd(s1+48); sendDataLcd(s0+48);}
}
#endif
