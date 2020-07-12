// hello-msp430f2011 is code to blink an LED.

#include  "msp430x20x2.h"

#define RED_LED BIT0
#define GREEN_LED BIT6
#define BUTTON BIT3

// Global variables used by main and interrupt service routine(s)
unsigned volatile short tick;

void InitializeClocks(void);
void InitializePorts(void);
void InitializeTimerA(void);
void CheckButton(void);
void ToggleGreenLED(void);

void main(void) {
	WDTCTL = WDTPW + WDTHOLD;	// stop watch dog timer
	InitializeClocks();
	InitializePorts();
	InitializeTimerA();
	__enable_interrupt();

	while(1) {
		CheckButton();
		ToggleGreenLED();
	}
}

void InitializeClocks(void)
{
  BCSCTL1 = CALBC1_1MHZ;	// basic clock system control register 1 = calibration for BC1 for 1 MHz
  DCOCTL = CALDCO_1MHZ;		// digital control oscillator control register = calibration for DCO at 1 MHz
  BCSCTL2 |= DIVS_3;    	// sub-main clock (for peripherals like TimerA ): SMCLK = DCO / 8  
}

void InitializePorts(void) {
	P1OUT = BUTTON;		// these two instructions set pull-up on BUTTON (P1.3)
	P1REN |= BUTTON;
	P1DIR = 0xff & ~BUTTON; // all output on P1 except for BUTTON which is an input
	
	P2OUT = 0;
	P2DIR = 0xff;
}

void InitializeTimerA(void) {
	TACTL |= TASSEL1 // timer A control: clock source = SMCLK (sub-main clock)
		+ ID1 + ID0	// input divider: divide by 8
		+ MC0;		// mode control: count up
	TACCR0 = 16;	// a count of 16 will give a tick period of 1.024 ms, given current clock configuration
	TACCTL0 |= CCIE;	// capture / compare interrupt enable
}

volatile unsigned short checkButtonCtr;
enum buttonStateT {pushed, maybeReleased, released};
void toggleRedLED(void);
void CheckButton(void) {
	static const unsigned short t = 30;
	static enum buttonStateT state = released;
	static unsigned short lda = 0;
	
	if (checkButtonCtr != 0 || lda == tick) {
		return;
	}
	lda = tick;
	checkButtonCtr = t;
	
	switch (state) {
	case released:
		if ((P1IN & BUTTON) == 0) {	// button pushed
			state = pushed;
			toggleRedLED();
		}
		break;
	case pushed:
		if ((P1IN & BUTTON) == 0) {
			state = pushed;
			break;
		}
		state = maybeReleased;
		break;
	case maybeReleased:
		if ((P1IN & BUTTON) == 0) {
			state = pushed;
			break;
		}
		state = released;
		break;
	}
}
void toggleRedLED(void){
	P1OUT ^= RED_LED;
}

unsigned volatile short toggleGreenLEDCtr;
void ToggleGreenLED(void) {
	static unsigned short t = 500;	// toggle LED every n ticks
	static unsigned short lda = 0;	// lda: last done at
	
	if (toggleGreenLEDCtr != 0 || lda == tick){
		return;
	}
	
	toggleGreenLEDCtr = t;	// reschedule the task
	lda = tick;
	
	P1OUT ^= GREEN_LED;
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void) {
	if (toggleGreenLEDCtr > 0) toggleGreenLEDCtr--;
	if (checkButtonCtr > 0) checkButtonCtr--;

	tick++;
	
	//TACCTL0 &= ~CCIFG;	// capture/compare interrupt flag is automatically cleared
}
