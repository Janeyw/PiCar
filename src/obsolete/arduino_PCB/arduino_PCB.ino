//  Created by Matt Kollada on 5/17/17.
//PCB
//#include <stdint.h>
#include <Servo.h>
#include <SPI.h>
#include "dshare.h"
#include "ddefs.h"
#include "spi_comm.h"
#include <avr/interrupt.h>
//FIX ME: see timer.c file
#include "timers.h"
#include "imu.h"
#include "fp.h"

Servo servo;
Servo esc;

float SPEED_SCALE = 1.5;
int tempAngle = 90;
int tempPWM = 0;

bool kill;

volatile uint16_t raising;
volatile uint16_t diff;
volatile uint8_t redge;
volatile uint8_t newfall;

// Kill Switch Interrupt
// FIX ME: use ICP3 not 1
ISR( TIMER3_CAPT_vect )
{
  Serial.println("abc");
  if( !newfall )
    if( redge )
    {
      bitClear( TCCR3B, ICES3 );
      raising = ICR3;
      redge = 0;
    }
    else
    {
      bitSet( TCCR3B, ICES3 );
      diff =  ;
      redge = 1;
      newfall = 1;
    }
}


// Servo Pwm Update interrupt
// FIX ME: use other timer
ISR(TIMER0_COMPA_vect)
{
  if(kill) {
    servo.write(90);
    esc.writeMicroseconds(1500);
  }
  else {
    if ( getData( SERVO_ANGLE, &tempAngle ) == DSHARE_OK ) {
        servo.write(tempAngle);
    }
    if ( getData( BLDC_DUTY_CYCLE, &tempPWM ) == DSHARE_OK ) {
        //run PID on PWM
        esc.writeMicroseconds(1500 + SPEED_SCALE*(float)tempPWM);
    }
  }
}

// SPI Interrupt (dealing with issue with SPSR)
ISR(SPI_STC_vect) {
    spiHandler();
}

//// IMU Interrupt
//ISR(TIMER3_COMPA_vect) {
//  //read IMU and set global data structure
//  getIMUData();
//}

void setup() {
  //open serial port for debugging
  Serial.begin(57600);
  
  //setup SPI
  SPI.attachInterrupt();
  pinMode(MISO, OUTPUT);
  SPCR |= _BV(SPE);

  //Initialize timers
  //initTimers();

  
  
  // setup esc and servo
  // FIX ME: pins change for leonardo
  servo.attach( 9 );
  esc.attach( 10 );

  // Turn kill switch off
  kill = false;

  //setup IMU
  imuSetup();


  // ICES1: 0: falling edge, 1: raising edge
  // ICNC1: Noise cancel enabled
  bitSet( TCCR3B, ICNC3 );
  bitSet( TCCR3B, ICES3 );
  bitSet( TIMSK3, ICIE3 ); // IC interrupt enabled
  redge = 1;
  newfall = 0;

} 

void loop() {
//  Serial.println(tempAngle);
//  Serial.println(SPEED_SCALE*tempPWM);

if( newfall )
  {
    noInterrupts();
    diff -= raising;

//    /////////////////////////////
//    // DEBUG
//    Serial.print( raising, HEX );
//    Serial.print( "\t" );
//    Serial.print( diff );
//    Serial.println( "" );
//    /////////////////////////////
Serial.println(kill);
    
    newfall = 0;
    if( diff < 2500 ) {
      kill = true;
    }
    if(diff > 3400 && diff < 4000) {
      kill = false;
    }
    interrupts();
  }
}


