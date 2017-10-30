#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>

#include <config.h>
/*****************************************
*setLed()
* INPUT:  None
* EFFECT: None
* RETURN: None
* LAYOUT:
*    LED,      PWR  WPS   WIFI   WAN   LAN1   LAN2   LAN3   LAN4    
*    GPIO,     18    3     8    14/15   4       5      6      7   
******************************************/


#define GPIO_OE         0x18040000
#define GPIO_OUT        0x18040008
#define GPIO_FUNC0      0x1804002c
#define GPIO_FUNC1      0x18040030
#define GPIO_FUNC2      0x18040034
#define GPIO_FUNC3      0x18040038
#define GPIO_FUNC4      0x1804003c
#define GPIO_FUNC5      0x18040040
#define GPIO_FUNC       0x1804006c

    .globl setLed
    .ent setLed
setLed:
/* Currently WR1043NDV4 use ap152. But the following gpio set if for Products with CONFIG_AP151 
 * setted. The gpio init code has been moved to "void ath_gpio_config(void)" in board956x.c */
#ifndef CONFIG_AP152  
#if (CONFIG_PRODUCT_ID == 0x09411001)
    /* set non-JTag */
    li t0, GPIO_FUNC
    lw t1, 0(t0)
    li t2,  (1<<1) /* we use GPIO14/GPIO15, so disable JTAG*/
    or t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO18, GPIO3, GPIO8, GPIO14/GPIO15, GPIO4, GPIO5, GPIO6, GPIO7 as output*/
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ~((1<<18) | (1<<3) | (1<<8) | (1<<14) | (1<<15) | (1<<6) | (1<<7)) 
    /* li t2, 0xfffb3e07 */
    and t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO1(reset & WPS), GPIO2(WIFI) as input */
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ((1<<1) | (1<<2) | (1<<4) | (1<<5)) 
    or t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO3's signal source */
    li t0, GPIO_FUNC0
    lw t1, 0(t0)
    li t2, 0x00ffffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO6/7's signal source */
    li t0, GPIO_FUNC1
    lw t1, 0(t0)
    li t2, 0x0000ffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO14/GPIO15's signal source */
    li t0, GPIO_FUNC3
    lw t1, 0(t0)
    li t2, 0x0000ffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO8's signal source */
    li t0, GPIO_FUNC2
    lw t1, 0(t0)
    li t2, 0xffffff00
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO18's signal source */
    li t0, GPIO_FUNC4
    lw t1, 0(t0)
    li t2, 0xff00ffff
    and t1, t1,  t2
    sw t1, 0(t0)


    /* turn on all wan blue*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ~((1<<18) | (1<<3) | (1 << 8) | (1<<14) |  (1<<6) | (1<<7))
    /*li t2, 0xfffb3e07*/
    and t1, t1, t2
    li t3, (1<<15)
    or t1, t1, t3
    sw t1, 0(t0)

    /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
1:
    addi t0, t0, 1
    bne t0, t1, 1b
    nop

    /* turn off all except pwr led*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ((1<<3) | (1 << 8) | (1<<14) | (1 << 15) | (1 << 18) | (1<<6) | (1<<7))
    or t1, t1, t2
    sw t1, 0(t0)
        /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
2:
    addi t0, t0, 1
    bne t0, t1, 2b
    nop
#else
#if (CONFIG_PRODUCT_ID == 0x09400004)
    /* set non-JTag */
    li t0, GPIO_FUNC
    lw t1, 0(t0)
    li t2,  (1<<1) /* we use GPIO14/GPIO15, so disable JTAG*/
    or t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO18, GPIO3, GPIO8, GPIO14/GPIO15, GPIO4, GPIO5, GPIO6, GPIO7 as output*/
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ~((1<<18) | (1<<3) | (1<<8) | (1<<14) | (1<<15) | (1<<4) | (1<<5) | (1<<6) | (1<<7)) 
    /* li t2, 0xfffb3e07 */
    and t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO1(reset & WPS), GPIO2(WIFI) as input */
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ((1<<1) | (1<<2))
    or t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO3's signal source */
    li t0, GPIO_FUNC0
    lw t1, 0(t0)
    li t2, 0x00ffffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO4/5/6/7's signal source */
    li t0, GPIO_FUNC1
    lw t1, 0(t0)
    li t2, 0x00000000
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO14/GPIO15's signal source */
    li t0, GPIO_FUNC3
    lw t1, 0(t0)
    li t2, 0x0000ffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO8's signal source */
    li t0, GPIO_FUNC2
    lw t1, 0(t0)
    li t2, 0xffffff00
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO18's signal source */
    li t0, GPIO_FUNC4
    lw t1, 0(t0)
    li t2, 0xff00ffff
    and t1, t1,  t2
    sw t1, 0(t0)


    /* turn on all wan blue*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ~((1<<18) | (1<<3) | (1<<8) | (1<<14) | (1<<15) | (1<<4) | (1<<5) | (1<<6) | (1<<7))
    /*li t2, 0xfffb3e07*/
    and t1, t1, t2
    sw t1, 0(t0)

    /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
1:
    addi t0, t0, 1
    bne t0, t1, 1b
    nop

    /* turn on wan orange*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ((1<<14)|(1<<15))
    or t1, t1, t2
    sw t1, 0(t0)

    /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
2:
    addi t0, t0, 1
    bne t0, t1, 2b
    nop

    /* turn off all except pwr led*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ((1<<3) | (1<<8) | (1<<14) | (1<<4) | (1<<18) | (1<<6) | (1<<7))
    or t1, t1, t2
	li t3, ~(1<<15)
    and t1, t1, t3
    sw t1, 0(t0)
#else
    /* set non-JTag */
    li t0, GPIO_FUNC
    lw t1, 0(t0)
    li t2,  (1<<1) /* we use GPIO14/GPIO15, so disable JTAG*/
    or t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO18, GPIO3, GPIO8, GPIO14/GPIO15, GPIO4, GPIO5, GPIO6, GPIO7 as output*/
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ~((1<<18) | (1<<3) | (1<<8) | (1<<14) | (1<<15) | (1<<4) | (1<<5) | (1<<6) | (1<<7)) 
    /* li t2, 0xfffb3e07 */
    and t1, t1, t2
    sw t1, 0(t0)

    /* GPIO init set GPIO1(reset & WPS), GPIO2(WIFI) as input */
    li t0, GPIO_OE
    lw t1, 0(t0)
    li t2,  ((1<<1) | (1<<2))
    or t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO3's signal source */
    li t0, GPIO_FUNC0
    lw t1, 0(t0)
    li t2, 0x00ffffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO4/5/6/7's signal source */
    li t0, GPIO_FUNC1
    lw t1, 0(t0)
    li t2, 0x00000000
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO14/GPIO15's signal source */
    li t0, GPIO_FUNC3
    lw t1, 0(t0)
    li t2, 0x0000ffff
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO8's signal source */
    li t0, GPIO_FUNC2
    lw t1, 0(t0)
    li t2, 0xffffff00
    and t1, t1, t2
    sw t1, 0(t0)

    /* select GPIO18's signal source */
    li t0, GPIO_FUNC4
    lw t1, 0(t0)
    li t2, 0xff00ffff
    and t1, t1,  t2
    sw t1, 0(t0)


    /* turn on all wan blue*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ~((1<<18) | (1<<3) | (1<<8) | (1<<14) | (1<<15) | (1<<4) | (1<<5) | (1<<6) | (1<<7))
    /*li t2, 0xfffb3e07*/
    and t1, t1, t2
    sw t1, 0(t0)

    /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
1:
    addi t0, t0, 1
    bne t0, t1, 1b
    nop

    /* turn on wan orange*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ((1<<14)|(1<<15))
    or t1, t1, t2
    sw t1, 0(t0)

    /* wait for a while */
    li t0, 0x0
    li t1, 0x9000000
2:
    addi t0, t0, 1
    bne t0, t1, 2b
    nop

    /* turn off all except pwr led*/
    li t0, GPIO_OUT
    lw t1, 0(t0)
    li t2, ((1<<3) | (1<<8) | (1<<14) | (1<<4) | (1<<5) | (1<<6) | (1<<7))
    or t1, t1, t2
    li t3, ~(1<<15)
    and t1, t1, t3
    sw t1, 0(t0)
#endif
#endif
#endif
    jr ra
    nop

.end setLed