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

#define SIZE 256 // unit: byte 
#define START_POINT 0
    
// Global variable 
unsigned char initFlag = 0;
unsigned char counter = 0;
int sensorVal_1 = 0;
float lowValueOfFrequency = 0.6; // unit: Hz
float highValueOfFrequency = 1.5; // unit: Hz

/*
unsigned char Period[3] = {0, 0, 0};
unsigned char Pos[2] = {0, 0};
unsigned char Neg[2] = {0, 0};
*/
unsigned char tempPositiveVal = 0;
unsigned char tempNegativeVal = 0;
unsigned char positiveVal = 0;
unsigned char negativeVal = 0;

unsigned char index = 0, outputCount = 0;
int rIndex = START_POINT, rHead = START_POINT;
int wIndex = START_POINT, wHead = START_POINT;
unsigned char buffer[SIZE] = {0};

// Method Prototype 
void checkFrequencyRange(float , float , int);

/* GPS Format 
 $GP GGA 
 $GP GLL 
 $GP GSA 
 $GP GSV 
 $GP MSS 
 $GP RMC 
 $GP VTG 
 $GP ZDA 
 */
void interrupt InterruptHandlerLow ()
{
    unsigned char cUART_char = 0;

    if (PIR1bits.RCIF == 1) // is interrupt occured by EUSART receive?,
    {                               // then RCREG is full we have new data (cleared when RCREG is read)
                               
        // if(RCSTA&0x06) //more efficient way than following commented method to check for reception error
        if (RCSTAbits.FERR == 1 || RCSTAbits.OERR == 1)
        {
            RCSTAbits.CREN = 0;    //Overrun error (can be cleared by clearing bit CREN)
            cUART_char = RCREG;    //clear Framing error
            RCSTAbits.CREN = 1;
        }
        else
        {
            cUART_char = RCREG; // read new data into variable
        }
        
        // {LeonHuang20180313+ [GPS Data filter] 
        
        // wHead, outputCount need to lock
        
        // Buffer is full 
        if ((rHead + rIndex) % SIZE == wHead)
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        
        // Head of GPS
        if (cUART_char == '$')
        {
            rIndex = 0;
            buffer(rHead) = cUART_char;
            
            
            rIndex++;
            return;
        }
        
        // {GPS Data Filter+ 
        int offset = 0;
        offset = rIndex - rHead; // 1 - 254 = -253, or 3 - 0 = 3 
        if (buffer[rHead] == '$' && (offset == 3 || offset == -253) && cUART_char == 'V') // $GPVTG,
        {
            buffer[rHead] = 0;
            rIndex = rHead;
            return;
        }
        else if (buffer[rHead] == '$' && (offset == 3 || offset == -253) && cUART_char == 'Z')  // $GPZDA
        {
            buffer[rHead] = 0;
            rIndex = rHead;
            return;
        }
        else if (buffer[rHead] == '$' && (offset == 3 || offset == -253) && cUART_char == 'M')  // $GPMSS
        {
            buffer[rHead] = 0;
            rIndex = rHead;
            return;
        }
        else if (buffer[rHead] == '$' && (offset == 3 || offset == -253) && cUART_char == 'S') // $GPGSA, $GPGSV,
        {
            buffer[rHead] = 0;
            rIndex = rHead;
            return;
        }
        else if (buffer[rHead] == '$' && (offset == 3 || offset == -253) && cUART_char == 'L') // $GPGLL
        {
            buffer[rHead] = 0;
            rIndex = rHead;
            return;
        }
        else if (buffer[rHead] == '$') // $GPGGA, $GPRMC,     We want 
        {
            buffer[(rHead + rIndex) % SIZE] = cUART_char;
            
            //  End of GPS 
            if (cUART_char == 0x0A) // Optimization
            {
                outputCount++;
                rHead = (rHead + rIndex + 1) % SIZE;
                rIndex = 0;
                return;
            }
            
            rIndex++;
        }
        else
        {
            // do nothing 
        }
        // GPS Data Filter-} 
        // LeonHuang20180313-}
               
        // LeonHuang20180313+ [ Original code, hot key: Ctrl + / ] 
//        if (outputFlag == 0)
//        {  
//            if (cUART_char == '$' )
//            {
//                index = 0;
//                buffer[index] = '$';
//                index++;
//            }
//            else if (index > 0 )
//            {
//                buffer[index] = cUART_char;
//     
//                if (buffer[index] == 0x0A) //  new line
//                    outputFlag = 1;  
//                index++;
//            }
//            else
//            {}
//                //; // ?
//           
//            if (index == 6) // ,
//            {
//                if ((buffer[5] == 'C') || (buffer[5] == 'A'))
//                {}
//                else
//                {
//                    index = 0;
//                    outputFlag = 0;
//                }
//            }
//        }
        // LeonHuang20180313-
    }
}

void main(void)
{
    unsigned char i = 0;
    
    __delay_ms(100); 
    
    Example_System_Init();    
    EUSARTInit();
            
    //TMR2ON =1;
    // INTCONbits.GIE = 1;             // Initialization complete. Begin servicing interrupts.

    // Initial Information
    if (initFlag == 0)
    {
        __delay_ms(1000);  
        initFlag = 1;
    }

    while (1)
    {
        // {LeonHuang20180313+ [Write GPS data] 
        //  [wHead], [outputCount] need to lock
        while (outputCount > 0) // Have a completed GPS data in buffer  
        {
            putch(buffer[(wHead + wIndex) % SIZE]);
            
            //  End of GPS data 
            if (buffer[(wHead + wIndex) % SIZE] == 0x0A)
            {
                outputCount--;
                wHead = (wHead + wIndex + 1) % SIZE; // new GPS data
                wIndex = -1; // milestone 
            }
            
            wIndex++;
        }
        // LeonHuang20180313-}
        
        if (outputCount == 1)
        {
            for (i = 0; i < 100; i++)
            {
                putch(buffer[i]);
                    if (buffer[i] == 0x0A)
                    break;
            }
            
            if (i<10)
                __delay_ms(19); // j= 1;
            
            if ((i >= 10) && ( i < 20))
                __delay_ms(18); // j= 2;
            
            if( (i>=20) && (i<30))
               __delay_ms(17); //j= 3;
            
            if( (i>=30) && (i<40))
               __delay_ms(16); //j= 4;
            
            if( (i>=40) && (i<50))
               __delay_ms(15); //j= 5;
            
            if( (i>=50) && (i<60))
               __delay_ms(14); //j= 6;
            
            if( (i>=60) && (i<70))
               __delay_ms(13); //j= 7;
            
            if( (i>=70) && (i<80))
               __delay_ms(12); //j= 8;
            
            if( (i>=80) && (i<100))
               __delay_ms(11);//j= 9;
            
            for (i = 0; i < 100; i++)
                buffer[i]= 0;

            index = 0;
            outputCount = 0;
        }
        else
        {
            __delay_ms(20);  
        }
            
        if ( g_Direction == 0)
            sensorVal_1 = 0;    
        else
            sensorVal_1 = 1;
        
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
    // Optimization
    float lowerBound = 33; //  ((1 / highValue) * 1000 / 20);
    float upperBound = 83; // ((1 / lowValue) * 1000 / 20);

    // Count
    if (sensorVal == 0)
    {
        tempNegativeVal += 1;

        if (tempPositiveVal > 0)
        {
            positiveVal = tempPositiveVal;
            tempPositiveVal = 0;
        }
    }
    else
    {
        tempPositiveVal += 1;
    
        if (tempNegativeVal > 0) 
        {
            negativeVal = tempNegativeVal;
            tempNegativeVal = 0;
        }
    }
    
    // Clear data
    if (tempNegativeVal > upperBound) // > 83 count
    {
        tempNegativeVal = 0;
        negativeVal = 0;
    }
    if (tempPositiveVal > upperBound) // > 83 count
    {
        tempPositiveVal = 0;
        positiveVal = 0;
    }

    // Check once per 200 ms
    counter++;
    if (counter >= 10) // 200 ms
    {
        unsigned char sum = negativeVal + positiveVal;
        if ((sum >= lowerBound) && (sum <= upperBound))
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

