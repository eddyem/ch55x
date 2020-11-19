
/********************************** (C) COPYRIGHT *******************************
* File Name          : TouchKey.C
* Author             : WCH
* Version            : V1.1
* Date               : 2017/07/05
* Description        : CH554Touch button sampling interval setting, channel selection and switching, and interrupt processing function
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>

#include "ch554.h"
#include "debug.h"
#include "touchkey.h"

uint16_t KeyFree[KEY_LAST-KEY_FIRST+1];                                        //Touch idle value storage, used to compare the state of the key
volatile uint8_t KeyBuf = 0;                                                               //Touch button status, 0 means no button, non-zero means currently detected button is pressed
volatile uint16_t KeyData;

/*******************************************************************************
* Function Name  : GetTouchKeyFree()
* Description    : Get the value of the touch button idle state
* Input          : None								 
* Output         : None
* Return         : None
*******************************************************************************/
void GetTouchKeyFree()
{
    uint8_t i, j;
    for(i = KEY_FIRST; i<= KEY_LAST; ++i)
        KeyFree[i-KEY_FIRST] = 0;
    for(j = 0; j < 4; ++j){
        for(i = KEY_FIRST; i<= KEY_LAST; ++i){
            TKEY_CTRL = ((TKEY_CTRL & 0xF8) | i) + 1;
            while((TKEY_CTRL & bTKC_IF) == 0);
            uint16_t val = TKEY_DAT & 0x3FFF;
            KeyFree[i-KEY_FIRST] += val;
        }
    }
    for(i = KEY_FIRST; i<= KEY_LAST; ++i)
        KeyFree[i-KEY_FIRST] /= 4;
#if INTERRUPT_TouchKey
    IE_TKEY = 1;                                                              //Enable Touch_Key interrupt
#endif   
}

/*******************************************************************************
* Function Name  : TouchKeyChannelSelect(uint8_t ch)
* Description    : Touch key channel selection
* Input          : uint8_t ch Use channel
                   0~5 Representing sampling channels
* Output         : None
* Return         : success 1
                   failure 0  Unsupported channel
*******************************************************************************/
uint8_t TouchKeyChannelSelect(uint8_t ch)
{
    KeyBuf = 0;
    if(ch < 6)
    {
        TKEY_CTRL = ((TKEY_CTRL & 0xF8) | ch)+1;
        return 1;
    }
    return 0;
}

#if INTERRUPT_TouchKey
/*******************************************************************************
* Function Name  : TouchKeyInterrupt(void)
* Description    : Touch_Key Interrupt service routine
*******************************************************************************/
void	TouchKeyInterrupt( void ) interrupt INT_NO_TKEY using 1                //Touch_Key interrupt service routine, use register set 1
{ 
          uint8_t	ch;
    uint16_t KeyData;

    KeyData = TKEY_DAT & 0x3fff;                                                       //Keep 87us, take it away as soon as possible
    ch = TKEY_CTRL&7;                                                         //Get current sampling channel
    if ( ch > KEY_LAST ){
       TKEY_CTRL = ((TKEY_CTRL & 0xF8) | KEY_FIRST) + 1;
    }			
    else
    {
       TKEY_CTRL ++;                                                          //Switch to the next sampling channel
    }
    if ( KeyData < (KeyFree[ch-KEY_FIRST] - KEY_ACT) )                        //If the condition is met, it means that the key is pressed
    {
        KeyBuf=ch;                                                            //You can perform key action processing here or set a flag to notify main for processing
    }
}
#else
/*******************************************************************************
* Function Name  : TouchKeyChannelQuery()
* Description    : Touch button channel status query
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TouchKeyChannelQuery()
{
          uint8_t	ch;

    while((TKEY_CTRL&bTKC_IF) == 0);                                          //When bTKC_IF becomes 1, the sampling of this cycle is completed
    KeyData = TKEY_DAT & 0x3FFF;                                                       //Keep 87us, take it away as soon as possible
    ch = (TKEY_CTRL&7) - 1;                                                         //Get current sampling channel
    if ( ch == KEY_LAST ){
       TKEY_CTRL = (TKEY_CTRL & 0xF8 | KEY_FIRST) + 1;                              //Start sampling from the first channel
    }
    else
    {
       ++TKEY_CTRL;                                                          //Switch to the next sampling channel
    }
    if ( KeyData < (KeyFree[ch-KEY_FIRST] - KEY_ACT) )                        //If the condition is met, it means that the key is pressed
    {
        KeyBuf=ch;                                                            //You can perform key action processing here or set a flag to notify main for processing
    }
}
#endif

