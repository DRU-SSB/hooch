#include "stm8l15x.h"
#include "lcd.c"
//#include "math.h"
#define I2C_BUSY 1
#define I2C_ACK_ERR 2
#define I2C_BUS_ERR 3
#define I2C_ARB_ERR 4
#define I2C_OTH_ERR 5
const int Temp[] ={
  -14313,-12734,-11189,-9676,
  -8203,-6761,-5348,-3970,-2614,-1297,
  0,1266,2507,3725,4914,6081,
  7226,8351,9450,10529,11590,
  12626,13645,14646,15626,16589,
  17536,18467,19379,20276,21158,
  22023,22878};

typedef struct _ads1100_resp
{
  int sample;
  unsigned char sreg;
} ads1100_resp;
typedef struct _pid_parms
{
  float Kp;
  float Ki;
  float Kd;
  float dest;
} pid_parms;
typedef struct _pid_stat
{
  float U1;
  float E1;
  float E2;
} pid_stat;
union
{
  struct
  {
    unsigned char type;
    unsigned char Kp_Tm[8];
    unsigned char Ki_Ta[8];
    unsigned char Kd_To[8];
    unsigned char Pm[2];
    unsigned char Pa[2];
  } data;
  unsigned char bytes[29];
} port_data;

pid_parms pid_main;
pid_parms pid_aux;
pid_stat stat_main;
pid_stat stat_aux;

unsigned char combine_c(char*d)
{
  unsigned char r;
  r = *d -0x41;
  r <<=4;
  r|=*(d+1) - 0x41;
  return r;
};
void split_c(unsigned char d, char* p)
{
  *p = ((d & 0xF0) >> 4) + 0x41;
  *(p+1) = (d & 0x0F)+0x41;
  return;
}
int adc2centigrade(int c)
{
  char i = 0;
  signed char j = -1;
  float add = 0;
  for(i = 0; i<32;i++)
  {
    if(Temp[i] <= c)
    {
      j = i;
    }
  }
  if(j == -1)
  {
    return -1;
  }
  else
  {
    add = ((float)(c - Temp[j]))/((float)(Temp[j+1] - Temp[j])) + (float)(j-10);
    return (int)(add*500);
  }
}
void i2c_init()
{
  I2C1->FREQR = 0x02; //for 2MHz system clock
  I2C1->TRISER = 0x03; //500ns
  I2C1->CCRL = 0x0A; //100kHz bus speed;
  I2C1->CCRH = 0x00;
  I2C1->CR1 = 0x01; //peripherial enable
  return;
}
char i2c_chk()
{
  if(I2C1->SR2)
  {
      if(I2C1->SR2 & 0x04)
      {
      	return I2C_ACK_ERR;
      }
      if(I2C1->SR2 & 0x01)
      {
        return I2C_BUS_ERR;
      }
      if(I2C1->SR2 &0x02)
      {
        return I2C_ARB_ERR;
      }
      return I2C_OTH_ERR;
  }
  return 0;
}
char i2c_read_data(char addr, unsigned char *buffer, char len)
{
  unsigned char i;
  char i2c_err = 0;
  
  if(I2C1->SR1 || I2C1->SR2 || I2C1->SR3)
  {
      I2C1->CR1 = 0x00;
      I2C1->CR2 = 0x80;
      while(I2C1->SR1 || I2C1->SR2 || I2C1->SR3);
      I2C1->CR2 = 0x00;
      i2c_init();
  }
  I2C1->CR2 = 0x05;
  
  while((I2C1->SR1 & 0x01) == 0)
  {
      i2c_err = i2c_chk();
      if(i2c_err)
      {
	  return i2c_err;
      }
  }
  I2C1->DR = addr*2 +1;

  for(i = 0; i<len; i++)
  {
    while((I2C1->SR1 & 0x40) == 0)
    {
      I2C1->SR3;
      i2c_err = i2c_chk();
      if(i2c_err)
      {
	 return i2c_err;
      }
    }
    buffer[i] = I2C1->DR;
  }
  I2C1->CR2 = 0x02; //STOP
  return 0;
}

char i2c_write_data(char addr, unsigned char* word, char len)
{
  char i2c_err = 0;
  unsigned char i;
  if(I2C1->SR1 || I2C1->SR2 || I2C1->SR3)
  {
      I2C1->CR1 = 0x00;
      I2C1->CR2 = 0x80;
      while(I2C1->SR1 || I2C1->SR2 || I2C1->SR3);
      I2C1->CR2 = 0x00;
      i2c_init();
  }
      
  I2C1->CR2 = 0x05;
  while((I2C1->SR1 & 0x01) == 0)
  {
      i2c_err = i2c_chk();
      if(i2c_err)
      {
	  return i2c_err;
      }
  }
  I2C1->DR = addr*2;
  while((I2C1->SR1 ^ 0x80) & 0x83)
  {
    I2C1->SR3;
    i2c_err = i2c_chk();
    if(i2c_err)
    {
	return i2c_err;
    }
  }
  for(i = 0; i < len; i ++)
  {
    I2C1->DR = word[i];
    while((I2C1->SR1 & 0x80) == 0)
    {
        i2c_err = i2c_chk();
        if(i2c_err)
        {
	    return i2c_err;
        }
    }
  }
  I2C1->CR2 = 0x02; //STOP
  return 0;
}
void init_pid(pid_stat *stat)
{
  (*stat).E1 = 0;
  (*stat).E2 = 0;
  (*stat).U1 = 50;
}

void eval_pid(float t, unsigned char* motor, pid_parms parm, pid_stat *statref)
{
  float R;
  float E;
  E = t - parm.dest;
  R = (*statref).U1;
  R += parm.Kp*(E - (*statref).E1);
  R += parm.Ki*E;
  R += parm.Kd*(E - 2*(*statref).E1 + (*statref).E2);
  if(R > 100)
  {
     R = 100;
  }
  else if(R < 0)
  {
    R = 0;
  }
  (*statref).E2 = (*statref).E1;
  (*statref).E1 = E;
  (*statref).U1 = R;
  *motor = (unsigned char)(R*2.55);
  
}
void sw()
{
        static char i;
        static int t_main = 0;
//        static int t_aux = 0;
        unsigned char motors[2] = {0x50, 0x20};
        ads1100_resp adc_data;
        i2c_write_data(0x49, motors,2);
        if(i2c_read_data(0x4A, (unsigned char*)&adc_data, 3))
	{
		lcd_putc("M_ADC  ");
	}
	else
	{
            t_main = adc2centigrade(adc_data.sample);
            lcd_value("T", t_main,1);
	}
        eval_pid((float)t_main/100, motors, pid_main, &stat_main);
        if((motors[0] > 220)||motors[1] > 220)
	{
	    GPIOE->ODR = 0;
	}
	else
	{
	    GPIOE->ODR = 128;
	}
        if(GPIOC->ODR == 128)
	{
		GPIOC->ODR = 0;
	}
	else
	{
	    GPIOC->ODR = 128;
	}
        switch(i)
        {
        case 0:
          lcd_bars(1,0,0,0);
          i++;
          break;
        case 1:
          lcd_bars(0,1,0,0);
          i++;
          break;
        case 2:
          lcd_bars(0,0,1,0);
          i++;
          break;
        case 3:
          lcd_bars(0,0,0,1);
          i = 0;
          break;
        default:  
          i = 0;
        }
        i2c_write_data(0x49, motors,2);
}

#pragma vector=21
__interrupt void timer20(void)
{
  TIM2->SR1 = 0;
  sw();
  return;
}
#pragma vector=29
__interrupt void usart_TX_cmp(void)
{
  //USART1->DR = 45;
  return;
}
#pragma vector=30
__interrupt void usart_RX_cmp(void)
{
/*  static char i = 0;
  static char msg_type = 0;
  unsigned char dr;
  dr = USART1->DR;
  switch(dr)
  {
  case '0':
    i = 0;
  }*/  
  return;
}

int main(void)
{
    unsigned char i = 0x0F;
    //System clock
    
    CLK->PCKENR1 = CLK_PCKENR1_TIM2 | CLK_PCKENR1_I2C1 | CLK_PCKENR1_TIM3 | CLK_PCKENR1_USART1;
    CLK->PCKENR2 = CLK_PCKENR2_ADC1 | CLK_PCKENR2_LCD | CLK_PCKENR2_COMP;
    CLK->CRTCR = 0x04;
    CLK->CKDIVR = 0x03; //2MHz
    //GPIO
    GPIOE->DDR = 128;
    GPIOE->CR1 = 128;
    GPIOE->CR2 = 0;
    GPIOE->ODR = 128;
    
    GPIOC->DDR = 128;
    GPIOC->CR1 = 128;
    GPIOC->CR2 = 0;
    //IIC
    i2c_init();
    //USART
    SYSCFG->RMPCR1 |= 0x10;
    USART1->PSCR = 0x01;
    USART1->BRR2 = 0x00;
    USART1->BRR1 = 0x0D;
    USART1->CR1 = 0;
    USART1->CR2 = USART_CR2_TIEN | USART_CR2_RIEN | USART_CR2_TEN | USART_CR2_REN;
    USART1->CR3 = 0;
    USART1->CR4 = 0;
    USART1->CR5 = 0;
    USART1->DR = 45;
    
    //TIMER2
    TIM2->PSCR = 0x03;
    TIM2->ARRH = 0x7A; 
    TIM2->ARRL = 0x12; //31250
    TIM2->SMCR = 0x00; //internal clock
    TIM2->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_ARPE;
    TIM2->CR2 = 0x20;
    TIM2->IER = TIM_IER_UIE;
    TIM2->EGR = TIM_EGR_UG;
    //LCD
    lcd_init();
    init_pid(&stat_main);
    lcd_putc("******");
    if(i2c_write_data(0x4A, &i,1))
    {
	lcd_putc("M_ADC  ");
    }
    enableInterrupts();
    

    while(1)
    {
    };
}