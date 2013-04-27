/*  =========================================================================
                                 LCD MAPPING
    =========================================================================
	    A
     _  ----------
COL |_| |\   |J  /|
       F| H  |  K |B
     _  |  \ | /  |
COL |_| --G-- --M--
        |   /| \  |
       E|  Q |  N |C
     _  | /  |P  \|   
DP  |_| -----------  
	    D         

 An LCD character coding is based on the following matrix:
      { E , D , F , COL }
      { M , C , G , H   }
      { B , A , B , A   }
      { G , F , K , J   }

 The character 'A' for example is:
  -------------------------------
 LSB	{ 1 , 0 , 0 , 0 }
	{ 1 , 1 , 0 , 0 }
        { 1 , 1 , 0 , 0 }
 MSB 	{ 1 , 1 , 0 , 0 }
  -------------------
  'A' =  F    E   0   0 hexa*/

/* code for 'µ' character */
#define C_UMAP 0x6081

/* code for 'm' character */
#define C_mMap 0xb210

/* code for 'n' character */
#define C_nMap 0x2210

/* constant code for '*' character */
#define star 0xA0D7

/* constant code for '-' character */
#define C_minus 0xA000


/* Constant table for cap characters 'A' --> 'Z' */
__CONST uint16_t CapLetterMap[26]=
    {
        /* A      B      C      D      E      F      G      H      I  */
        0xFE00,0x6711,0x1d00,0x4711,0x9d00,0x9c00,0x3f00,0xfa00,0x0011,
        /* J      K      L      M      N      O      P      Q      R  */
        0x5300,0x9844,0x1900,0x5a42,0x5a06,0x5f00,0xFC00,0x5F04,0xFC04,
        /* S      T      U      V      W      X      Y      Z  */
        0xAF00,0x0411,0x5b00,0x18c0,0x5a84,0x00c6,0x0052,0x05c0
    };

/* Constant table for number '0' --> '9' */
__CONST uint16_t NumberMap[10]=
    {
        /* 0      1      2      3      4      5      6      7      8      9  */
        0x5F00,0x4200,0xF500,0x6700,0xEa00,0xAF00,0xBF00,0x04600,0xFF00,0xEF00
    };

void lcd_bars(char a, char b, char c, char d)
{
  if(a)
  {
        LCD->RAM[11] |= 0x80;
  }
  else
  {
        LCD->RAM[11] &= 0x7F;
  }
  if(c)
  {
        LCD->RAM[11] |= 0x20;
  }
  else
  {
        LCD->RAM[11] &= 0xDF;
  }
  if(d)
  {
        LCD->RAM[8] |= 0x02;
  }
  else
  {
        LCD->RAM[8] &= 0xFD;
  }
  if(b)
  {
        LCD->RAM[8] |= 0x08;
  }
  else
  {
        LCD->RAM[8] &= 0xF7;
  }
}

void lcd_init()
{
    LCD->CR1 = 0x06;
    LCD->CR2 = 0x28;
    LCD->CR3 = 0x50;
    LCD->FRQ = 0x0F;
    LCD->PM[0] = 0xFF;
    LCD->PM[1] = 0xFF;
    LCD->PM[2] = 0xFF;
    lcd_bars(1,1,1,1);
    lcd_bars(0,0,0,0);
}

static void LCD_Conv_Char_Seg(char* c,bool point, bool column, uint8_t* digit)
{
  uint16_t ch = 0 ;
  uint8_t i,j;
  
  switch (*c)
    {
    case ' ' : 
      ch = 0x00;
      break;
    
    case '*':
      ch = star;
      break;
                  
    case 'µ' :
      ch = C_UMAP;
      break;
    
    case 'm' :
      ch = C_mMap;
      break;
                  
    case 'n' :
      ch = C_nMap;
      break;					
                  
    case '-' :
      ch = C_minus;
      break;
                  
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':			
      ch = NumberMap[*c-0x30];		
      break;
          
    default:
      /* The character c is one letter in upper case*/
      if ( (*c < 0x5b) && (*c > 0x40) )
      {
        ch = CapLetterMap[*c-'A'];
      }
      /* The character c is one letter in lower case*/
      if ( (*c <0x7b) && ( *c> 0x60) )
      {
        ch = CapLetterMap[*c-'a'];
      }
      break;
  }
       
  /* Set the digital point can be displayed if the point is on */
  if (point)
  {
    ch |= 0x0008;
  }

  /* Set the "COL" segment in the character that can be displayed if the column is on */
  if (column)
  {
    ch |= 0x0020;
  }		

  for (i = 12,j=0 ;j<4; i-=4,j++)
  {
    digit[j] = (ch >> i) & 0x0f; //To isolate the less signifiant dibit
  }
}

/**
  * @brief  This function writes a char in the LCD frame buffer.
  * @param  ch: the character to dispaly.
  * @param  point: a point to add in front of char
  *         This parameter can be: POINT_OFF or POINT_ON
  * @param  column: flag indicating if a column has to be add in front
  *         of displayed character.
  *         This parameter can be: COLUMN_OFF or COLUMN_ON.           
  * @param  position: position in the LCD of the caracter to write [0:7]
  * @retval None
  * @par    Required preconditions: The LCD should be cleared before to start the
  *         write operation.  
  */
void lcd_writeChar(char* ch, bool point, bool column, uint8_t position)
{
  uint8_t digit[4];     /* Digit frame buffer */

/* To convert displayed character in segment in array digit */
  LCD_Conv_Char_Seg(ch,point,column,digit);

  switch (position)
  {
    /* Position 1 on LCD (Digit1)*/
    case 1:
      LCD->RAM[0] &= 0x0fc;
      LCD->RAM[0] |= (uint8_t)(digit[0]& 0x03); // 1M 1E	
      
      LCD->RAM[2] &= 0x3f;
      LCD->RAM[2] |= (uint8_t)((digit[0]<<4) & 0xc0); // 1G 1B
    
      LCD->RAM[3] &= 0x0cf;
      LCD->RAM[3] |= (uint8_t)(digit[1]<<4 & 0x30); // 1C 1D
                                                                                                                                      
      LCD->RAM[6] &= 0xf3;
      LCD->RAM[6] |= (uint8_t)(digit[1]&0x0c); // 1F 1A
      
      LCD->RAM[7] &= 0x0fc;
      LCD->RAM[7] |= (uint8_t)(digit[2]&0x03); // 1Col 1P		
      
      LCD->RAM[9] &= 0x3f;
      LCD->RAM[9] |= (uint8_t)((digit[2]<<4) & 0xc0); // 1Q 1K										
      
      LCD->RAM[10] &= 0xcf;
      LCD->RAM[10] |= (uint8_t)((digit[3]<<2)& 0x30); // 1DP 1N	
                                                                                                                                      
      LCD->RAM[13] &= 0xf3;
      LCD->RAM[13] |= (uint8_t)((digit[3]<<2) & 0x0c); // 1H 1J
      break;
    
    /* Position 2 on LCD (Digit2)*/
    case 2:
      LCD->RAM[0] &= 0x0f3;
      LCD->RAM[0] |= (uint8_t)((digit[0]<<2)&0x0c); // 2M 2E	
      
      LCD->RAM[2] &= 0xcf;
      LCD->RAM[2] |= (uint8_t)((digit[0]<<2)&0x30); // 2G 2B
      
      LCD->RAM[3] &= 0x3f;
      LCD->RAM[3] |= (uint8_t)((digit[1]<<6) & 0xc0); // 2C 2D
      
      LCD->RAM[6] &= 0xfc;
      LCD->RAM[6] |= (uint8_t)((digit[1]>>2)&0x03); // 2F 2A
      
      LCD->RAM[7] &= 0xf3;
      LCD->RAM[7] |= (uint8_t)((digit[2]<<2)& 0x0c); // 2Col 2P		
      
      LCD->RAM[9] &= 0xcf;
      LCD->RAM[9] |= (uint8_t)((digit[2]<<2)&0x30); // 2Q 2K										
      
      LCD->RAM[10] &= 0x3f;
      LCD->RAM[10] |= (uint8_t)((digit[3]<<4)& 0xC0); // 2DP 2N	
      
      LCD->RAM[13] &= 0xfc;
      LCD->RAM[13] |= (uint8_t)((digit[3])& 0x03); // 2H 2J
      break;
    
    /* Position 3 on LCD (Digit3)*/
    case 3:
      LCD->RAM[0] &= 0xcf;
      LCD->RAM[0] |= (uint8_t)(digit[0]<<4) & 0x30; // 3M 3E	
      
      LCD->RAM[2] &= 0xf3;
      LCD->RAM[2] |= (uint8_t)(digit[0]) & 0x0c; // 3G 3B
      
      LCD->RAM[4] &= 0xfc;
      LCD->RAM[4] |= (uint8_t)(digit[1]) & 0x03; // 3C 3D
      
      LCD->RAM[5] &= 0x3f;
      LCD->RAM[5] |= (uint8_t)(digit[1]<<4) & 0xc0; // 3F 3A
                                                                                                                                      
      LCD->RAM[7] &= 0xcf;
      LCD->RAM[7] |= (uint8_t)(digit[2]<<4)&0x30; // 3Col 3P		
                                                                                                                                      
      LCD->RAM[9] &= 0xf3;
      LCD->RAM[9] |= (uint8_t)(digit[2]) & 0x0C;  // 3Q 3K										
      
      LCD->RAM[11] &= 0xfc;
      LCD->RAM[11] |= (uint8_t)(digit[3]>>2) & 0x03 ; // 3DP 3N	
                                                                                                      
      LCD->RAM[12] &= 0x3f;
      LCD->RAM[12] |= (uint8_t)(digit[3]<<6) & 0xc0; // 3H 3J
      break;
    
    /* Position 4 on LCD (Digit4)*/
    case 4:
      LCD->RAM[0] &= 0x3f;
      LCD->RAM[0] |= (uint8_t)(digit[0]<<6) & 0xc0; // 4M 4E	
      
      LCD->RAM[2] &= 0xfc;
      LCD->RAM[2] |= (uint8_t)(digit[0]>>2) & 0x03; // 4G 4B
      
      LCD->RAM[4] &= 0xf3;
      LCD->RAM[4] |= (uint8_t)(digit[1]<<2) & 0x0C; // 4C 4D
      
      LCD->RAM[5] &= 0xcf;
      LCD->RAM[5] |= (uint8_t)(digit[1]<<2) & 0x30; // 4F 4A
                                                                                                                                      
      LCD->RAM[7] &= 0x3f;
      LCD->RAM[7] |= (uint8_t)(digit[2]<<6) & 0xC0; // 4Col 4P		
    
      LCD->RAM[9] &= 0xfc;				
      LCD->RAM[9] |= (uint8_t)(digit[2]>>2) & 0x03 ; // 4Q 4K										
    
      LCD->RAM[11] &= 0xf3;				
      LCD->RAM[11] |= (uint8_t)(digit[3]) & 0x0C; // 4DP 4N	
    
      LCD->RAM[12] &= 0xcf;				
      LCD->RAM[12] |= (uint8_t)(digit[3]<<4) & 0x30; // 4H 4J
      break;
    
    /* Position 5 on LCD (Digit5)*/
    case 5:
      LCD->RAM[1] &= 0xfc;
      LCD->RAM[1] |=  (uint8_t)(digit[0]) & 0x03; // 5M 5E	
      
      LCD->RAM[1] &= 0x3f;
      LCD->RAM[1] |=  (uint8_t)(digit[0]<<4) & 0xc0; // 5G 5B
    
      LCD->RAM[4] &= 0xcf;				
      LCD->RAM[4] |= (uint8_t)(digit[1]<<4) & 0x30; // 5C 5D
    
      LCD->RAM[5] &= 0xf3;				
      LCD->RAM[5] |=  (uint8_t)(digit[1]) & 0x0c; // 5F 5A
    
      // 5 DP 5 COL not used
      
      LCD->RAM[8] &= 0xfe;
      LCD->RAM[8] |=  (uint8_t)(digit[2]) & 0x01; //  5P	
      
      LCD->RAM[8] &= 0x3f;					
      LCD->RAM[8] |=  (uint8_t)(digit[2]<<4) & 0xc0; // 5Q 5K										
    
      LCD->RAM[11] &= 0xef;				
      LCD->RAM[11] |=  (uint8_t)(digit[3]<<2) & 0x10; // 5N	
    
      LCD->RAM[12] &= 0xf3;				
      LCD->RAM[12] |=  (uint8_t)(digit[3]<<2) & 0x0C; // 5H 5J
      break;
    
    /* Position 6 on LCD (Digit6)*/
    case 6:
      LCD->RAM[1] &= 0xf3;
      LCD->RAM[1] |=  (uint8_t)(digit[0]<<2) & 0x0C; // 6M 6E	
    
      LCD->RAM[1] &= 0xcf;				
      LCD->RAM[1] |=  (uint8_t)(digit[0]<<2) & 0x30; // 6G 6B
    
      LCD->RAM[4] &= 0x3f;				
      LCD->RAM[4] |= (uint8_t)(digit[1]<<6) & 0xc0; // 6C 6D
    
      LCD->RAM[5] &= 0xfc;				
      LCD->RAM[5] |=  (uint8_t)(digit[1]>>2) & 0x03; // 6F 6A
      
      LCD->RAM[8] &= 0xfb;
      LCD->RAM[8] |=  (uint8_t)(digit[2]<<2) & 0x04; //  6P	
      
      // 6 DP 6COL not used
      LCD->RAM[8] &= 0xcf;					
      LCD->RAM[8] |=  (uint8_t)(digit[2]<<2) & 0x30; // 6Q 6K	
    
      LCD->RAM[11] &= 0xbf;				
      LCD->RAM[11] |=  (uint8_t)(digit[3]<<4) & 0x40; // 6N	
    
      LCD->RAM[12] &= 0xfc;				
      LCD->RAM[12] |= (uint8_t)(digit[3]) & 0x03; // 6H	6J
      break;
    
      default:
              break;
  }
}
void lcd_putc(char * c)
{
  int i = 0;
  for(i = 0; i < 7; i++)
  {
    if(c[i] == 0)
    {
      i = 8;
    }
    else
    {
      lcd_writeChar(c + i, FALSE,FALSE,i+1);
    }
  }
}
void lcd_value(char * name, long value, unsigned char point)
{
  char i = 0;
  char j = 0;
  long maxval = 1;
  char dig[] = "0123456789";
  char digit = 0;
  bool sign = FALSE;
  if(value<0)
  {
    sign = TRUE;
    value = -value;
  }
  //printing name
  if(*name != 0)
  {
    if(*(name+1)==0)
    {
      lcd_writeChar(name, FALSE,TRUE,1);
      i = 2;
    }
    else
    {
      lcd_writeChar(name, FALSE,FALSE,1);
      lcd_writeChar(name +1, FALSE, TRUE,2);
      i = 3;
    }
  }
  //testing/removing fract if possible
  for(j = 6; j >= i; j--)
  {
    maxval*=10;
  }
  if(sign)
  {
    maxval/=10;
  }
  if(value >= maxval && point)
  {
     value /= 100;
     point = 0;
  }
  if(value >=maxval)
  {
    if(i < 3)
    {
      lcd_writeChar(" ", FALSE, FALSE,2);
    }      
    if(i < 2)
    {
      lcd_writeChar(" ", FALSE, FALSE,1);
    }      
    lcd_writeChar(" ", FALSE, FALSE,3);
    lcd_writeChar("O", FALSE, FALSE,4);
    lcd_writeChar("L", FALSE, FALSE,5);
    lcd_writeChar(" ", FALSE, FALSE,6);
    return;
  }
  // begin drowing from 6 to i
  for(j = 6; j >= i; j--)
  {
    digit = value % 10;
    value/=10;
    
    lcd_writeChar(dig + digit, (point &&(j == 4)), FALSE, j);
    if(value == 0 && digit == 0 && (j < 4 || point == 0))
    {
      if(sign) 
      {
        lcd_writeChar("-", FALSE, FALSE, j);
        sign = FALSE;
      }
      else
      {
        lcd_writeChar(" ", FALSE, FALSE, j);
      }
      if(j == 6)
      {
        lcd_writeChar("0", FALSE, FALSE, j);
      }        
    }
  }
  return;
}
