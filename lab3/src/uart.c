#include "gpio.h"
#include "uart.h"
#include "sprintf.h"
#include "registers.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

// get address from linker
extern volatile unsigned char _end;


//implement first in first out buffer with a read index and a write index
char uart_tx_buffer[MAX_BUF_SIZE]={}; 
unsigned int uart_tx_buffer_widx = 0;  //write index
unsigned int uart_tx_buffer_ridx = 0;  //read index
char uart_rx_buffer[MAX_BUF_SIZE]={};
unsigned int uart_rx_buffer_widx = 0;
unsigned int uart_rx_buffer_ridx = 0;

/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |=1;       // Set AUXENB register to enable mini UART. Then mini UART register can be accessed.
    *AUX_MU_CNTL = 0;      // Set AUX_MU_CNTL_REG to 0. Disable transmitter and receiver during configuration.
    *AUX_MU_IER = 0;       // Set AUX_MU_IER_REG to 0. Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_LCR = 3;       // Set AUX_MU_LCR_REG to 3. Set the data size to 8 bit.
    *AUX_MU_MCR = 0;       // Set AUX_MU_MCR_REG to 0. Don’t need auto flow control.
    *AUX_MU_IIR = 0x6;     // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15 (disable pull up/down)
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx

    while((*AUX_MU_LSR&0x01))*AUX_MU_IO; //clean rx data
}

// maybe don't do so many step
void  disable_uart()
{
    register unsigned int r;
    *AUX_ENABLE &= ~(unsigned int)1;
    *AUX_MU_CNTL = 0;
    r=*GPFSEL1;
    r|=((7<<12)|(7<<15)); // gpio14, gpio15
    r&=~(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 2;            // enable pins 14 and 15 (pull down)
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
}

/**
 * Send a character  (only replace uart_put with uart_async_putc when non-echo output)
 */
void uart_putc(char c) {
    unsigned int intc = c;
    /* wait until we can send */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20));
    /* write the character to the buffer */
    *AUX_MU_IO=intc;
}

/**
 * Receive a character
 */
char uart_getc() {
    char r;
    /* wait until something is in the buffer */
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));
    /* read it and return */
    r=(char)(*AUX_MU_IO);

    /*
        echo back
    */
    if(r == '\r')
    {
        uart_printf("\r\r\n");
        do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x40));  //wait for output success Transmitter idle
    }else if(r == '\x7f') // backspace -> get del
    {
        uart_putc('\b');
        uart_putc(' ');
        uart_putc('\b');
    }else
    {
        uart_putc(r);
    }
    /* convert carrige return to newline */
    return r=='\r'?'\n':r;
}

/**
 * Display a string with newline
 */
int uart_puts(char *s) {
    int i=0;

    while(*s) {
        uart_putc(*s++);
        i++;
    }
    uart_putc('\r');
    uart_putc('\n');

    return i+2;
}

/**
 * Display a string with newline
 */
int uart_async_puts(char *s) {
    int i=0;

    while(*s) {
        uart_async_putc(*s++);
        i++;
    }
    uart_async_putc('\r');
    uart_async_putc('\n');

    return i+2;
}

/**
 * get a string (use async getc)
 */
char* uart_async_gets(char *buf)
{
    int count;
	char c;
	char *s;
	for (s = buf,count = 0; (c = uart_async_getc()) != '\n' && count!=MAX_BUF_SIZE-1 ;count++)
    {
        *s = c;
        if(*s=='\x7f') //delete -> backspace
        {
            count--;
            if(count==-1)
            {
                uart_putc(' '); // prevent back over command line #
                continue;
            }
            s--;
            count--;
            continue;
        }
        s++;
    }
	*s = '\0';
	return buf;
}

/**
 * get a string (use async getc)
 */
char* uart_gets(char *buf)
{
    int count;
	char c;
	char *s;
	for (s = buf,count = 0; (c = uart_getc()) != '\n' && count!=MAX_BUF_SIZE-1 ;count++)
    {
        *s = c;
        if(*s=='\x7f')
        {
            count--;
            if(count==-1)
            {
                uart_putc(' '); // prevent back over command line #
                continue;
            }
            s--;
            count--;
            continue;
        }
        s++;
    }
	*s = '\0';
	return buf;
}

/**
 * printf (from https://github.com/bztsrc/raspi3-tutorial/tree/master/12_printf)
   initial printf from github dont use any va_end, and it is also can run. (in assembly there is nothing compiled from __builtin_va_end)
 */
int uart_printf(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    // we don't have memory allocation yet, so we
    // simply place our string after our code
    char *s = (char*)buf;
    // use sprintf to format our string
    int count = vsprintf(s,fmt,args);
    // print out as usual
    while(*s) {
        uart_putc(*s++);
    }
    __builtin_va_end(args); 
    return count;
}

int uart_async_printf(char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    char buf[MAX_BUF_SIZE];
    // we don't have memory allocation yet, so we
    // simply place our string after our code
    char *s = (char*)buf;
    // use sprintf to format our string
    int count = vsprintf(s,fmt,args);
    // print out as usual
    while(*s) {
        uart_async_putc(*s++);
    }
    __builtin_va_end(args); 
    return count;
}

//https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf p13
/*
 AUX_MU_IIR
 on read bits[2:1] :
 00 : No interrupts
 01 : Transmit holding register empty
 10 : Receiver holds valid byte
 11: <Not possible> 
*/

// buffer read, write
void uart_interrupt_handler(){

    if(*AUX_MU_IIR & (0b01<<1)) //can write
    {
        // buffer empty
        if(uart_tx_buffer_ridx == uart_tx_buffer_widx)
        {
            disable_mini_uart_w_interrupt();  // disable w_interrupt to prevent interruption without any async output
            return;  
        }
        uart_putc(uart_tx_buffer[uart_tx_buffer_ridx++]);  
        if(uart_tx_buffer_ridx>=MAX_BUF_SIZE)uart_tx_buffer_ridx=0; // cycle pointer
    }
    else if(*AUX_MU_IIR & (0b10<<1)) //can read
    {
         //read buffer full
        if((uart_rx_buffer_widx + 1) % MAX_BUF_SIZE == uart_rx_buffer_ridx)
        {   
            disable_mini_uart_r_interrupt();    //disable read interrupt when read buffer full
            return;
        }
        uart_rx_buffer[uart_rx_buffer_widx++] = uart_getc();
        if(uart_rx_buffer_widx>=MAX_BUF_SIZE)uart_rx_buffer_widx=0;
    }else
    {
        uart_printf("uart_interrupt_handler error!!\n");
    }

}

void uart_async_putc(char c){

    // full buffer wait
    while( (uart_tx_buffer_widx + 1) % MAX_BUF_SIZE == uart_tx_buffer_ridx )
    {
        // start asynchronous transfer 
        enable_mini_uart_w_interrupt();
    }
    uart_tx_buffer[uart_tx_buffer_widx++] = c;
    if(uart_tx_buffer_widx>=MAX_BUF_SIZE)uart_tx_buffer_widx=0;  // cycle pointer

    // start asynchronous transfer 
    enable_mini_uart_w_interrupt();
}

char uart_async_getc(){

    // while buffer empty
    // enable read interrupt to get some input into buffer
    while(uart_rx_buffer_ridx == uart_rx_buffer_widx)enable_mini_uart_r_interrupt();

    char r = uart_rx_buffer[uart_rx_buffer_ridx++];

    return r;
}

void enable_mini_uart_interrupt(){
    enable_mini_uart_r_interrupt();
    enable_mini_uart_w_interrupt();
    *IRQS1 |= 1 << 29;
}

void disable_mini_uart_interrupt(){
    disable_mini_uart_r_interrupt();
    disable_mini_uart_w_interrupt();
}

void enable_mini_uart_r_interrupt(){
    *AUX_MU_IER |=1;  // read interrupt
}

void enable_mini_uart_w_interrupt(){
    *AUX_MU_IER |=2;  // write interrupt
}

void disable_mini_uart_r_interrupt(){
    *AUX_MU_IER &= ~(1);
}

void disable_mini_uart_w_interrupt(){
    *AUX_MU_IER &= ~(2);
}