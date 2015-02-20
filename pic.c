//#define __16f88
#include <pic14/pic12f683.h>

typedef unsigned int config;
config __at _CONFIG CONFIG =
_CP_OFF &
_MCLRE_OFF&

//_INTRC_IO & 
////_EXTRC_IO &
//_MCLR_OFF &
_INTOSCIO &
_WDT_OFF  &
//_FOSC_EC;
0xFFFF;



void delay2us(){
  __asm__("nop");
}


void delay1ms(){
  int i;
  for(i=0;i<52;++i);
}

void delay10ms(){
  int i;
  for(i=0;i<524;++i);
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
}

void delay100ms(){
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
  delay10ms();
}

void delay1s(){
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
  delay100ms();
}


void delay200us(){
  int i;
  for(i=0;i<10;++i);
}


void main(void){
  //TRISA = 0b00000010; // 0 - output , 1 - input. here RA3 as input and rest as output. 
  //TRISB = 0b00000000; // 0 - output , 1 - input. here RA3 as input and rest as output. 
  ANSEL=0;
  TRISIO=0b1000;
  GPIO=0;
  IOC=0;
  while(1){
    if(GP3){
      GPIO=0xF;
      GPIO=0;
      delay1ms();
    }
  }
}

