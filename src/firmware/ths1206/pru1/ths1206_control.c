/*
 * ths1206_control.c
 *
 * Description:
 *
 * (C) 2016 Visaoni
 * Licensed under the MIT License.
 */

// TODO: ensure timing works out with abstractions
// May need to macro and/or switch to precalculated masks

#include "ths1206_control.h"
#include "pin_control.h"
#include "store_readings.h"


static uint8_t trigger_level = 1;


void TC_init_defaults()
{
   trigger_level = 1;

   TC_write( TC_CSR1_BIT | TC_RESET_BIT );  // Sets reset in CR1
   TC_write( TC_CSR1_BIT );  // Clear reset in CR1
}

void TC_init( uint16_t CSR0, uint16_t CSR1 )
{
   // TODO: In general, should account for multiple channels
   //       Won't use more than one here though
   //       Doesn't follow nice patterns with multiple channels
   //       Up to 4 channels selected with 5 bits
   if(      (CSR1 & TC_TRIG0_BIT) == 0 && (CSR1 & TC_TRIG1_BIT) == 0 )
   {
      trigger_level = 1;
   }
   else if( (CSR1 & TC_TRIG0_BIT) == 1 && (CSR1 & TC_TRIG1_BIT) == 0 )
   {
      trigger_level = 4;
   }
   else if( (CSR1 & TC_TRIG0_BIT) == 0 && (CSR1 & TC_TRIG1_BIT) == 1 )
   {
      trigger_level = 8;
   }
   else if( (CSR1 & TC_TRIG0_BIT) == 1 && (CSR1 & TC_TRIG1_BIT) == 1 )
   {
      trigger_level = 14;
   }

   TC_init_defaults();

   TC_write( CSR0 );
   TC_write( CSR1 );
}

void TC_write( uint16_t value )
{
   set_pin( PA_WR_BIT );   // Needed? shouldn't hurt
   set_pin( PA_RD_BIT );
   clear_pin( PA_WR_BIT );

   write_reg( (uint32_t) value );
   __delay_cycles(2);

   set_pin( PA_WR_BIT );
   __delay_cycles(1);   // Probably extraneous, needs 2 ns
}

// For fake-data branch only
static uint16_t count = 0x4141;

uint16_t TC_read()
{
   uint16_t result;

   set_pin( PA_WR_BIT );
   clear_pin( PA_RD_BIT );
   __delay_cycles(2);   // 10ns delay until data ready

   result = (uint16_t)read_reg();
   set_pin( PA_RD_BIT );
   __delay_cycles(1);   // Probably extraneous, needs 5 ns delay to CS invalid, but CS tied active

   // For fake-data branch only
   result = count++;

   return result;
}

void TC_store_next_n_reads( uint32_t n )
{
   // TODO: Handle n % trigger_level != 0 case better
   uint32_t i;
   for( i = 0; i < n; i += trigger_level )
   {
      // Note: DATA_AV defaults to an active high pulse with width half CONV_CLK input
      // Busy wait here should catch it
      // while( !read_pin( PA_DATA_AV_BIT ) );

      int j;
      for( j = 0; j < trigger_level; j++ )
      {
         SR_store( TC_read() );
      }
      // Shouldn't need a delay here - typical time to DATA_AV inactive is 12ns
         // No range given though
   }
}

int8_t TC_test()
{
   TC_init( TC_CSR0_BIT | TC_TEST0_BIT, TC_CSR1_BIT );
   uint16_t v_max = TC_read();

   TC_init( TC_CSR0_BIT | TC_TEST1_BIT, TC_CSR1_BIT );
   uint16_t v_mid = TC_read();

   TC_init( TC_CSR0_BIT | TC_TEST0_BIT | TC_TEST1_BIT, TC_CSR1_BIT );
   uint16_t v_min = TC_read();

   if( v_min < v_mid && v_mid < v_max )
   {
      return 0;
   }
   else
   {
      // TODO: Add more descriptive errors
      return -1;
   }
}
