#include <xc.h>
#include "RGBChroma.h"
// CONFIG1
#pragma config FOSC     = INTOSC    //  (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE     = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE    = OFF       // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE    = ON        // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP       = OFF       // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN    = OFF       // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF       // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT      = OFF       // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN   = ON        // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV     = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP      = OFF       // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)
#define _XTAL_FREQ   16000000  //Used by the XC8 delay_ms(x) macro
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
//#define LED_A5          PORTAbits.RA5
#define g_Direction         PORTAbits.RA4
// AN1562 - Added define to omit interrupts
#define INTERRUPTS_NOT_USED// PROTOTYPES
    void            Example_System_Init (void);
// AN1562 - Added test for INTERRUPTS_NOT_USED)

  //  void interrupt  ISR                 (void);

unsigned char Flow_Init = 0;
unsigned char counter= 0;
int sensorVal_1= 0;
float lowValueOfFrequency = 0.6; // unit: Hz
float highValueOfFrequency = 1.5; // unit: Hz
  
unsigned char Period[3]= {0, 0, 0};
unsigned char Pos[2]= {0, 0};
unsigned char Neg[2]= {0, 0};
void checkFrequencyRange(float , float , int);

unsigned char GPS_flag= 0, GPS_out= 0;
unsigned char GPS[100]={0};
unsigned char test=0;


void interrupt InterruptHandlerLow ()
{
  unsigned char cUART_char= 0;
  
  if (PIR1bits.RCIF==1)//is interrupt occured by EUSART receive?,
                        //then RCREG is full we have new data (cleared when RCREG is read)
  {
  // if(RCSTA&0x06) //more efficient way than following commented method to check for reception error
    if(RCSTAbits.FERR==1 || RCSTAbits.OERR==1 )
    {
      RCSTAbits.CREN=0;    //Overrun error (can be cleared by clearing bit CREN)
      cUART_char=RCREG;    //clear Framing error
      RCSTAbits.CREN=1;
    }
    else
    {
       cUART_char = RCREG; // read new data into variable
    }
    
    
    if( cUART_char == '$' )
    {
        GPS_flag= 0;
        GPS_out= 0;
        GPS[GPS_flag++]= '$';
    }
    else if( GPS_flag > 0 )
    {
        test=1;
        GPS[GPS_flag++]= cUART_char;
        
        GPS_out= 1;
    }
    else
        ;

  }
}


void main(void)
{
    unsigned char i= 0;
    
    __delay_ms(100); 
    
    Example_System_Init();    
	EUSARTInit();
            
    //TMR2ON =1;
   // INTCONbits.GIE = 1;             // Initialization complete. Begin servicing interrupts.

      // Initial Information
  if (Flow_Init == 0)
  {
    __delay_ms(1000);  
    Flow_Init = 1;
  }

	while(1)
	{

        if( GPS_out == 1)
        {
            
            for(i=0; i < 100;i++)
            {
                putch(GPS[i]);
                if(GPS[i+1]==0)
                    break;
            }
 
            for(i=0; i < 100;i++)
                GPS[i]= 0;

            GPS_flag = 0;
            GPS_out = 0;
            
            __delay_ms(12); 
        }
        else
           __delay_ms(20);  
         
         
        if( g_Direction == 0)
            sensorVal_1 =0;    
        else
            sensorVal_1 =1;
        checkFrequencyRange(lowValueOfFrequency, highValueOfFrequency, sensorVal_1); 
        
}
}
//================================================================================================
//  _____                           _        ____            _                   ___       _ _   
// | ____|_  ____ _ _ __ ___  _ __ | | ___  / ___| _   _ ___| |_ ___ _ __ ___   |_ _|_ __ (_) |_ 
// |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \ \___ \| | | / __| __/ _ \ '_ ` _ \   | || '_ \| | __|
// | |___ >  < (_| | | | | | | |_) | |  __/  ___) | |_| \__ \ ||  __/ | | | | |  | || | | | | |_ 
// |_____/_/\_\__,_|_| |_| |_| .__/|_|\___| |____/ \__, |___/\__\___|_| |_| |_| |___|_| |_|_|\__|
//                           |_|                   |___/                          
//================================================================================================               

void Example_System_Init() 
{

   
        #if     _XTAL_FREQ == 32000000
            OSCCON  = 0b11110000;       // PLL=on,  IRCF=8  MHz, SCS=Fosc
        #elif   _XTAL_FREQ == 16000000
            OSCCON  = 0b01111000;       // PLL=off, IRCF=16 MHz, SCS=Fosc
        #else
            #error Unsupported clock frequency, edit _XTAL_FREQ in mTouch_config.h
        #endif
  
    ANSELA      = 0b00000000;           // all inputs digital
    LATA        = 0b00110100;           // RA2=1, RA4=1, RA5=1      -? all led's off by default.
    TRISA       = 0b00010000;           // all PORT A configured as outputs

}
//================================================================================================
//  _____                           _        ___ ____  ____  
// | ____|_  ____ _ _ __ ___  _ __ | | ___  |_ _/ ___||  _ \ 
// |  _| \ \/ / _` | '_ ` _ \| '_ \| |/ _ \  | |\___ \| |_) |
// | |___ >  < (_| | | | | | | |_) | |  __/  | | ___) |  _ < 
// |_____/_/\_\__,_|_| |_| |_| .__/|_|\___| |___|____/|_| \_\
//                           |_|                                                                                     
//================================================================================================

// AN1562 - added test for INTERRUPTS_NOT_USED
/**/
/*
void interrupt ISR(void)
{
    g_counter++;
    Stop_turning_LR();
    TMR2IF =0;
    TMR2ON =1;
}
*/
/*
void Stop_turning_LR(){
	
  if(g_counter==0){
    if( g_Direction == 1){ 
		  TurningR_counter++; 
      TurningL_counter= 0;
	   }


	   if( g_Direction == 0){
        TurningL_counter++;
        TurningR_counter= 0;
        
        if(TurningL_counter>= 13){ //15 //10
            g_DIR= 0;
            TurningL_counter=0;
        }
        
        if(TurningR_counter>=7){
			     g_DIR= 1;
			     g_counter= 60; 
		    }
        
	   }
   }
   else{
    
      if(g_counter>0)
        g_counter= g_counter-1;
    
   }
}
*/

void checkFrequencyRange(float lowValue, float highValue, int sensorVal)
{
  float countForHighValue = ((1 / highValue) * 1000 / 20);
  float countForLowValue = ((1/ lowValue) * 1000 / 20);

  // Count
  if (sensorVal == 0)
  {
    Neg[0] = Neg[0] + 1;
    if (Pos[0] > 0)
      Pos[1] = 1;     
  }
  else
  {
    Pos[0] = Pos[0] + 1;
    if (Neg[0] > 0)
      Neg[1] = 1;
  }

  // Clear data
  if (Pos[0] > countForLowValue)
  {
    Pos[0]= 0;
    Period[0]= 0; 
  }
  if (Neg[0] > countForLowValue)
  {
    Neg[0]= 0;
    Period[1]= 0; 
  }

  // Record count information for HIGH
  if (Pos[1] == 1)
  {
//    Serial.print('%');
//    Serial.println(Pos[0], DEC);
    Period[0] = Pos[0]; 
    Pos[0] = 0;
    Pos[1] = 0; 
  }

  // Record count information for LOW
  if (Neg[1] == 1)
  {
//    Serial.print('%');
//    Serial.println(Neg[0], DEC);
    Period[1] = Neg[0];
    Neg[0] = 0;
    Neg[1] = 0;
  }

  // Check frequency range
  counter++;
  if (counter >= 10) // 200 ms
  {
    Period[2] = Period[0] + Period[1];


   // if ((Period[2] >= 33) && (Period[2] <= 83))
    
    if ((Period[2] >= countForHighValue) && (Period[2] <= countForLowValue))
    {
      putch('%');
      putch('0');
 putch('\r');
  putch('\n');
      //putch(g_Direction);
    }
    else
    {
      putch('%');
      putch('1');   
  putch('\r');
  putch('\n');
 
      //putch(g_Direction);
    }

    counter = 0;
  }
}
/// @endcond

