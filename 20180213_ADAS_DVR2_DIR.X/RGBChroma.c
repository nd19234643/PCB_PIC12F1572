/*************************************************************************
 *  � 2014 Microchip Technology Inc.                                       
 *  
 *  Project Name:    RGBbadge Chroma
 *  FileName:        RGBChroma.c
 *  Dependencies:    configbits.h
 *  Processor:       PIC12F1572
 *  Compiler:        XC8 v1.30
 *  IDE:             MPLAB� X IDE v2.05 or later
 *  Hardware:        RGBbadge demonstration board 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Description:     Main routine
 *                   - Example of specifying RGB LED Color based on
 *                      serial commands providing by the Chromaticity
 *                      Selector PC application
 *************************************************************************/
/*************************************************************************
 * MICROCHIP SOFTWARE NOTICE AND DISCLAIMER: You may use this software, and
 * any derivatives created by any person or entity by or on your behalf,
 * exclusively with Microchip's products in accordance with applicable
 * software license terms and conditions, a copy of which is provided for
 * your referencein accompanying documentation. Microchip and its licensors
 * retain all ownership and intellectual property rights in the
 * accompanying software and in all derivatives hereto.
 *
 * This software and any accompanying information is for suggestion only.
 * It does not modify Microchip's standard warranty for its products. You
 * agree that you are solely responsible for testing the software and
 * determining its suitability. Microchip has no obligation to modify,
 * test, certify, or support the software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE, ITS INTERACTION WITH
 * MICROCHIP'S PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY
 * APPLICATION.
 *
 * IN NO EVENT, WILL MICROCHIP BE LIABLE, WHETHER IN CONTRACT, WARRANTY,
 * TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), STRICT
 * LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE,
 * FOR COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE,
 * HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY
 * OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWABLE BY LAW,
 * MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS
 * SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID
 * DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF
 * THESE TERMS.
 *************************************************************************/
/** @file   RGBChroma.c
*   @brief  Example implementation of RGB Color Mixing
*/
/// @cond

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <xc.h>
//#include "mTouch/mTouch_config.h"
#include "RGBChroma.h"
//#include "RGBSlider.h"



//#define ASCII_DATA
#if defined(ASCII_DATA)
    #define RXDATASIZE      12  // 3 16-bit ASCII values for RED, GRN, BLU
#else // ASCII_DATA
    #define RXDATASIZE      6   // 3 16-bit integer values for RED, GRN, BLU
#endif // ASCII_DATA

#define _XTAL_FREQ   16000000  //Used by the XC8 delay_ms(x) macro
#define INTER_CHARACTER_TIMEOUT 4   // greater than (4) 100 ms ticks = 500 ms
#define ASCII_ACK               0x06    // keep alive reponse from PC


uint8_t   rxBytes[RXDATASIZE] = { 0 };


void EUSARTInit()
{

    // ================
    // Configure EUSART
    // ================
    // when BRG16 == 1, and BRGH == 1
    // baud                     = FOSC /(4 * (n+1))
    // (4 * baud) / FOSC        = 1 / (n+1)
    // FOSC / (4 * baud)           = n + 1
    // ( FOSC / (4 * baud)) - 1    = n

// TODO: move this
#define SETBAUD16H(bps)     ((_XTAL_FREQ)/( 4L * (bps)))   // BRGH=1, BRG16=1
#define SETBAUD16(bps)      ((_XTAL_FREQ)/(16L * (bps)))   // BRGH=0, BRG16=1
#define SETBAUD8H(bps)      ((_XTAL_FREQ)/(16L * (bps)))   // BRGH=1, BRG16=0
#define SETBAUD8(bps)       ((_XTAL_FREQ)/(64L * (bps)))   // BRGH=0, BRG16=0
    APFCON0bits.TXCKSEL  = 0;               //TX==>RA0 //Tsunghua 20180213
    APFCON0bits.RXDTSEL  = 1;               //RX==>RA5 //Tsunghua 20180213
    TRISAbits.TRISA5    = 1;                // set Rx to input //Tsunghua 20180213
    TRISAbits.TRISA0    = 0;                // Make UART TX pin output
    BAUDCONbits.BRG16   = 1;                // 16-bit baud rate generator
    TXSTAbits.BRGH      = 1;                // high speed baud
    SPBRG               = SETBAUD16H(9600); // set serial baud rate 9600 - FOSC=16MHz
    TXSTAbits.TXEN      = 1;                // enable serial transmissions.
    RCSTAbits.CREN      = 1;                // enable reception.
    RCSTAbits.SPEN      = 1;                // turn on serial port.
    TXSTAbits.SYNC      = 0;                //0 = Asynchronous mode

    PIE1bits.RCIE       = 1;                // 1 = Enables the EUSART receive interrupt
    INTCONbits.GIE      = 1;                // enable interrupts
    INTCONbits.PEIE     = 1;    

    
} // end EUSARTInit()



void EUSARTShutdown()
{

    // ================
    // Configure EUSART
    // ================
    RCSTAbits.SPEN      = 0;                // turn on serial port.
    PIR1bits.RCIF       = 0;                // make sure receive interrupt flag is clear.
    TRISAbits.TRISA1    = 0;                // return Rx to an output for mTouch

} // end EUSARTShutDown()



uint8_t receiveRGB()
{
 /*   uint8_t     dataAvailable   = PIR1bits.RCIF;
    uint8_t     rxCount         = 0;
#if defined(ASCII_DATA)
    uint8_t     tmpRx           = 0;
#endif // ASCII_DATA
    static unsigned int    loopCount   = 0;
    

    // if data available, process it
    while( dataAvailable )
    {
        // if byte has been received by the EUSART, process it

        serviceRGBTimer( true);

#if !defined ASCII_DATA
        rxBytes[ rxCount++ ]    = RCREG;
        
#else // ASCII_DATA
        // the data received is ASCII formatted hexadecimal
        // get the ASCII byte and convert it to the actual binary value
        tmpRx                   = RCREG;


        // minimal ASCII to hexadecimal conversion - NO error detection
        tmpRx       -= '0';     // adjust for values 0 to 9
        if( 9 < tmpRx  )
        {
            tmpRx   -= 7;       // adjust for values A to F
            if( 0xf < tmpRx )
            {
                tmpRx   -= 0x20;
            }
        }

        if(0x01 & rxCount)
        {
            rxBytes[ (rxCount >> 1) ]    |= tmpRx;        //
        }
        else
        {
            rxBytes[ (rxCount >> 1) ]     = tmpRx << 4;   // store upper nibble
        }
        rxCount++;
#endif // ASCII_DATA

        if( RXDATASIZE == rxCount )
        {
            break;      // all expected data received
        }
            
        // wait for next character received or timeout
        while( !PIR1bits.RCIF && (INTER_CHARACTER_TIMEOUT > ticks))
        {
            if(PIR1bits.RCIF) 
            {
                break;      // new character received - stop waiting
            }
            serviceRGBTimer(false);
        }

        if( !PIR1bits.RCIF )
        {
            // data timeout error
            dataAvailable = false;

            // test if this was not the keep alive acknowledge
            if((1 != rxCount) || (ASCII_ACK != rxBytes[0]))
            {
                // reset EUSART and send error signal 'E'
                RCSTAbits.SPEN  = 0;            // turn off serial port to clear error
                NOP();
                RCSTAbits.SPEN  = 1;            // turn on serial port.
                putch('E');
            }
        }

    } // end while( dataAvailable )

    if( dataAvailable )
    {


        // only 8-bit duty cycle values are supported in colorMix
        redDC       = rxBytes[0];
        greenDC     = rxBytes[2];
        blueDC      = rxBytes[4];
        colorMix();

    }
    else
    {
        if( 0 == ++loopCount )
        {
            putch('.');                     // show continued operation
        }
    }

    return dataAvailable;
*/
} // end receiveRGB()


/*
uint8_t USBSignalling(void)
{
    // test if USB to serial converter is asserting the RX line
    uint8_t lastRXTRIS      = RGB_RX_TRISbit;
    uint8_t lastRXANSEL     = RGB_RX_ANSELbit;
    uint8_t eusartActive;

    RGB_RX_ANSELbit     = 0;            // configure as digital
    RGB_RX_TRISbit      = 1;            // configure RX line as input
    NOP();
    NOP();
    NOP();
    // bct RGB_RX_TRISbit      = 0;            // configure RX line as output
    // bct RGB_RX_LATbit       = 1;    // toggle for test
    // bct RGB_RX_LATbit       = 0;    // toggle for test
    eusartActive        = RGB_RXpin;    // is RX line asserted?
    RGB_RX_TRISbit      = lastRXTRIS;   // restore TRIS configuration
    RGB_RX_ANSELbit     = lastRXANSEL;  // restore ANSEL configuration

    return (eusartActive);
}
*/


// this directs printf output to the serial port.
void putch(char data) 
{
    if( 1 == RCSTAbits.SPEN )
    {
        while( ! TXIF) continue;
        TXREG = data;
    }
}

