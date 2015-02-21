#define __16f88
#include<pic14/pic16f88.h>
#include<string.h>
#include<stdlib.h>
//strcpy & strlen
//atoi & itoa


#define RB_TRIG  RB0
#define RB_READY RB1
//#define RB_RX  RB2
#define RB_BAUD  RB3
#define RB_WRAP  RB4
//#define RB_TX  RB5
#define RB_NWRAP RB6
#define RB_GATE  RB7


// On trigger signal Timer1 and Timer2 are started
// simultaneously.
// Timer2, when finish, starts Timer0.
// Timer0 is the one responsible of the PORTA pins
// toggleing.
// Timer0 makes nPeaks spikes on PORTA.
// If Timer1 finish before Timer0 or Timer1 it switches off
// everything.
//
// External triggers are accepted on RB0.



typedef unsigned int config;
config __at _CONFIG1 gCONFIG1 =
_LVP_OFF &  // RB3 is digital I/O, HV on MCLR must be used for programming.
_INTRC_IO & 
_MCLR_OFF &
_WDT_OFF  &
_CP_OFF;


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
unsigned char portaMask=0b11011111;
unsigned char impOffset=0; // eg 1ms
unsigned char impint=0;
unsigned char t1postscale=4;

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
void run(){/*{{{*/
  //TMR2=0;
  RB4=1;          // Gate out
  RB3=0;          // Anti gate
  TMR1H=TMR1L=0;  // clear counters
  t1postscale_i=0;
  TMR1ON=1;
  TMR2ON=1;
  TMR2=(impOffset==0?PR2-1:0); // if 0ms offset, make tmr2 to finish immediately
}/*}}}*/

void main(void){
  // Global configuration/*{{{*/

  //int i=0;
  TRISA=0x0;
  TRISB=0x1;
  PORTA=PORTB=0;

  OSCCON=0b01101110;       // Fosc 4MHz
  IOFS=0; while(IOFS!=1);  // wait for stable frequency

  // Interrupts setting
  INTCON=0b11100000;
  //       76543210
  PIE1=0b0110011;
  //     6543210

  // watchdog
  SWDTEN=0; //disable watchdog
  WDTCON=0; // presc watchdog 1:32

  /*}}}*/
  // Configure Timer 0/*{{{*/

  INT0IE=1;  // enable interrupts on RB0 Debug only
  PSA=0; // prescaler is assigned to the Timer0
  OPTION_REGbits.PS=impint;

  T0CS=0; //TMR0 Internal instruction cycle

  /*}}}*/
  // Configure Timer 1/*{{{*/
  TMR1H=TMR1L=0;  // clear counters
  T1CON=0b00100100;  // presc 1:4
  //    0b76543210

  TMR1ON=0; //T1CON|=0b00000001;  // enable timer 1
  TMR1IE=1;
  /*}}}*/
  // Configure Timer 2/*{{{*/
  TMR2=0;
  TMR2IE=1;
  T2CON=0b00000001; // t2 is off, postscale 1:1, prescale 1:4
  T2CON&=0b111;T2CON|=impOffset*0b1000; // preserves last 3 bits and changes first four
  PR2=250; // t2 period
  /*}}}*/
  // Configure RS232/*{{{*/
  SYNC=0;
  SPEN=1;
  TRISB|=0b100100; // 5 & 2=1
  CREN=1; //reception is enabled

  TXSTA=0b10000110;
  RCSTA=0b10010000;

  // 9k6 bps is not stable
  //  SPBRG=25;  //9600
  SPBRG=207; //1200

  rs_send("\nPic Led Driver\n");
  TXIE=1;
  /*}}}*/
  while(1){ }
}


static void interruptf(void) __interrupt 0 {
  // IRQ Timer 0/*{{{*/
  if(TMR0IF){
    TMR0IF=0;
    if(
        (++nPeaks_i)>nPeaks
        //&&nPeaks>0
      ){
      TMR0IE=0;
      RB6=0;
      RB1=1;  // anti RB6
    }else{
      //RB6=1; //TODO test only

      RB7=1;
      PORTA=portaMask;
      PORTA=0x0;
      RB7=0;
    }
    return;
  }
  /*}}}*/
  // IRQ Timer 1/*{{{*/
  if(TMR1IF){
    TMR1IF=0;
    if((++t1postscale_i)>t1postscale){
      TMR1ON=0;
      TMR2ON=0;
      TMR0IE=0; //disable TMR0
      RB7=0;
      RB6=0;
      RB1=1;  // anti RB6
      RB4=0;    // switch off gate
      RB3=1;    // Anti gate
      rs_send("Tout");
    }
    return;
  }
  /*}}}*/
  // IRQ Timer 2/*{{{*/
  if(TMR2IF){
    TMR2IF=0;
    TMR2ON=0;
    nPeaks_i=0;
    RB6=1;
    RB1=0;  // anti RB6
    TMR0=0;
    TMR0IE=1;
    return;
  }
  /*}}}*/
  // IRQ External Trigger RB0/*{{{*/
  if(INT0IF){
    INT0IF=0;
    if(RB0==1){
      run();
      rs_send("Ext T");
    }
    return;
  }
  /*}}}*/
  // IRQ AUSART Receive {{{
  if(RCIF){
    char tmp;
    tmp=RCREG;
    rs_char(tmp);
    switch (tmp){
      case '': case '': //backspace
        uartRXi-=(uartRXi>0?1:0);
        break;
      case '@': //restart
        SWDTEN=1; //enable watchdog
        break;
      case '!': //SW Trigger
        run();
        rs_send("Soft Trig");
        break;
      case '\r': case '\n':
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
              if(uartRXi>1)portaMask=atoi(&(uartRXbuf[1]))&0b11011111; // RA5 is allways input
              _itoa(portaMask,tmpstr,2);
              rs_send(tmpstr);
              break;

            case 't': //impulse interval
              if(uartRXi>1)impint=atoi(&(uartRXbuf[1]));
              _itoa(64<<impint,tmpstr,10);
              OPTION_REGbits.PS=impint;
              rs_send(tmpstr);
              break;

            case 'g': //Gate
              if(uartRXi>1)t1postscale=atoi(&(uartRXbuf[1]));
              _itoa(t1postscale>>2,tmpstr,10);
              rs_send(tmpstr);
              break;

            case 'o': //offset for the imp
              if(uartRXi>1){
                impOffset=atoi(&(uartRXbuf[1]));
                if(impOffset>16)impOffset=16;
              }
              _itoa(impOffset,tmpstr,10);
              T2CON&=0b111;T2CON|=(impOffset>0?impOffset-1:impOffset)*0b1000; // preserves last 3 bits and changes first four
              rs_send(tmpstr);
              break;

            case 'l':  // for ls
              rs_send("not Linux!\n\rh for help");
              break;

            case '?': case 'h': //help
              switch (uartRXbuf[1]){
                case 'n':rs_send("num imp"); break;
                case 'm':rs_send("porta mask"); break;
                case 't':rs_send("imp int (64^#)us"); break;
                case 'g':rs_send("gate ~(#/4)s"); break;
                case 'o':rs_send("offs (#)ms"); break;
                default:
                         rs_send("[h?][nmtgo]");
              }
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
  // IRQ AUSART Transmit {{{
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
}
