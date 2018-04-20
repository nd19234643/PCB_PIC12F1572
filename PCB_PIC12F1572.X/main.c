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
#define DIRECTION_LIGHT_SQUARE_WAVE         PORTAbits.RA4
//#define LED_A5          PORTAbits.RA5
// AN1562 - Added define to omit interrupts
#define INTERRUPTS_NOT_USED// PROTOTYPES
// AN1562 - Added test for INTERRUPTS_NOT_USED)
//  void interrupt  ISR                 (void);
void Example_System_Init (void);

// {LeonHuang20180313+ [Optimization] 
#define SIZE 214 // unit: byte 
#define DELAY_TIME 20 // unit: ms 

// Optimization 
//float lowFrequency = 0.6; // unit: Hz
//float highFrequency = 1.5; // unit: Hz
#define LOWER_BOUND 33 // ((1 / lowFrequency) * 1000 / DELAY_TIME);
#define UPPER_BOUND 83 // ((1 / highFrequency) * 1000 / DELAY_TIME);

/*
unsigned char Period[3] = {0, 0, 0};
unsigned char Pos[2] = {0, 0};
unsigned char Neg[2] = {0, 0};
*/
//unsigned char sensorVal_1 = 0;

unsigned char tempPositiveVal = 0, tempNegativeVal = 0;
unsigned char positiveVal = 0, negativeVal = 0;

unsigned char highestPriority = 0;
unsigned char outputCount = 0, freqCount = 0;
short int timerCount = 0;
short int rIndex = 0, rHead = 0;
short int wIndex = 0, wHead = 0;
unsigned char buffer[SIZE] = {0};

// Method Prototype 
void initTimer0();
void checkFrequencyRange();
// LeonHuang20180313-}

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

    if (PIR1bits.RCIF == 1) // The RCIF interrupt flag bit will be set when there is an unread character in the FIFO
    {
                               
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
        
        // {LeonHuang20180313+ [Read GPS Data] 
        if (highestPriority == 1)
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        
        if ((rHead + rIndex + 1) % SIZE == wHead) // Buffer is full 
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        
        // Head of GPS
        if (cUART_char == '$')
        {
            rIndex = 1;
            buffer[rHead] = cUART_char;
            
            return;
        }
        
        // {GPS Data Filter+ 
        if (buffer[rHead] == '$' && rIndex == 3 && cUART_char == 'V') // $GPVTG,
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        else if (buffer[rHead] == '$' && rIndex == 3 && cUART_char == 'Z')  // $GPZDA
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        else if (buffer[rHead] == '$' && rIndex == 3 && cUART_char == 'M')  // $GPMSS
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        else if (buffer[rHead] == '$' && rIndex == 4 && cUART_char == 'S') // $GPGSA, $GPGSV,
        {
            rIndex = 0;
            buffer[rHead] = 0;
            return;
        }
        else if (buffer[rHead] == '$' && rIndex == 4 && cUART_char == 'L') // $GPGLL
        {
            rIndex = 0;
            buffer[rHead] = 0;
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
               
        // {LeonHuang20180313+ [ Original code, hot key: Ctrl + / ] 
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
        // LeonHuang20180313-}
    }
}

void main(void)
{
//    unsigned char i = 0;
    
    Example_System_Init();    
    initEUSART();
            
    //TMR2ON =1;
    // INTCONbits.GIE = 1;             // Initialization complete. Begin servicing interrupts.

    // {LeonHuang20180319+ [Init Timer0] 
    initTimer0();
    // test Timer0, use oscilloscope to analyze, cycle: 20 ms   
//    while (1)
//    {        
//        if (TMR0IF) 
//        {
//            timerCount++;
//            if (timerCount == 313) // 312 + 1, 20 ms 
//            {
//                putch('A');
//                timerCount = 0;
//                TMR0 = 126; //  128 - 2
//            }
//            TMR0IF = 0;
//        }
//    }
    // LeonHuang20180319-}
    
    // Wait some time 
    __delay_ms(100);

    while (1)
    {
        // {LeonHuang20180313+ [Write GPS data and Direction light data] 
        // {Direction light+ 
        if (TMR0IF) 
        {
            timerCount++;
            if (timerCount >= 313) // 312 + 1, 20 ms 
            {
                checkFrequencyRange();
                timerCount = 0;
                TMR0 = 126; //  128 - 2 
            }
            TMR0IF = 0;
        }
        // Direction light-}
        
        // Need 30 ~ 79 ms to output a completed GPS data
        while (outputCount > 0) // Have a completed GPS data in buffer  
        {
            putch(buffer[(wHead + wIndex) % SIZE]);
            
            //  End of GPS data 
            if (buffer[(wHead + wIndex) % SIZE] == 0x0A)
            {
                highestPriority = 1;
                outputCount--;
                wHead = (wHead + wIndex + 1) % SIZE; // new GPS data 
                wIndex = 0;
                highestPriority = 0;
                break;
            }
            wIndex++;
            
            // Timer overflow detect 
            if (TMR0IF) 
            {
                timerCount++;
                TMR0IF = 0;
            }
        }
        
        if (timerCount >= 313) // 312 + 1, 20ms 
        {
            checkFrequencyRange();
            timerCount = 0;
            TMR0 = 126; //  128 - 2
        }
        // LeonHuang20180313-}
        
    // {LeonHuang20180313+ [ Original code, hot key: Ctrl + / ] 
//        if (outputCount == 1)
//        {
//            for (i = 0; i < 100; i++)
//            {
//                putch(buffer[i]);
//                    if (buffer[i] == 0x0A)
//                    break;
//            }
//            
//            if (i<10)
//                __delay_ms(19); // j= 1;
//            
//            if ((i >= 10) && ( i < 20))
//                __delay_ms(18); // j= 2;
//            
//            if( (i>=20) && (i<30))
//               __delay_ms(17); //j= 3;
//            
//            if( (i>=30) && (i<40))
//               __delay_ms(16); //j= 4;
//            
//            if( (i>=40) && (i<50))
//               __delay_ms(15); //j= 5;
//            
//            if( (i>=50) && (i<60))
//               __delay_ms(14); //j= 6;
//            
//            if( (i>=60) && (i<70))
//               __delay_ms(13); //j= 7;
//            
//            if( (i>=70) && (i<80))
//               __delay_ms(12); //j= 8;
//            
//            if( (i>=80) && (i<100))
//               __delay_ms(11);//j= 9;
//            
//            for (i = 0; i < 100; i++)
//                buffer[i]= 0;
//
//            index = 0;
//            outputCount = 0;
//        }
//        else
//        {
//            __delay_ms(20);  
//        }
//            
//        if ( DIRECTION_LIGHT_SQUARE_WAVE == 0)
//            sensorVal_1 = 0;    
//        else
//            sensorVal_1 = 1;
//        
//        checkFrequencyRange(lowValueOfFrequency, highValueOfFrequency, sensorVal_1); 
        // LeonHuang20180313-}
    }
}

void Example_System_Init() 
{
#if     _XTAL_FREQ == 32000000
    OSCCON  = 0b11110000;       // PLL=on, IRCF=8 MHz, SCS=00 -> CONFIG1.FOSC
#elif   _XTAL_FREQ == 16000000
    OSCCON  = 0b01111000;       // PLL = off, IRCF = 16 MHz, SCS = 00 -> CONFIG1.FOSC
#else
    #error Unsupported clock frequency, edit _XTAL_FREQ in mTouch_config.h
#endif
  
    ANSELA      = 0b00000000;           // all inputs digital
    LATA        = 0b00110100;           // RA2=1, RA4=1, RA5=1      -? all led's off by default.
    TRISA       = 0b00010000;           // all PORT A configured as outputs
}

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
    if( DIRECTION_LIGHT_SQUARE_WAVE == 1){ 
		  TurningR_counter++; 
      TurningL_counter= 0;
	   }


	   if( DIRECTION_LIGHT_SQUARE_WAVE == 0){
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
// {LeonHuang20180313+ [Optimization] 
/* Timer0 20ms Setup 
 1. Processor frequency: 16MHz， 16M source edges per second
 2. Internal instruction cycle clock = Fosc / 4 = 16MHz / 4 = 4MHz
 3. 4M Internal instruction cycle per second, therefore 20ms has 80K Internal instruction cycle
 4. OPTION_REG, PSA = 1 means TMR0 rate = 1 : 1
 5. one Internal instruction cycle -> TMR0 + 1
 6. When TMR0 overflow (>255) -> INTCON, TMR0IF = 1
 80K / 256 = 312... 128 -> overflow 312 times, TMR0 = 128 (means it pass 20ms)
 TMR0 = 126 // 128 - 2 (Write data to TMR0 need 2 instruction cycle)
 */
void initTimer0()
{
    TMR0 = 126; // 128 - 2 
    OPTION_REGbits.T0CS = 0; // Internal instruction cycle clock (FOSC/4)
    OPTION_REGbits.PSA = 1; // If PSA = 1 (Prescaler Assigned to Watchdog Timer), TMR0 Rate = 1:1
    INTCONbits.T0IF = 0;
}

void checkFrequencyRange()
{
    // Count positive and negative 
    if (DIRECTION_LIGHT_SQUARE_WAVE == 0)
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
    if (tempNegativeVal > UPPER_BOUND) // > 83 count
    {
        tempNegativeVal = 0;
        negativeVal = 0;
    }
    if (tempPositiveVal > UPPER_BOUND) // > 83 count
    {
        tempPositiveVal = 0;
        positiveVal = 0;
    }

    // Check once per 200 ms
    freqCount++;
    if (freqCount >= 10) // 200 ms
    {
        unsigned char sum = negativeVal + positiveVal;
        if ((sum >= LOWER_BOUND) && (sum <= UPPER_BOUND))
        {
            putch('%');
            putch('0');
            putch('\r');
            putch('\n');
            //putch(DIRECTION_LIGHT_SQUARE_WAVE);
        }
        else
        {
            putch('%');
            putch('1');   
            putch('\r');
            putch('\n');
            //putch(DIRECTION_LIGHT_SQUARE_WAVE);
        }

        freqCount = 0;
    }
}
// LeonHuang20180313-}
/// @endcond

