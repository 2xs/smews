#ifndef MCU_H_
#define MCU_H_

/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   Global macros                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/

#define ENABLE_GLOBAL_INT()     sei()
#define DISABLE_GLOBAL_INT()    cli()



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                ATMega128L SPI               ************************
 *******************************************************************************************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
/* SPI--->SPCR : SPI Control Register */
#define SPR0    0   //SPI Clock Rate Select  : fosc/4 :00, fosc/16: SPR0:1 SPR1:0
#define MSTR    4   //Master/Slave Select : This bit selects Master SPI mode when written to one
#define SPE     6   //SPI Enable : This bit must be set to enable any SPI operations.

/* SPI--->SPSR : SPI Status Register */
#define SPIF    7   //SPI Interrupt Flag

//------------------------------------------------------------------------------------------------------

/*******************************************************************************************************
 *******************************************************************************************************
 **************************                ATmega128L I/O PORTS               **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
// Port A 
#define YLED            0  // PA.0 - Output: Yellow LED
#define GLED            1  // PA.1 - Output: Green LED 
#define RLED            2  // PA.2 - Output: Red LED 
#define VREG_EN         5  // PA.5 - Output: VREG_EN to CC2420 
#define RESET_N         6  // PA.6 - Output: RESET_N to CC2420 


// Port B 
#define CSN             0  // PB.0 - Output: SPI Chip Select (CS_N) 
#define SCK             1  // PB.1 - Output: SPI Serial Clock (SCLK) 
#define MOSI            2  // PB.2 - Output: SPI Master out - slave in (MOSI) 
#define MISO            3  // PB.3 - Input:  SPI Master in - slave out (MISO) 
#define FIFO            7  // PB.7 - Input:  FIFO from CC2420

// Port D 
#define SFD             4  // PD.4 - Input:  SFD from CC2420 (on input captur 1)
#define CCA             6  // PD.6 - Input:  CCA from CC2420 (Timer1)
#define FIFOP           7  // PD.7 - Input:  FIFOP from CC2420  (Timer2)


// Enables/disables the SPI interface
#define SPI_DISABLE()           ( PORTB |=  BM(CSN) ) // chip select CSn
#define SPI_ENABLE()            ( PORTB &= ~BM(CSN) ) // chip select CSn (active low) 

//------------------------------------------------------------------------------------------------------

 /******************************************************************************************************
 *******************************************************************************************************
 **************************                 CC2420 PIN ACCESS                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
// Pin status
#define FIFO_IS_1       (!!(PINB & BM(FIFO)))
#define CCA_IS_1        (!!(PIND & BM(CCA)))
#define RESET_IS_1      (!!(PINA & BM(RESET_N)))
#define VREG_IS_1       (!!(PINA & BM(VREG_EN)))
#define FIFOP_IS_1      (!!(PIND & BM(FIFOP)))
#define SFD_IS_1        (!!(PIND & BM(SFD)))

// CC2420 voltage regulator enable pin
#define SET_VREG_ACTIVE()       ( PORTA |=  BM(VREG_EN) ) //Votltage regulator  (active high) 
#define SET_VREG_INACTIVE()     ( PORTA &= ~BM(VREG_EN) ) //Votltage regulator

// The CC2420 reset pin  
#define SET_RESET_ACTIVE()      ( PORTA &= ~BM(RESET_N) ) //reset pin (active low) 
#define SET_RESET_INACTIVE()    ( PORTA |=  BM(RESET_N) ) //reset pin 
//-------------------------------------------------------------------------------------------------------


/*******************************************************************************************************
 *******************************************************************************************************
 **************************               INTERRUPTS                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
/* FIFOP on Timer2 over flow : external clock */
/* Interrupts --->TIFR : Timer/Counter Interrupt Flag Register */
#define TOV2 6		//Bit 6 : TOV2: Timer/Counter2 Overflow Flag
#define ICF1 5		//Bit 5 : ICF1: Timer/Counter1, Input Capture Flag

/* Interrupts --->TIMSK: Timer/Counter Interrupt Mask Register */
#define TICIE1  5	//Timer/Counter1, Input Capture Interrupt Enable	
#define TOIE2  6 	//Timer/Counter2 Overflow Interrupt Enable

/* Interrupts --->TCCR2: Timer/Counter Control Register */
#define WGM20 6		//Waveform Generation Mode : to set Timer/Counter Mode of Operation
#define WGM21 3		//Waveform Generation Mode : to set Timer/Counter Mode of Operation	
#define CS20 0		//Clock Select 0 : to set External clock source on T2 pin
#define CS21 1		//Clock Select 1 : to set External clock source on T2 pin
#define CS22 2		//Clock Select 2 : to set External clock source on T2 pin

/* Interrupts --->TCCR1B: Timer/Counter1 Control Register B */
#define ICES1 6		//Input Capture Edge Select Timer1

//Normal mode  on Timer2 : TOP 0xFF, immediate update, TOV2 MAX
//Need to initialize TCNT2 to 0xFF : it will generate an overflows interrupt
//TCNT2 should be initialized again to 0xFF in the ISR
#define FIFOP_INT_INIT()        do {    TCNT2=0xFF; \
                                        TCCR2 &=  ~(BM(WGM20)|BM(WGM21)); \
                                        TCCR2 |= (BM(CS22) | BM(CS21)| BM(CS20)); \
                                        CLEAR_FIFOP_INT(); \
					} while (0) 

#define ENABLE_FIFOP_INT()		do {TIMSK |= BM(TOIE2); } while(0)

#define DISABLE_FIFOP_INT()     do {TIMSK &= ~BM(TOIE2); } while (0) 

#define CLEAR_FIFOP_INT()       do {TIFR  &= ~BM(TOV2); } while(0)


// SFD interrupt on timer 1 capture pin
#define ENABLE_SFD_CAPTURE_INT()    do { TIMSK |=  BM(TICIE1); } while (0)
#define DISABLE_SFD_CAPTURE_INT()   do { TIMSK &= ~BM(TICIE1); } while (0)
#define CLEAR_SFD_CAPTURE_INT()     do { TIFR = BM(ICF1); } while (0)


//-------------------------------------------------------------------------------------------------------
 
 /******************************************************************************************************
 *******************************************************************************************************
 **************************                        LED                        **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
// LED
#define RLED_EN()               PORTA &= ~(1<<RLED)
#define RLED_DISABLE()          PORTA |= (1<<RLED) 
#define RLED_TOGGLE()           PORTA ^= (1<<RLED) 
#define GLED_EN()               PORTA &= ~(1<<GLED)
#define GLED_DISABLE()          PORTA |= (1<<GLED)
#define GLED_TOGGLE()           PORTA ^= (1<<GLED)
#define YLED_EN()               PORTA &= ~(1<<YLED)
#define YLED_DISABLE()          PORTA |= (1<<YLED)
#define YLED_TOGGLE()           PORTA ^= (1<<YLED)  

//------------------------------------------------------------------------------------------------------


 /******************************************************************************************************
 **************************                     Definitions                      **************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
void PORT_Init(void);
void SPI_Init(void);
unsigned char spi(unsigned char data);
void Sleep(void);
//------------------------------------------------------------------------------------------------------



#endif



