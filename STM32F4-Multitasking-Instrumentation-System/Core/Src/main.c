/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "lcd.h"
#include "ds1307.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
RTC_HandleTypeDef hrtc;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t sn=0, dak=25, saat=10, gun=15, haftagun=4, ay=5, yil=26;

/* 7 SEGMENT ORTAK ANOT TABLOSU */
const unsigned char segtable[]={0xC0,0xF9,0xA4,0xB0,0x99,
		                        0x92,0x82,0xF8,0x80,0x90,0x9C,0xC6};

uint16_t s4=0, s3=0, s2=0, s1=0;
uint16_t tiksay=0, tiksay1=0, tiksay2=0;

/* NAVİGASYON VE DURUM BAYRAKLARI */
volatile int menu_index = 0;
volatile uint8_t enter_pressed = 0;
volatile uint8_t back_pressed = 0;
uint8_t current_app = 0;

uint32_t seg_timer = 0;
uint8_t display_mode = 0;
uint32_t app_timer = 0;
uint8_t buzzer_state = 0;
uint8_t led_toggle_state = 0;

float temperature = 25.0f;
float pot_voltage = 0.0f;

const char *menu_items[5] = {
    "1.Buzzer Uyg.",
    "2.LED Uyg.",
    "3.RTC Tarih",
    "4.POT Voltaj",
    "5.UART Haber"
};
/* USER CODE END PV */

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_ADC1_Init(void);
void MX_I2C1_Init(void);
void MX_TIM2_Init(void);
void MX_RTC_Init(void);
void MX_USART2_UART_Init(void);

void Update_Menu_Display(void);
void Execute_Applications(void);
void Read_Sensors(void);

void delay_us(uint16_t delay) {
    volatile uint16_t i;
    while(delay--) { for(i = 0; i < 10; i++); }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

/* KİLİTLENMEYİ ÖNLEYEN VE PARAZİT ENGELLEYİCİ AKILLI BUTON MOTORU */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_exti_tick = 0;
    if(HAL_GetTick() - last_exti_tick < 250) return;
    last_exti_tick = HAL_GetTick();

    if(GPIO_Pin == GPIO_PIN_12){
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) {
            if(current_app == 0) {
                menu_index = (menu_index - 1 + 5) % 5;
                Update_Menu_Display();
            }
        }
    }
    if(GPIO_Pin == GPIO_PIN_13){
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET) {
            if(current_app == 0) {
                menu_index = (menu_index + 1) % 5;
                Update_Menu_Display();
            }
        }
    }
    if(GPIO_Pin == GPIO_PIN_14){
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET) {
            if(current_app == 0) enter_pressed = 1;
        }
    }
    if(GPIO_Pin == GPIO_PIN_15){
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET) {
            if(current_app != 0) back_pressed = 1;
        }
    }
}

void Update_Menu_Display(void)
{
    clearLcd();
    uint8_t start = (menu_index / 2) * 2;

    char line1[21];
    char line2[21];

    sprintf(line1, "%c%s", (menu_index == start) ? '*' : ' ', menu_items[start]);
    writeStr(0, 0, line1);

    if(start + 1 < 5)
    {
        sprintf(line2, "%c%s", (menu_index == start + 1) ? '*' : ' ', menu_items[start + 1]);
        writeStr(1, 0, line2);
    }
}

void Execute_Applications(void)
{
    if(back_pressed == 1) {
        back_pressed = 0;
        enter_pressed = 0;
        current_app = 0;
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        clearLcd();
        HAL_Delay(100);
        Update_Menu_Display();
        return;
    }

    if(enter_pressed) {
        current_app = menu_index + 1;
        enter_pressed = 0;
        clearLcd();
        app_timer = HAL_GetTick();
        buzzer_state = 0;
        led_toggle_state = 0;
    }

    if(current_app == 0) return;

    switch(current_app) {
        case 1: /* BUZZER UYGULAMASI */
            writeStr(0, 0, "Buzzer Uygulama ");
            if(buzzer_state == 0) {
                writeStr(1, 0, "Durum: AKTIF (3s)");
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                if(HAL_GetTick() - app_timer >= 3000) {
                    buzzer_state = 1;
                    app_timer = HAL_GetTick();
                }
            } else {
                writeStr(1, 0, "Durum: PASIF (1s)");
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                if(HAL_GetTick() - app_timer >= 1000) {
                    buzzer_state = 0;
                    app_timer = HAL_GetTick();
                }
            }
            break;

        case 2: /* LED FLASHER MODU */
            writeStr(0, 0, "LED Uygulamasi  ");
            if(HAL_GetTick() - app_timer >= 500) {
                app_timer = HAL_GetTick();
                led_toggle_state = !led_toggle_state;
            }
            if(led_toggle_state) {
                writeStr(1, 0, "LED: OO XX      ");
            } else {
                writeStr(1, 0, "LED: XX OO      ");
            }
            break;

        case 3: /* RTC MODU */
            {
                char rtc_buf[21];
                sprintf(rtc_buf, "Tar: %02d/%02d/20%02d", gun, ay, yil);
                writeStr(0, 0, "RTC Tarih Bilgisi");
                writeStr(1, 0, rtc_buf);
            }
            break;

        case 4: /* POT VOLTAJ MODU */
            writeStr(0, 0, "POT Voltaj Degeri");
            displayFloat(1, 0, pot_voltage);
            break;

        case 5: /* UART PYTHON MODU */
            writeStr(0, 0, "UART Tx Calisiyor");
            writeStr(1, 0, "Python'a Yollandi");
            if(HAL_GetTick() - app_timer >= 1000) {
                app_timer = HAL_GetTick();
                GetDateTime_ds1307();
                char tx_msg[120];
                sprintf(tx_msg, "%.1f,%02d:%02d:%02d,%02d/%02d/20%02d\r\n",
                        temperature, saat, dak, sn, gun, ay, yil);
                HAL_UART_Transmit(&huart2, (uint8_t*)tx_msg, strlen(tx_msg), 100);
            }
            break;
    }
}

void Read_Sensors(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    sConfig.Channel = ADC_CHANNEL_0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        pot_voltage = (HAL_ADC_GetValue(&hadc1) * 3.3f) / 4095.0f;
    }
    HAL_ADC_Stop(&hadc1);

    sConfig.Channel = ADC_CHANNEL_7;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t raw_ntc = HAL_ADC_GetValue(&hadc1);
        float v_ntc = (raw_ntc * 3.3f) / 4095.0f;
        temperature = 25.0f + ((1.65f - v_ntc) * 35.0f);
    }
    HAL_ADC_Stop(&hadc1);
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_RTC_Init();
  MX_USART2_UART_Init();

  lcdInit();
  writeStr(0, 0, "    Sistem      ");
  writeStr(1, 0, "    Basliyor    ");
  HAL_Delay(1000);
  clearLcd();

  SetDateTime_ds1307();
  Update_Menu_Display();

  HAL_TIM_Base_Start_IT(&htim2);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
        Read_Sensors();

        if (current_app != 5) {
            GetDateTime_ds1307();
        }

        // PB12: YUKARI BUTONU
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) {
            HAL_Delay(30);
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) {
                if (current_app == 0) {
                    menu_index = (menu_index - 1 + 5) % 5;
                    Update_Menu_Display();
                }
                while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET);
            }
        }

        // PB13: AŞAĞI BUTONU
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET) {
            HAL_Delay(30);
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET) {
                if (current_app == 0) {
                    menu_index = (menu_index + 1) % 5;
                    Update_Menu_Display();
                }
                while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET);
            }
        }

        // PB14: ENTER BUTONU
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET) {
            HAL_Delay(30);
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET) {
                if (current_app == 0) {
                    enter_pressed = 1;
                }
                while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET);
            }
        }

        // PB15: BACK BUTONU
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET) {
            HAL_Delay(30);
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET) {
                if (current_app != 0) {
                    back_pressed = 1;
                }
                while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET);
            }
        }

        Execute_Applications();

        /* SEVEN SEGMENT DISPLAY SÜRÜCÜ MANTIĞI */
        if (display_mode == 0) {
            int t_val = (int)temperature;
            if(t_val < 0)  t_val = 0;
            if(t_val > 99) t_val = 99;

            s4 = t_val / 10;
            s3 = t_val % 10;
            s2 = 10;
            s1 = 11;
        }
        else {
            s4 = saat / 10;
            s3 = saat % 10;
            s2 = dak / 10;
            s1 = dak % 10;
        }

        if (tiksay >= 50) {
            tiksay = 0;
            if (current_app == 0) {
                Update_Menu_Display();
            }
        }

        HAL_Delay(5);
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

void MX_ADC1_Init(void)
{
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);
}

void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);
}

void MX_RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  HAL_RTC_Init(&hrtc);
}

void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim2);
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
}

void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
}

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_15, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8
                          |GPIO_PIN_9, GPIO_PIN_RESET);

  /* PC13 (Buzzer Çıkışı) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* GPIOA Çıkış Portları (Segmentler ve LCD Kontrol) */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* GPIOA Özel Segment Maske Çıkışları */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_15;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* GPIOB Çıkışları (Hane Seçiciler ve LCD Data) */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PB12-PB15 Kesme Buton Girişleri (FALLING ve PULLUP) */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
