/*
  spindle_control.c - spindle control methods
  Part of Grbl
 
  PEN_SERVO update by Bart Dring 8/2017
  Copyright (c) 2012-2017 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud
 
  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#include "grbl.h"
 
/*
Pen Servo: 
 
For a pen bot I want to use the spindle PWM to control a servo.
In stepper.c it will will look at the Z position. Any Z>0 is pen up.
 
The spindle output is using a PWM, but we need to adjust that 
 
We only need a rough value because we are only up and down
 
Use 1024 prescaler to get. ... 16,000,000 Mhz  / 1024 = 15625 Hz
It is an 8 bit timer so 15625 / 256 = 61 Hz. This is pretty close the the 50Hz recommended for servos
Each tick = 0.000064sec 
One end of servo is 0.001 sec (0.001 / 0.000064 = 15.6 ticks)
The other end is 0.002 sec (0.002 / 0.000064 = 31 ticks)
 
 
*/
// #define PEN_SERVO    // ...define here or in cpu_map.h to activate

// these are full travel values. If you want to move less than full travel adjust these values
// If your servo is going the wrong way, swap them.
#define PEN_SERVO_DOWN     31      
#define PEN_SERVO_UP       16        
 
#define SERVO_PWM_DDR	  DDRB
  #define SPINDLE_PWM_PORT  PORTB
  #define SERVO_PWM_BIT	  3    // Uno Digital Pin 11 
  #define SERVO_TCCRA_REGISTER	  TCCR2A
  #define SERVO_TCCRB_REGISTER	  TCCR2B
  #define SERVO_OCR_REGISTER      OCR2A
  #define SERVO_COMB_BIT	        COM2A1
 
void init_servo()
{
	SERVO_PWM_DDR |= (1<<SERVO_PWM_BIT); // Configure as output pin.
	SERVO_TCCRA_REGISTER = (1<<COM2A1) | ((1<<WGM20) | (1<<WGM21));
  SERVO_TCCRB_REGISTER = (1<<CS22) | (1 <<CS21) | (1<<CS20);
	
	set_pen_pos();
	
}	

void pen_up()
{
	SERVO_OCR_REGISTER = PEN_SERVO_UP;
}

void pen_down()
{
	SERVO_OCR_REGISTER = PEN_SERVO_DOWN;
}

void set_pen_pos()
{
	float wpos_z;
	
	wpos_z = system_convert_axis_steps_to_mpos(sys_position, Z_AXIS) - gc_state.coord_system[Z_AXIS];  // get the machine Z in mm	
		
	if (wpos_z >= 0.1) { // within one step   
		pen_up();
	}
	else {
		pen_down();
	}	
}

 
void spindle_init()
{
    // Configure no variable spindle and only enable pin.
    SPINDLE_ENABLE_DDR |= (1<<SPINDLE_ENABLE_BIT); // Configure as output pin.
    spindle_stop();
		#ifdef PEN_SERVO
			init_servo(); // put it here so we don't have to edit other files.
		#endif
	
}
 
 
uint8_t spindle_get_state()
{
    
        
    if (bit_istrue(SPINDLE_ENABLE_PORT,(1<<SPINDLE_ENABLE_BIT))) 
	{        
      return(SPINDLE_STATE_CW);
    }
    
    return(SPINDLE_STATE_DISABLE);
}
 
 
// Disables the spindle and sets PWM output to zero when PWM variable spindle speed is enabled.
// Called by various main program and ISR routines. Keep routine small, fast, and efficient.
// Called by spindle_init(), spindle_set_speed(), spindle_set_state(), and mc_reset().
void spindle_stop()
{
	SPINDLE_ENABLE_PORT &= ~(1<<SPINDLE_ENABLE_BIT); // Set pin to low
}
 
 
 
 
// Immediately sets spindle running state with direction and spindle rpm via PWM, if enabled.
// Called by g-code parser spindle_sync(), parking retract and restore, g-code program end,
// sleep, and spindle stop override.

void _spindle_set_state(uint8_t state)
{
  if (sys.abort) { return; } // Block during abort.
  if (state == SPINDLE_DISABLE) { // Halt or set spindle direction and rpm.
   
    
    spindle_stop();
   
  } else {
   
  SPINDLE_ENABLE_PORT |= (1<<SPINDLE_ENABLE_BIT);
      }
   
  sys.report_ovr_counter = 0; // Set to report change immediately
}

void _spindle_sync(uint8_t state)
  {
    if (sys.state == STATE_CHECK_MODE) { return; }
    protocol_buffer_synchronize(); // Empty planner buffer to ensure spindle is set when programmed.
    _spindle_set_state(state);
  }
 
 
