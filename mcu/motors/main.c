#include "stm8l15x.h"
#define I2C_BUSY 1
#define I2C_ACK_ERR 2
#define I2C_BUS_ERR 3
#define I2C_ARB_ERR 4
#define I2C_OTH_ERR 5

#pragma vector=26
__interrupt void timer_set(void)
{
  if(TIM1->SR1 & 0x02)
  {
    GPIOD->ODR |= 0x10;
    TIM1->SR1 &= 0xFD;
  }
  if(TIM1->SR1 & 0x04)
  {
    GPIOD->ODR |= 0x20;
    TIM1->SR1 &= 0xFB;
  }
  return;
}
#pragma vector=25
__interrupt void timer_clr(void)
{
   static int i = 0;
   static int step = 1;
   if(i > 2)
   {
	i = 0;
	TIM1->CCR1L+=step;
	if (TIM1->CCR1L > 0xF1)
	     step = -1;
	if(TIM1->CCR1L < 0xC3)
	     step = 1;
   }
   i++;
	GPIOD->ODR = 0x00;
   TIM1->SR1 &= 0xFE;
   return;
}

#pragma vector=31
__interrupt void i2c_interrupt(void)
{
  static char motors = 1; 
  if(I2C1->SR1 & I2C_SR1_ADDR) //DATA BIT;
   {
     I2C1->SR3;
     motors = 1;
   }
   if(I2C1->SR1 &I2C_SR1_RXNE) //STOP
   {
     if(motors == 1)
     {
       TIM1->CCR1L = 0xFF - I2C1->DR;
     }
     else
     {
       TIM1->CCR2L = 0xFF - I2C1->DR;
     }
     motors = 2;
   }
   return;
}

int main( void )
{
     //System clock
    CLK->PCKENR1 = CLK_PCKENR1_I2C1; //I2C enabled
    CLK->PCKENR2 = CLK_PCKENR2_TIM1; //Timer 1 enabled
    CLK->CKDIVR = 0x00;
    
    //GPIO
    GPIOD->DDR = 0x30;
    GPIOD->CR1 = 0x30;
    GPIOD->CR2 = 0;
    GPIOD->ODR = 0x0;
       
    //TIMER1
    TIM1->SMCR = 0x00;
    TIM1->CR1 = TIM1_CR1_ARPE | TIM1_CR1_CEN;
    TIM1->CR2 = 0x20;
    TIM1->IER = TIM1_IER_UIE | TIM1_IER_CC1IE | TIM1_IER_CC2IE; //break interrupt
    TIM1->PSCRH = 0x02;
    TIM1->PSCRL = 0x80;
    TIM1->ARRH = 0x01;
    TIM1->ARRL = 0x00;
    TIM1->EGR = TIM1_EGR_UG;// F2-C2
    TIM1->CCR1L = 0xC2;
    //IIC
    I2C1->FREQR = 0x10; //for 16MHz system clock
    I2C1->TRISER = 0x03; //500ns
    I2C1->CCRL = 0x0A; //100kHz bus speed;
    I2C1->CCRH = 0x00;
    I2C1->CR1 = I2C_CR1_PE; //peripherial enable
    I2C1->CR2 = I2C_CR2_ACK;
    I2C1->OARH = 0x40;
    I2C1->OARL = 0x92;  //Address = 0x49;
    I2C1->ITR = I2C_ITR_ITBUFEN | I2C_ITR_ITEVTEN;
    

    enableInterrupts();
    while(1)
    {
    };
}
