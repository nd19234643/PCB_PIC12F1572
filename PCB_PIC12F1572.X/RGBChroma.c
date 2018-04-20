/*************************************************************************
 *   2014 Microchip Technology Inc.                                       
 *  
 *  Project Name:    RGBbadge Chroma
 *  FileName:        RGBChroma.c
 *  Dependencies:    configbits.h
 *  Processor:       PIC12F1572
 *  Compiler:        XC8 v1.30
 *  IDE:             MPLAB X IDE v2.05 or later
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
#define SETBAUD16H(bps)     ((_XTAL_FREQ)/( 4L * (bps)))   // BRGH = 1, BRG16 = 1
#define SETBAUD16(bps)      ((_XTAL_FREQ)/(16L * (bps)))   // BRGH = 0, BRG16 = 1
#define SETBAUD8H(bps)      ((_XTAL_FREQ)/(16L * (bps)))   // BRGH = 1, BRG16 = 0
#define SETBAUD8(bps)       ((_XTAL_FREQ)/(64L * (bps)))   // BRGH = 0, BRG16 = 0
#define AUTO_BAUD_RATE_DETECTION // LeonHuang20180419, [Automatic detection and calibration of the baud rate] 

//#define INTER_CHARACTER_TIMEOUT 4   // greater than (4) 100 ms ticks = 500 ms
//#define ASCII_ACK               0x06    // keep alive reponse from PC

//uint8_t   rxBytes[RXDATASIZE] = { 0 };

/*** Reference PIC12F1571-2 Data Sheet, Chapter 21.0 - EUSART ***/
// Enhanced Universal Synchronous Asynchronous Receiver Transmitter (EUSART)
// When the receiver or transmitter section is not enabled, then the corresponding RX or TX pin may be used for general purpose input and output (GPIO).
void initEUSART()
{
    // ================
    // Configure EUSART
    // ================
    // BRGH = 1, BRG16 = 1, Fosc = 16 MHz, Desired Baud Rate = 9600 (14400, 19200, 38400, 57600, 115200) 
    // Desired Baud Rate = Fosc / 4 (SPBRG + 1)
    // SPBRG = (Fosc / 4 * Desired Baud Rate) - 1
    // SPBRG = 16000000 / 4 * 9600 - 1 = 415.66

    unsigned char temp;
    
    // steer specific peripheral input and output functions between different pins
    APFCON0bits.TXCKSEL  = 0;               // TX/CK function is on RA0
    APFCON0bits.RXDTSEL  = 1;               // RX/DT function is on RA5
    
    // PORTA pin is input, output
    TRISAbits.TRISA5    = 1;                // configure RA5 as input
    TRISAbits.TRISA0    = 0;                // configure RA0 as output
    
    /*
        The operation of the EUSART module is controlled through three registers:
        • Transmit Status and Control (TXSTA)
        • Receive Status and Control (RCSTA)
        • Baud Rate Control (BAUDCON)
     */
    BAUDCONbits.BRG16   = 1;                // 16-bit Baud Rate Generator is used
    TXSTAbits.BRGH      = 1;                // High speed Baud Rate

    TXSTAbits.SYNC      = 0;                // EUSART Mode : Asynchronous mode
    RCSTAbits.SPEN      = 1;                // Serial port is enabled (configures RX/DT and TX/CK pins as serial port pins)
    TXSTAbits.TXEN      = 1;                // Transmit is enabled
    RCSTAbits.CREN      = 1;                // Enables receiver
    
    // {LeonHuang20180419+ [Automatic detection and calibration of the baud rate] 
#ifdef AUTO_BAUD_RATE_DETECTION
AUTOBAUD:
    BAUDCONbits.ABDEN   = 1;                // Starts the auto-baud calibration sequence
    while(!PIR1bits.RCIF);                  // Waiting for USART incoming U
    temp = RCREG;                           // Must read RCREG to clear RCIF
    
    while(!PIR1bits.RCIF);                  // Waiting for USART incoming byte 'U' (for real this time)
    temp = RCREG;
    if (temp != 'U')
        goto AUTOBAUD;
#else
    SPBRG               = SETBAUD16H(9600); // Set Baud Rate 9600
#endif
    // LeonHuang20180419-}
    
    PIE1bits.RCIE       = 1;                // Enables the USART receive interrupt
    INTCONbits.GIE      = 1;                // Enables all active interrupts
    INTCONbits.PEIE     = 1;                // Enables all active peripheral interrupts
}

void EUSARTShutdown()
{
    // ================
    // Configure EUSART
    // ================
    RCSTAbits.SPEN      = 0;                // turn on serial port.
    PIR1bits.RCIF       = 0;                // make sure receive interrupt flag is clear.
    TRISAbits.TRISA1    = 0;                // return Rx to an output for mTouch

}


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
    if (1 == RCSTAbits.SPEN) // Serial Port is enable
    {
        // The TXIF flag is set when the TXEN enable bit is set.
        // The TXIF flag bit will be set whenever the TXREG is empty
        while (!TXIF) continue;
        TXREG = data;
    }
}

