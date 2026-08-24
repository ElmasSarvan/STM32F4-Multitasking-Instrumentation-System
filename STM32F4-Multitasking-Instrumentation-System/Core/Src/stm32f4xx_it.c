#include "main.h"
#include "stm32f4xx_it.h"

extern const unsigned char segtable[];
extern uint16_t s4, s3, s2, s1;
extern uint16_t tiksay, tiksay1, tiksay2;

extern uint32_t seg_timer;
extern uint8_t display_mode;

extern volatile int menu_index;
extern volatile uint8_t enter_pressed;
extern volatile uint8_t back_pressed;
extern uint8_t current_app;

static uint8_t digit_index = 0;

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void)
{
  tiksay++;
  HAL_IncTick();
}

void Drive_Segments_Custom(uint8_t seg_value)
{
    // Maskeli temizlik yardımıyla LCD pinlerini (PA8) bozmadan segment hatlarını sıfırla
    GPIOA->BSRR = (GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_15) << 16;

    if(!(seg_value & (1<<0))) GPIOA->BSRR = GPIO_PIN_15; // A -> PA15
    if(!(seg_value & (1<<1))) GPIOA->BSRR = GPIO_PIN_1;  // B -> PA1
    if(!(seg_value & (1<<2))) GPIOA->BSRR = GPIO_PIN_9;  // C -> PA9
    if(!(seg_value & (1<<3))) GPIOA->BSRR = GPIO_PIN_12; // D -> PA12
    if(!(seg_value & (1<<4))) GPIOA->BSRR = GPIO_PIN_4;  // E -> PA4
    if(!(seg_value & (1<<5))) GPIOA->BSRR = GPIO_PIN_5;  // F -> PA5
    if(!(seg_value & (1<<6))) GPIOA->BSRR = GPIO_PIN_6;  // G -> PA6
}

void ADC_IRQHandler(void)
{
  HAL_ADC_IRQHandler(&hadc1);
}

void TIM2_IRQHandler(void)
{
  // HAL Kütüphanesinin otomatik bayrak temizleme işlemini el ile yönetiyoruz kilitlenmeyi önlemek için
  if(__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) !=RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);

      tiksay1++;
      static uint16_t div_pre = 0;
      if(++div_pre >= 5) { // 7-segment tarama hızını optimize eden filtre
          div_pre = 0;
          tiksay2++;
          seg_timer++;

          if(seg_timer >= 2000) { // Tam 10 saniyede bir geçiş lojiği
              display_mode = !display_mode;
              seg_timer = 0;
          }

          // Gölgelenmeyi önleyici temizlik
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10, GPIO_PIN_RESET);

          switch(digit_index)
          {
              case 0:
                  Drive_Segments_Custom(segtable[s4]);
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
              break;
              case 1:
                  Drive_Segments_Custom(segtable[s3]);
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
              break;
              case 2:
                  Drive_Segments_Custom(segtable[s2]);
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
              break;
              case 3:
                  Drive_Segments_Custom(segtable[s1]);
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
              break;
          }
          if(++digit_index >= 4) digit_index = 0;
      }
    }
  }
}

void EXTI15_10_IRQHandler(void)
{
  // Buton kararlılığı ve kilitlenme önleyici donanımsal register temizliği
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
  }
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
  }
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
  }
  if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET) {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
  }
}
