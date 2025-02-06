#include "rpi.h"
#include "common.h"
#include "util.h"
#include <stdarg.h>
// #include <stdint.h>

/*********** GPIO CONFIGURATION ********************************/

#define GPFSEL_REG(reg) (*(uint32_t*)(GPIO_BASE + GPFSEL_OFFSETS[reg]))
#define GPIO_PUP_PDN_CNTRL_REG(reg) (*(uint32_t*)(GPIO_BASE + GPIO_PUP_PDN_CNTRL_OFFSETS[reg]))

static void setup_gpio(uint32_t pin, uint32_t setting, uint32_t resistor)
{
    uint32_t reg = pin / 10;
    uint32_t shift = (pin % 10) * 3;
    uint32_t status = GPFSEL_REG(reg); // read status
    status &= ~(7u << shift); // clear bits
    status |= (setting << shift); // set bits
    GPFSEL_REG(reg) = status;

    reg = pin / 16;
    shift = (pin % 16) * 2;
    status = GPIO_PUP_PDN_CNTRL_REG(reg); // read status
    status &= ~(3u << shift); // clear bits
    status |= (resistor << shift); // set bits
    GPIO_PUP_PDN_CNTRL_REG(reg) = status; // write back
}

// GPIO initialization, to be called before UART functions.
// For UART3 (line 2 on the RPi hat), we need to configure the GPIO to route
// the uart control and data signals to the GPIO pins 4-7 expected by the hat.
// GPIO pins 14 & 15 already configured by boot loader, but redo for clarity.
void gpio_init()
{
    setup_gpio(4, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(5, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(6, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(7, GPIO_ALTFN4, GPIO_NONE);
    setup_gpio(14, GPIO_ALTFN0, GPIO_NONE);
    setup_gpio(15, GPIO_ALTFN0, GPIO_NONE);
}

/*********** UART CONTROL ************************ ************/

// Configure the line properties (e.g, parity, baud rate) of a UART and ensure that it is enabled
void uart_config_and_enable(size_t line)
{
    uint32_t baud_ival, baud_fval;
    uint32_t stop2;

    switch (line) {
    // setting baudrate to approx. 115246.09844 (best we can do); 1 stop bit
    case CONSOLE:
        baud_ival = 26;
        baud_fval = 2;
        stop2 = 0;
        break;
    // setting baudrate to 2400; 2 stop bits
    case MARKLIN:
        baud_ival = 1250;
        baud_fval = 0;
        stop2 = UART_LCRH_STP2;
        break;
    default:
        return;
    }

    // line control registers should not be changed while the UART is enabled, so disable it
    uint32_t cr_state = UART_REG(line, UART_CR);
    UART_REG(line, UART_CR) = cr_state & ~UART_CR_UARTEN;

    // set the baud rate
    UART_REG(line, UART_IBRD) = baud_ival;
    UART_REG(line, UART_FBRD) = baud_fval;

    // set the line control registers: 8 bit, no parity, 1 or 2 stop bits, FIFOs enabled
    UART_REG(line, UART_LCRH) = UART_LCRH_WLEN_HIGH | UART_LCRH_WLEN_LOW | UART_LCRH_FEN | stop2;

    // re-enable the UART; enable both transmit and receive regardless of previous state
    UART_REG(line, UART_CR) = cr_state | UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

unsigned char uart_getc(size_t line)
{
    unsigned char ch;
    /* wait for data if necessary */
    while (UART_REG(line, UART_FR) & UART_FR_RXFE)
        ;
    ch = UART_REG(line, UART_DR);
    return ch;
}

void uart_putc(size_t line, char c)
{
    // make sure there is room to write more data
    while (UART_REG(line, UART_FR) & UART_FR_TXFF)
        ;
    UART_REG(line, UART_DR) = c;
}

void uart_putl(size_t line, const char* buf, size_t blen)
{
    for (size_t i = 0; i < blen; i++) {
        uart_putc(line, *(buf + i));
    }
}

void uart_puts(size_t line, const char* buf)
{
    while (*buf) {
        uart_putc(line, *buf);
        buf++;
    }
}

void va_uart_printf(size_t line, const char* fmt, va_list va)
{
    char ch, buf[12];
    int width = 0, pad_zero = 0;

    while ((ch = *(fmt++))) {
        if (ch != '%') {
            uart_putc(line, ch);
        } else {
            // Reset width and padding flag
            width = 0;
            pad_zero = 0;

            // Parse width specifier (e.g., %2u, %03d)
            ch = *(fmt++);
            if (ch == '0') { // Zero-padding detected
                pad_zero = 1;
                ch = *(fmt++);
            }
            while (ch >= '0' && ch <= '9') { // Extract width
                width = width * 10 + (ch - '0');
                ch = *(fmt++);
            }

            switch (ch) {
            case 'u':
                ui2a(va_arg(va, unsigned int), 10, buf);
                break;
            case 'd':
                i2a(va_arg(va, int), buf);
                break;
            case 'x':
                ui2a(va_arg(va, unsigned int), 16, buf);
                break;
            case 's':
                uart_puts(line, va_arg(va, char*));
                continue;
            case '%':
                uart_putc(line, '%');
                continue;
            case '\0':
                return; // End of format string
            default:
                uart_putc(line, ch);
                continue;
            }

            // Get length of formatted number
            int len = strlen(buf);

            // Handle padding (either '0' or ' ')
            while (len < width) {
                uart_putc(line, pad_zero ? '0' : ' ');
                len++;
            }

            // Print formatted number
            uart_puts(line, buf);
        }
    }
}

void uart_printf(size_t line, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    va_uart_printf(line, fmt, va);
    va_end(va);
}

int uart_read_available(size_t line)
{
    return UART_REG(line, UART_FR) & ~UART_FR_RXFE;
}

unsigned char uart_assert_getc(size_t line)
{
    unsigned char ch;
    ASSERT(uart_read_available(line), "char not ready");
    ch = UART_REG(line, UART_DR);
    return ch;
}

void uart_assert_putc(size_t line, char c)
{
    ASSERT(!(UART_REG(line, UART_FR) & UART_FR_TXFF), "data line not ready for char");
    UART_REG(line, UART_DR) = c;
}

void ASSERT(int condition, const char* message)
{
    if (!condition) {
        uart_puts(CONSOLE, "ASSERTION FAILED: ");
        uart_puts(CONSOLE, message);
        uart_puts(CONSOLE, "\r\n");
        for (;;) { }
    }
}

void ASSERTF(int condition, const char* message, ...)
{
    if (!condition) {
        uart_puts(CONSOLE, "ASSERTION FAILED: ");
        va_list va;
        va_start(va, message);
        va_uart_printf(CONSOLE, message, va);
        va_end(va);
        uart_puts(CONSOLE, "\r\n");
        for (;;) { }
    }
}
