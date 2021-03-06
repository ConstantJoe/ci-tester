/*
 * Copyright (c) 2015-2021 University of Antwerp, Aloxy NV.
 *
 * This file is part of Sub-IoT.
 * See https://github.com/Sub-IoT/Sub-IoT-Stack for further info.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "em_device.h"
#include "hwuart.h"
#include "hwatomic.h"
#include "hwleds.h"
#include "hwlcd.h"
#include "platform_lcd.h"
#include <stdio.h>
#include <em_usbd.h>
#include "timer.h"
#include "SEGGER_RTT.h"
#include "console.h"

//Overwrite _write so 'printf''s get pushed over the uart
int _write(int fd, char *ptr, int len)
{
#ifdef FRAMEWORK_LOG_OUTPUT_ON_RTT
  SEGGER_RTT_Write(0, ptr, len);
#else
  console_print_bytes((uint8_t*)ptr, len);
#endif

  return len;
}

//Overwrite _exit so we don't get a fault that's impossible to debug
void _exit(int exit)
{
    start_atomic();
	//wait forever while the interrupts are disabled
        while(1)
        {
        	//blink the leds so we know _exit has been called
        	for(uint32_t i = 0; i < 0x1FFFFF; i++){}
			led_toggle(0);
			led_toggle(1);
        }
    end_atomic();
}

//we override __assert_func to flash the leds (so we know something bad has happend)
//and to repeat the error message repeatedly (so we have a chance to attach the device to a serial console before the error message is gone)
void __assert_func( const char *file, int line, const char *func, const char *failedexpr)
{
	start_atomic();
	led_on(0);
	led_on(1);
#ifdef PLATFORM_USE_USB_CDC
	// Dissable all IRQs except the one for USB
	for(uint32_t j=0;j < EMU_IRQn; j++)
		NVIC_DisableIRQ(j);

	NVIC_EnableIRQ( USB_IRQn );

	end_atomic();
#endif

	lcd_clear();
	lcd_write_string("ERROR");
	lcd_write_number(timer_get_counter_value());

	while(1)
	{
    printf("assertion \"%s\" failed: line %d%s%s\n",failedexpr, line, file ? ", file: ", file : "", func ? ", function: " : "", func ? func : "");
    if (CoreDebug->DHCSR & 1)
    {
        __BKPT (0); // break into debugger, when attached
    }

		for(uint32_t j = 0; j < 20; j++)
		{
			//blink at twice the frequency of the _exit call, so we can identify which of the two events has occurred
			for(uint32_t i = 0; i < 0xFFFFF; i++){}
			led_toggle(0);
			led_toggle(1);
		}
	}
	end_atomic();

}
