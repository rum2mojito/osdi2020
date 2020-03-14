#include "uart.h"

void
main()
{
	uart_init();
	uart_puts("Hello");

	while(1) {
		uart_send(uart_getc());
	}
}
