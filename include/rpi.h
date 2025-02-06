#ifndef _rpi_h_
#define _rpi_h_

#include "common.h"
#include <stddef.h>

#define CONSOLE 1
#define MARKLIN 2

static char* const MMIO_BASE = (char*)0xFE000000;

static char* const UART0_BASE = (char*)(MMIO_BASE + 0x201000);
static char* const UART3_BASE = (char*)(MMIO_BASE + 0x201600);

static char* const line_uarts[] = { NULL, UART0_BASE, UART3_BASE };
#define UART_REG(line, offset) (*(volatile uint32_t*)(line_uarts[line] + offset))

static char* const GPIO_BASE = (char*)(MMIO_BASE + 0x200000);
static const uint32_t GPFSEL_OFFSETS[6] = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14 };
static const uint32_t GPIO_PUP_PDN_CNTRL_OFFSETS[4] = { 0xe4, 0xe8, 0xec, 0xf0 };

// function control settings for GPIO pins
static const uint32_t GPIO_INPUT = 0x00;
static const uint32_t GPIO_OUTPUT = 0x01;
static const uint32_t GPIO_ALTFN0 = 0x04;
static const uint32_t GPIO_ALTFN1 = 0x05;
static const uint32_t GPIO_ALTFN2 = 0x06;
static const uint32_t GPIO_ALTFN3 = 0x07;
static const uint32_t GPIO_ALTFN4 = 0x03;
static const uint32_t GPIO_ALTFN5 = 0x02;

// pup/pdn resistor settings for GPIO pins
static const uint32_t GPIO_NONE = 0x00;
static const uint32_t GPIO_PUP = 0x01;
static const uint32_t GPIO_PDP = 0x02;

// UART register offsets
static const uint32_t UART_DR = 0x00;
static const uint32_t UART_FR = 0x18;
static const uint32_t UART_IBRD = 0x24;
static const uint32_t UART_FBRD = 0x28;
static const uint32_t UART_LCRH = 0x2c;
static const uint32_t UART_CR = 0x30;
static const uint32_t UART_IMSC = 0x38;
static const uint32_t UART_MIS = 0x40;
static const uint32_t UART_ICR = 0x44;

// masks for specific fields in the UART registers
static const uint32_t UART_FR_RXFE = 0x10;
static const uint32_t UART_FR_TXFF = 0x20;
static const uint32_t UART_FR_RXFF = 0x40;
static const uint32_t UART_FR_TXFE = 0x80;

static const uint32_t UART_CR_UARTEN = 0x01;
static const uint32_t UART_CR_LBE = 0x80;
static const uint32_t UART_CR_TXE = 0x100;
static const uint32_t UART_CR_RXE = 0x200;
static const uint32_t UART_CR_RTS = 0x800;
static const uint32_t UART_CR_RTSEN = 0x4000;
static const uint32_t UART_CR_CTSEN = 0x8000;

static const uint32_t UART_LCRH_PEN = 0x2;
static const uint32_t UART_LCRH_EPS = 0x4;
static const uint32_t UART_LCRH_STP2 = 0x8;
static const uint32_t UART_LCRH_FEN = 0x10;
static const uint32_t UART_LCRH_WLEN_LOW = 0x20;
static const uint32_t UART_LCRH_WLEN_HIGH = 0x40;

static const uint32_t UART_IMSC_RXIM = 0x10;
static const uint32_t UART_IMSC_TXIM = 0x20;
static const uint32_t UART_IMSC_RTIM = 0x40;

static const uint32_t UART_MIS_RXMIS = 0x10;
static const uint32_t UART_MIS_TXMIS = 0x20;
static const uint32_t UART_MIS_RTIM = 0x40;

static const uint32_t UART_ICR_RXIC = 0x10;
static const uint32_t UART_ICR_TXIC = 0x20;
static const uint32_t UART_ICR_RTIM = 0x40;

void gpio_init();
void uart_config_and_enable(size_t line);
unsigned char uart_getc(size_t line);
void uart_putc(size_t line, char c);
void uart_putl(size_t line, const char* buf, size_t blen);
void uart_puts(size_t line, const char* buf);
void uart_printf(size_t line, const char* fmt, ...);

int uart_read_available(size_t line);
int uart_write_available(size_t line);
unsigned char uart_assert_getc(size_t line);
void uart_assert_putc(size_t line, char c);

void ASSERT(int condition, const char* message);
void ASSERTF(int condition, const char* message, ...);

#endif /* rpi.h */
