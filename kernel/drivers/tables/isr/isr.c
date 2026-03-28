#include "isr.h"
#include "../../../terminal/terminal.h"

void isr_handler(registers_t regs)
{
    print("recieved interrupt: ");
    print_int(regs.int_no);
    print("\n");
    if (regs.int_no == 13)
    {
	    for(;;);

    }

} 
