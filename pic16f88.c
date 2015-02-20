#define __16f88
#include <pic14/pic16f88.h>
#include<string.h>
#include <stdlib.h>
  //strcpy & strlen
  //atoi & itoa

typedef unsigned int config;
config __at _CONFIG1 gCONFIG1 =
_LVP_OFF &  // RB3 is digital I/O, HV on MCLR must be used for programming.
_INTRC_IO & 
_MCLR_OFF &
_WDT_OFF  &
_CP_OFF ;




char uartRXi=0;
char uartTXi=0;
char uartTXlen=0;
char uartRXbuf[24];
char uartTXbuf[24];
char tmpstr[8];
unsigned int tmpint;


unsigned int nPeaks_i;
unsigned int t1postscale_i;

unsigned int  nPeaks=100;
unsigned char portaMask=0xff;
unsigned char impOffset=0; // eg 1ms
unsigned char impint=0;
unsigned char t1postscale=20;

#define RESTART SWDTEN=1;

void rs_char(char data){/*{{{*/
  uartTXlen=1;
  TXREG=data;
  TXEN=1;
}/*}}}*/
void rs_send(char* data){/*{{{*/
  uartTXi=0;
  uartTXbuf[0]='\n';
  uartTXbuf[1]='\r';
  strcpy(&(uartTXbuf[2]),data);
  uartTXlen=2+strlen(data);
  uartTXbuf[uartTXlen++]='\n';
  uartTXbuf[uartTXlen++]='\r';
  uartTXbuf[uartTXlen++]='>';
  uartTXlen++;
  TXEN=1;
}/*}}}*/
void wait(){/*{{{*/
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
}/*}}}*/


void main(void){
  int i=0;
  TRISA=0x0;
  TRISB=0x1;
  PORTA=PORTB=0;

  OSCCON=0b01101110;

  IOFS=0;
  while(IOFS!=1);

  //gCONFIG1&=0b11111011; //disable watchdog
  SWDTEN=0; //disable watchdog
  WDTCON=0; // presc watchdog 1:32




  // Timer 0 {{{
  T0CS=0; //TMR0 Internal instruction cycle
  // Timer 0 }}}

  INTCON=0b11100000;
  //       76543210
  //
  PIE1=0b0110011;
  //    76543210

  INT0IE=1;  // enable interrupts on RB0 Debug only
  PSA=0; // prescaler is assigned to the Timer0
  OPTION_REGbits.PS=impint;


  // Timer 1 {{{
  TMR1H=TMR1L=0;  // clear counters
  T1CON=0b00100000;  // presc 1:4

  TMR1ON=0; //T1CON|=0b00000001;  // enable timer 1
  TMR1IE=1;

  // Timer 1 }}}
  // Timer 2 {{{
  TMR2=0;
  T2CON=0b00000001; // t2 is off, postscale 1:1, prescale 1:4
  T2CON&=0b111;T2CON|=impOffset*0b1000; // preserves last 3 bits and changes first four
  PR2=250; // t2 period
  // Timer 2 }}}
  // RS232 config{{{

  SYNC=0;
  SPEN=1;
  TRISB|=0b111100;
  CREN=1; //reception is enabled

  TXSTA=0b10000110;
  RCSTA=0b10010000;
  SPBRG=25; //9600
  SPBRG=207; //1200

  rs_send("\nLed Driver\n");
  TXIE=1;
  // RS232 }}}

  while(1){
  }
}








static void interruptf(void) __interrupt 0 {

  if(TMR0IF) {
    TMR0IF=0;
    if((nPeaks_i++)<=nPeaks){
      PORTA=portaMask;
      PORTA=0x0;
    }
    return;
  }

  if(TMR1IF){
    TMR1IF=0;
    if((t1postscale_i++)>t1postscale){
      TMR1ON=0;
      TMR2ON=0;
      TMR0IE=0; //disable TMR0
      RB7=0;    // switch off gate
      rs_send("Tout");
    }
    return;
  }

  if(TMR2IF){
    TMR2IF=0;
    TMR2ON=0;
    TMR0IE=1;
    nPeaks_i=0;
    return;
  }

  if(INT0IF){  // external gate
    INT0IF=0;
    if(RB0==1){
      TMR2=0;
      RB7=1;    // Gate out
      TMR1H=TMR1L=0;  // clear counters
      t1postscale_i=0;
      TMR1ON=1;
      TMR2ON=1;
    }
    rs_send("Ext T");
    return;
  }

  // Receive just after the transmit part
  // AUSART Receive {{{
  if(RCIF){
    char tmp;
    tmp=RCREG;
    rs_char(tmp);
    switch (tmp){
      case '': case '': //backspace
        uartRXi-=(uartRXi>0?1:0);
        break;
      case '@':
        RESTART;
        break;
      case '!': //start
        TMR2=0;
        RB7=1;    // Gate out
        TMR1H=TMR1L=0;  // clear counters
        t1postscale_i=0;
        TMR1ON=1;
        TMR2ON=1;
        rs_send("Self Trig");
        break;
      case '\r':
      case '\n':
        if(uartRXi>0){
          uartRXbuf[uartRXi]=0;
          switch (uartRXbuf[0]){
            case '\r':
              rs_send("");
              break;

            case '\n':
              rs_send("main nn\r\n");
              break;

            case 'n':
              if(uartRXi>1)nPeaks=atoi(&(uartRXbuf[1]));
              _itoa(nPeaks,tmpstr,10);
              rs_send(tmpstr);
              break;

            case 'm': // PORTA mask
              if(uartRXi>1)portaMask=atoi(&(uartRXbuf[1]))&0b11011111;
              _itoa(portaMask,tmpstr,2);
              rs_send(tmpstr);
              break;

            case 'i': //impulse interval
              if(uartRXi>1)impint=atoi(&(uartRXbuf[1]));
              _itoa(64<<impint,tmpstr,10);
              OPTION_REGbits.PS=impint;
              rs_send(tmpstr);
              break;

            case 'o': //offset for the imp, in ms 0=1ms, 15=16ms
              if(uartRXi>1)impOffset=atoi(&(uartRXbuf[1]));
              _itoa(impOffset+1,tmpstr,2);
              rs_send(tmpstr);
              T2CON&=0b111;T2CON|=impOffset*0b1000; // preserves last 3 bits and changes first four
              break;
            case 'G': //Gate
              if(uartRXi>1)t1postscale=atoi(&(uartRXbuf[1]));
              _itoa(t1postscale>>2,tmpstr,10);
              rs_send(tmpstr);
              break;
            case '?': case 'h': //help
              switch (uartRXbuf[1]){
                case 'n':rs_send("num imp"); break;
                case 'm':rs_send("porta mask"); break;
                case 'i':rs_send("imp int (64<<#)us"); break;
                case 'G':rs_send("gate ~(#/4)s"); break;
                case 'S':rs_send("Start w/o gate"); break;
                case 'o':rs_send("offs 0=1ms 15=16ms"); break;
                default:
                         rs_send("[h?][nmiGSo]");
              }
              break;
            case 'S': //start
              TMR2=0;
              RB7=1;    // Gate out
              TMR1H=TMR1L=0;  // clear counters
              t1postscale_i=0;
              TMR1ON=1;
              TMR2ON=1;
              rs_send("Self Trig");
              break;
            default:
              rs_send("N/A cmd");
          }
          uartRXi=0;
        }else{
          rs_send("");
        }
        break;
      default:
        uartRXbuf[uartRXi++]=tmp;
        if(uartRXi>13){
          rs_send("TooLongInstruct");
          uartRXi=0;
        }
    }
    return;
  }
  //AUSART Receive }}}

  // AUSART Transmit {{{
  if(TXIF){
    if(uartTXlen>0){
      if(uartTXi<uartTXlen){
        TXREG=uartTXbuf[uartTXi++];
      }else{
        uartTXlen=0;
        uartTXi=0;
      }
    }else{
      TXEN=0;
    }
    return;
  }
  // AUSART Transmit }}}

  return;
}
