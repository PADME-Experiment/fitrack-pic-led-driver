#define __16f88
#include<pic14/pic16f88.h>
#include<string.h>
#include<stdlib.h>


typedef unsigned int config;
config __at _CONFIG1 gCONFIG1 =
_LVP_OFF &  // RB3 is digital I/O, HV on MCLR must be used for programming.
_INTRC_IO &
//_INTRC_CLKOUT &
_MCLR_OFF &
_WDT_OFF  &
_CP_OFF;



#define RB_TRIG  RB0
#define RB_READY_soft RB1
#define RB_READY RB6
//#define RB_RX  RB2
#define RB_BAUD  RB3
#define RB_WRAP  RB4
//#define RB_TX  RB5
//#define RB_NWRAP RB7
#define RB_GATE  RB7




//           Blinking LED  ┐     ┌  Gate output
//        Blinking LED  ┐  │     │  ┌  READY
//     Blinking LED  ┐  │  │     │  │  ┌  TX RS232
//   Blinking LED ┐  │  │  │  +  │  │  │  ┌  Burst WRAP
//                ↑  ↑  ↑  ↑     ↑  ↑  ↑  ↑
//              ┌─┴──┴──┴──┴──┴──┴──┴──┴──┴─┐
//              │ 1  0  7  6  P  7  6  5  4 │
//              │             w             │
//              │    PORTA    S    PORTB    │
//              │             u             │
//              │ 2  3  4  5  p  0  1  2  3 │
//              └─┬──┬──┬──┬──┬──┬──┬──┬──┬─┘
//                ↓  ↓  ↓  ↑     ↑  ↓  ↑  ↓
//  Blinking LED  ┘  │  │  │  -  │  │  │  └  Baud
//     Blinking LED  ┘  │  │     │  │  └  RX RS232
//        Blinking LED  ┘  │     │  └  READY_soft
//                    N/C  ┘     └  Trigger input



//   run()    timer1=ON, timer2=ON
//   timer1   gate=OFF, READY=ON
//   timer2   timer2=OFF, timer0=ON
//   timer0   toggle led outputs
//            if (n_i>N) timer0=OFF


char uartRXi=0;
char uartTXi=0;
char uartTXlen=0;
char uartRXbuf[33];
char uartTXbuf[33];
char tmpstr[8];
unsigned int tmpint;

unsigned int nPeaks_i;
unsigned int t1postscale_i;

unsigned int nPeaks;
unsigned int t1postscale;

unsigned char portaMask;
unsigned char impOffset;
unsigned char impint;

void rs_char(char data){/*{{{*/
  uartTXlen=1;
  TXREG=data;
  TXEN=1;
}/*}}}*/
void rs_send(char* data){/*{{{*/
  uartTXi=0;
  uartTXbuf[0]='\n';
  uartTXbuf[1]='\r';
  uartTXlen=2+strlen(data);
  if(uartTXlen>27){
    rs_send("err");
  }else{
    strcpy(&(uartTXbuf[2]),data);
    uartTXbuf[uartTXlen++]='\n';
    uartTXbuf[uartTXlen++]='\r';
    uartTXbuf[uartTXlen++]='>';
    uartTXlen++;
    TXEN=1;
  }
}/*}}}*/
void wait(){//16 nop /*{{{*/
  __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
  __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
  __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
  __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
}/*}}}*/
char run(){/*{{{*/
  if(RB_READY){
    RB_READY=RB_READY_soft=0;

    TMR0IE=0; //disable TMR0

    TMR1H=TMR1L=0;  // clear counters
    t1postscale_i=0;
    TMR1ON=1;

    TMR2=(impOffset==0?PR2-1:0); // if 0ms offset, make tmr2 to finish immediately
    TMR2ON=1;

    RB_GATE=1;
    return 1;
  }
  return 0;
}/*}}}*/
char isFirstT1Call;

void main(void){
  // Global configuration/*{{{*/

  //int i=0;
  TRISA=0x0;
  TRISB=0x1;
  ANSEL=0;
  PORTA=PORTB=0;


  OSCCON=0b01101110;       // Fosc 4MHz
  IOFS=0; while(IOFS!=1);  // wait for stable frequency





  // Interrupts setting
  INTCON=0b01100000;  // GIE=OFF
  //       76543210
  //      76543210
  PIE1=0b0100011;
  //     6543210

  // watchdog
  SWDTEN=0; //disable watchdog
  WDTCON=0; // presc watchdog 1:32
  WDTCON=0b1000; // presc watchdog 1:512
  WDTCON=0b1110; // presc watchdog


  uartRXi=uartTXi=uartTXlen=uartRXbuf[0]=uartTXbuf[0]=uartRXbuf[1]=uartTXbuf[1]=0;

  nPeaks=10;
  portaMask=0b11011111;
  t1postscale=4; // ~1 s
  impOffset=3;
  impint=2;


  /*}}}*/
  // Configure Timer 0/*{{{*/

  INT0IE=1;  // enable interrupts on RB0 Trigger input

  OPTION_REG=0b01000000;
  //           76543210
  OPTION_REGbits.PS=impint;

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
  TMR2ON=0;
  /*}}}*/


  // Configure RS232/*{{{*/
  RCIE=1;  // RX IRQ
  SYNC=0;
  SPEN=1;
  TRISB2=TRISB5=1;
  CREN=1; //reception is enabled

  TXSTA=0b10000100;  // shift register status bit is off
  //      76543210
  RCSTA=0b10010000;
  //      76543210

  // 9k6 bps is not stable
  //SPBRG=25;  //9600
  SPBRG=207; //1200
  // 0xFF nop
  wait();wait(); wait();wait();
  wait();wait(); wait();wait();
  wait();wait(); wait();wait();
  wait();wait(); wait();wait();

  GIE=1;  //enable global interrupts

  rs_send("\nPic Led Driver\n");
  TXIE=1;
  while(TXEN);
  /*}}}*/

  RB_READY_soft=0;
  RB_READY=1;

  // delay before ready
  isFirstT1Call=1;

  TMR1H=TMR1L=0;  // clear counters
  t1postscale_i=0;
  TMR1ON=1;

  while(1){ }
}

static void interruptf(void) __interrupt 0 {
  // IRQ Timer 0/*{{{*/
  if(TMR0IF){
    TMR0IF=0;
    if(TMR0IE){
      RB_BAUD=1;
      PORTA=portaMask;
      PORTA=0x0;
      RB_BAUD=0;
      if((++nPeaks_i)>=nPeaks){
        TMR0IE=0;
        RB_WRAP=0;
        //RB_NWRAP=1;
      }
    }
  }
  /*}}}*/
  // IRQ Timer 1/*{{{*/
  if(TMR1IF){
    TMR1IF=0;
    if(TMR1ON){
      if((++t1postscale_i)>t1postscale){
        //TMR0IE=0; //disable TMR0
        RB_GATE=0;
        //TMR2ON=0;  should present
        TMR1ON=0;
        rs_send("READY");
        RB_READY=1;
        RB_READY_soft=(!isFirstT1Call);
        isFirstT1Call=0;
      }
    }
  }
  /*}}}*/
  // IRQ Timer 2/*{{{*/
  if(TMR2IF){
    TMR2IF=0;
    if(TMR2ON){
      RB_WRAP=1;  // this should be in the if
      TMR2ON=0;
      nPeaks_i=0;
      //RB_NWRAP=0;
      TMR0=125; // impulses in the middle of the wrap
      TMR0IE=1;
    }
  }
  /*}}}*/
  // IRQ AUSART Receive {{{
  if(RCIF&&RCIE){
    char tmp;
    tmp=RCREG;
    rs_char(tmp);
    switch (tmp){
      case '': case '': //backspace
        uartRXi-=(uartRXi>0?1:0);
        break;
      case '%':case '': //restart
        GIE=0;
        uartRXbuf[uartRXi=0]=0;
        SWDTEN=1; //enable watchdog
        break;
      case '@':case '': //make ready
               uartRXbuf[uartRXi=0]=0;
               RB_READY=RB_READY_soft=1;
               rs_send("Get READY");
               break;
      case '#':case '': //make busy
               uartRXbuf[uartRXi=0]=0;
               TMR1ON=0;
               RB_READY=RB_READY_soft=0;
               rs_send("Stay BUSY");
               break;
      case '!':case '	': //SW Trigger
               uartRXbuf[uartRXi=0]=0;
               if(run())
                 rs_send("Soft Trig");
               else
                 rs_send("BUSY: Ign STrig");
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
                     if(uartRXi>1)impint=(atoi(&(uartRXbuf[1]))&0b111);
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
                       case 'n':rs_send("num imp [0-32760]"); break;
                       case 'm':rs_send("porta mask [0-F]"); break;
                       case 't':rs_send("imp int (64*2^#)us [0-7]"); break;
                       case 'g':rs_send("gate ~(#/4)s [0-32768]"); break;
                       case 'o':rs_send("offs (#)ms [0-16]"); break;
                       case '1':rs_send("Press !,^I to self trig"); break;
                       case '2':rs_send("Press @,^F to make ready"); break;
                       case '3':rs_send("Press #,^C to make busy"); break;
                       case '5':rs_send("Press %,^R for Restart"); break;
                       default:
                                rs_send("[h?][nmtgo1235]");
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
               if(uartRXi>15){
                 rs_send("Too Long Instruction");
                 uartRXi=0;
               }
               uartRXbuf[uartRXi++]=tmp;
    }
    return;
  }
  //AUSART Receive }}}
  // IRQ AUSART Transmit {{{
  if(TXIF&&TXEN){
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
  // IRQ External Trigger RB0/*{{{*/
  if(INT0IF){
    INT0IF=0;
    if(RB_TRIG==1){
      if(run())
        rs_send("Ext Trig");
      else
        rs_send("BUSY: Ign ETrig");
    }
  }
  /*}}}*/
}
