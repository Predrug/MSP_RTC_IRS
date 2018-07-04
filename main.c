/*
 * @file main.c
 * @author Predrag Mitrovic 2016/0678
 * @date 24 may 2018
 * @brief Low power real time clock and calendar
 *
 * @details This is a system that realizes the clock and real-time calendar on msp430 with a couple of additional functionalities.
 * Time and date are displayed on 7 segment LED display which turns on when button is pressed and gradually extinguishes after 5 seconds since the last button press.
 * Firstly, hours and minutes are displayed, again pressing the button shows the date and pressing it again shows the year.
 * Time and date in RTC time can be changed using the other 3 buttons.
 * 10 seconds after the last button press system goes in sleep mode where all timers interrupts are disabled.
 * The entire program runs in LP3 mode except all interrupts that are processed in operational mode.
 */

    /* Standard include*/
    #include <msp430.h>

    /* Variable with the information indicating whether the time, date or year is displayed*/
    volatile unsigned int ucDISPLAY=0;

    /* Variable with the information indicating whether 1st, 2nd ,3rd or 4th 7 segment led is selected*/
    volatile unsigned int uc7SEG=1;

    /* indicator for sleep mode*/
    volatile unsigned int SLEEPmode=1;

    /* Variable with the information of how many half of a secounds passed since last button press*/
    volatile unsigned int SECpassed = 0;

    /* Constants for initial RTC timer values*/
    /* 50 seconds*/
    const unsigned int  SECOUNDS_init = 0b01010000;
    /* 30 minutes*/
    const unsigned int  MINUTES_init = 0b00110000;
    /* 9 hours*/
    const unsigned int  HOURS_init = 0b00001001;
    /* 15 days*/
    const unsigned int  DAY_init = 0b010101;
    /* 05 month*/
    const unsigned int  MONTH_init = 0b00000101;
    /* 20 year higher digits*/
    const unsigned int  YEAR_H_init = 0b00100000;
    /* 18 year lower digits*/
    const unsigned int  YEAR_L_init = 0b00011000;


    /**
    * Timer A counts to 64 which means interrupt is generated with a frequency of TA=512Hz
    * This frequency is used for 7 segment multiplexing
    */
    const unsigned int ACLKdeviderA = 64;

    /**
    * Timer B counts to 256 which means interrupt is generated with a frequency of TB=2Hz
    * This interrupt is used for counting seconds passed since last button press
    */
    const unsigned int ACLKdeviderB = 256;

    /* Constants used for 7 segment brightness duty ratio*/
    const unsigned int PWMFactor1 = 56;
    const unsigned int PWMFactor2 = 50;
    const unsigned int PWMFactor3 = 44;
    const unsigned int PWMFactor4 = 38;
    const unsigned int PWMFactor5 = 32;
    const unsigned int PWMFactor6 = 26;
    const unsigned int PWMFactor7 = 20;
    const unsigned int PWMFactor8 = 14;
    const unsigned int PWMFactor9 = 8;
    const unsigned int PWMFactor10 = 4;



 /**
 * @brief Main program consisting of initialization and empty infinite for loop
 *
 *  Peripheral that are initialized and used: four 7 segment LED , four buttons
 *  RTC ,TimerA and TimerB initialized
 */

    int main(void)
    {
    /* Stop watchdog timer*/
	WDTCTL = WDTPW | WDTHOLD;
	
	/* Configuring I/O ports*/
	/* initialize buttons*/
	P2DIR &= ~(BIT7|BIT6|BIT5|BIT4);                            /**< P2.4, P2.5, P2.6, P2.7 as in*/
	P2IES &= ~(BIT7|BIT5|BIT4);                                 /**< falling edge*/
	P2IFG &= ~(BIT7|BIT5|BIT4);                                 /**< clear flags*/
	P2IE |= BIT7;                                               /**< enable interrupt only for P2.7*/

	/*initialize 7-seg display*/
	P10DIR |= BIT6 | BIT7;
	P11DIR |= BIT0 | BIT1;
	P6DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;

	/*Configuring RTC_A*/
	RTCCTL1 &= ~BIT6;                                           /**< RTC in operational*/
	RTCCTL1 |= BIT5;                                            /**< calendar mode selected*/
	RTCCTL1 |= BIT7;                                            /**< BCD code for calendar selected*/


	/*configuring timerA*/
	TA0CTL |= TASSEL__ACLK;                                     /**< ACLK as timer source*/
	TA0CTL |= MC_1;                                             /**< Timer A started*/
	TA0CTL &= ~TAIE;                                            /**< TimerA0 interrupt disable*/
	TA0CCR0 = ACLKdeviderA;
	TA0CCR1 = PWMFactor1;
	TA0CCTL0 &= ~BIT4;                                          /**< disable interrupt for display multiplexing*/
	TA0CCTL1 &= ~BIT8;                                          /**< compare mode selected*/
	TA0CCTL1 &= ~BIT4;                                          /**< disable interrupt for 7 segment brightness pulse width modulation*/


	/*configuring timerB*/
	 TB0CTL|=TBSSEL__ACLK;                                      /**< ACLK as timer source*/
	 TB0CTL |= MC_1;                                            /**< Timer B stopped*/
	 TB0CTL &= ~TBIE;                                           /**< TimerB0 interrupt disable*/
	 TA0CCTL0 &= ~BIT4;                                         /**< disable interrupt for seconds counting*/
	 TB0CCR0 = ACLKdeviderB;                                    /**< makes timer B interrupt ever 0.5 seconds*/
	 TB0EX0 |= TBIDEX_7 ;                                       /**< TimerB clock source is divided by 8*/
	 TB0CTL |= ID_3;                                            /**< divided by 8 once again  = 512HZ timer tick*/

	 /*setting initial RTC values*/
	 RTCSEC = SECOUNDS_init;
	 RTCMIN = MINUTES_init;
	 RTCHOUR = HOURS_init;
	 RTCDAY = DAY_init;
	 RTCMON = MONTH_init;
	 RTCYEARH = YEAR_H_init;
	 RTCYEARL = YEAR_L_init;

	 /*Enter LPM3*/
	 _BIS_SR(LPM3_bits + GIE);

	 /* GIE*/
 	 //__enable_interrupt();

	 /* Empty infinite for loop*/
	 for( ;; );
    }

 /**
 * @brief Interrupt routine for all buttons that have some kind of functionalities
 *
 * in sleep mode only interrupt for button 4 is enabled. After pressing it display turns on and other button interrupts become enabled as well.
 * Pressing button 4 multiple times it changes display in circular way.
 * When display is turned on, by holding button 3 time and date can be changed by pressing button 1 incrementing higher digits and
 * button 2 for incrementing lower digits.
 * Logic for working with BCD numbers and RTC counter time and calendar registers added.
 */
    void __attribute__ ((interrupt(PORT2_VECTOR))) P2ISR (void)
    {
    /* wait for 300 cycles - debounce */
    __delay_cycles(300);

    /* check if P2.4 or P2.5 or P2.7  flag is set- debouncing purpose */
    if ((P2IFG & (BIT4 | BIT5 | BIT7)) != 0)
    {
    /* check if P2.4 is still pressed*/
    if ((0 == (P2IN & BIT4))|(0 == (P2IN & BIT5))|(0 == (P2IN & BIT7)))
    {
    /* reset secounds passed every time button is pressed*/
    SECpassed = 0;

    switch (SLEEPmode)
        {
        /* getting out of sleep mode*/
        case 1:
            ucDISPLAY = 0;
            /* enable interupt for display multyplexing*/
            TA0CCTL0 |= BIT4;

            /* enable interupt for secounds counting*/
            TB0CCTL0 |= BIT4;

            /* clear flag Timer A*/
            TA0CCTL0 &= ~BIT0;

            /* clear flag Timer B*/
            TB0CCTL0 &= ~BIT0;

            SLEEPmode = 0;
            TA0CTL |= TACLR;                                    /**< Reset timer A when getting out of sleep mode*/
            P2IE |= BIT5|BIT4;                                  /**< enable interrupt for other buttons when getting out of sleep mode*/
            P2IFG &= ~(BIT7|BIT4|BIT5);                         /**< clear flags*/
            break;
        case 0:
            switch(P2IV)
                {
            /* Logic for button 4*/
                case 16:
                    /*clear flag*/
                    P2IFG &= ~BIT7;

                    /*increment display count*/
                    ucDISPLAY++;

                    if (ucDISPLAY == 3)
                        {
                        ucDISPLAY = 0;
                        }
                    break;
             /* Logic for button 2*/
                case 12:
                    /*clear flag*/
                    P2IFG &= ~BIT5;

                    /*increment lower digits in circular way*/
                    switch (ucDISPLAY)
                        {
                        case 0:
                            RTCMIN++;
                            if (RTCMIN == 97 )
                                {
                                RTCMIN = 0;
                                }
                            else if ((RTCMIN == 10)|(RTCMIN == 26)|(RTCMIN == 42)|(RTCMIN == 58)|(RTCMIN == 74)|(RTCMIN == 90))
                                {
                                RTCMIN= RTCMIN+6;
                                }
                            break;
                        case 1:
                            RTCMON++;
                            if (RTCMON == 19 )
                                {
                                RTCMON = 0;
                                }
                            else if (RTCMON == 10)
                                {
                                RTCMON= RTCMON+6;
                                }
                            break;
                        case 2:
                            RTCYEARL++;
                            if (RTCYEARL == 154 )
                                {
                                RTCYEARL = 0;
                                }
                            else if ((RTCYEARL == 10)|(RTCYEARL == 26)|(RTCYEARL == 42)|(RTCYEARL == 58)|(RTCYEARL == 74))
                                {
                                RTCYEARL=RTCYEARL+6;
                                }
                            else if ((RTCYEARL == 90)|(RTCYEARL == 106)|(RTCYEARL == 122)|(RTCYEARL == 138))
                                {
                                RTCYEARL= RTCYEARL+6;
                                }
                            break;
                        default:
                            break;
                        }
                    break;
                /* Logic for button 1*/
                case 10:
                    /*clear flag for button 1*/
                    P2IFG &= ~BIT4;

                    /*increment higher digits in circular way*/
                    switch (ucDISPLAY)
                        {
                        case 0:
                        RTCHOUR++;
                        if (RTCHOUR == 37 )
                            {
                            RTCHOUR = 0;
                            }
                        else if ((RTCHOUR == 10)|(RTCHOUR == 26))
                            {
                            RTCHOUR= RTCHOUR+6;
                            }
                        break;
                        case 1:
                        RTCDAY++;
                        if (RTCDAY == 50 )
                            {
                            RTCDAY = 0;
                            }
                        else if ((RTCDAY == 10)|(RTCDAY == 26)|(RTCDAY == 42))
                            {
                            RTCDAY= RTCDAY+6;
                            }
                        break;
                        case 2:
                        RTCYEARH++;
                        if (RTCYEARH == 42)
                            {
                            RTCYEARH=32;
                            }
                         break;
                         default:
                         break;
                         }
                    break;
                default:
                    break;
                }
            break;

        default:
            break;
        }
    /*disable interupt for pwm*/
    TA0CCTL1 &= ~BIT4;

    /* reset secounds passed every time any button is pressed*/
    SECpassed = 0;

    /*reset timerB every time any button is pressed*/
    TB0CTL |= TBCLR;
            }
            else
            {
            /*clear flags*/
            P2IFG &= ~(BIT7|BIT4|BIT5);
            }
        }
    }


 /**
 * @brief Timer B interrupt routine
 *
 * Counts seconds passed since last button press:
 * If its more then 5, PWM brightness adjustment with PWM is enabled and set on proper value.
 * After 10 seconds since last button press system goes into sleep mode where all interrupts except button 4 is disabled.
 */
    void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) TB0ISR (void)
    {
    /*clear flag for Timer B*/
    TB0CCTL0 &= ~BIT0;

    /*increment seconds passed every 0.5 secs*/
    SECpassed ++;
    switch (SECpassed)

        {
        case 10:
            /*enable interupt for pwm*/
            TA0CCTL1 |= BIT4;

            TA0CCR1 = PWMFactor1;
            break;

        case 11:
            TA0CCR1 = PWMFactor2;
            break;

        case 12:
            TA0CCR1 = PWMFactor3;
            break;

        case 13:
            TA0CCR1 = PWMFactor4;
            break;

        case 14:
            TA0CCR1 = PWMFactor5;
            break;

        case 15:
            TA0CCR1 = PWMFactor6;
            break;

        case 16:
            TA0CCR1 = PWMFactor7;
            break;

        case 17:
            TA0CCR1 = PWMFactor8;
            break;

        case 18:
            TA0CCR1 = PWMFactor9;
            break;

        case 19:
            TA0CCR1 = PWMFactor10;
            break;
        /* disabling all timers for more efficient low power mode*/
        case 20:
            /*disable interupt for display multyplexing*/
            TA0CCTL0 &= ~BIT4;

            /*disable interupt for secounds counting*/
            TA0CCTL0 &= ~BIT4;

            /*entering sleep mode*/
            SLEEPmode = 1;

            /*disable interrupt for other buttons after entering sleep mode*/
            P2IE &= ~(BIT5|BIT4);

            break;

        default:
            break;
        }
    }
