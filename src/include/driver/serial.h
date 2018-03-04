#ifndef SERIAL_H
#define SERIAL_H

#include <main.h>

#define SERIAL_BUFFER(port)         (port+0)
#define SERIAL_REG_IER(port)        (port+1)
#define SERIAL_REG_IIR(port)        (port+2)
#define SERIAL_REG_FCR(port)        (port+2)
#define SERIAL_REG_LCR(port)        (port+3)
#define SERIAL_REG_MCR(port)        (port+4)
#define SERIAL_REG_LSR(port)        (port+5)
#define SERIAL_REG_MSR(port)        (port+6)
#define SERIAL_REG_SCRATCH(port)    (port+7)

// Modem Control Register bits.
enum {
    SERIAL_MCR_FORCE_DATA_TERMINAL      = 0x01, // Force Data Terminal Ready.
    SERIAL_MCR_FORCE_REQUEST_TO_SEND    = 0x02, // Force Request to Send.
    SERIAL_MCR_AUX_OUTPUT1              = 0x04, // Aux Output 1.
    SERIAL_MCR_INTERUPT                 = 0x08, // Controls the UART-CPU interrupt process.
    SERIAL_MCR_LOOPBACK_MODE            = 0x10, // Loopback Mode.
    SERIAL_MCR_AUTOFLOW                 = 0x20  // Autoflow Control Enabled (16750 only).
};

// Line Status Register.
enum {
    SERIAL_LSR_DATA_READY               = 0x01, // Data ready.
    SERIAL_LSR_OVERRUN_ERROR            = 0x02, // Overrun error.
    SERIAL_LSR_PARITY_ERROR             = 0x04, // Parity error.
    SERIAL_LSR_FRAMING_ERROR            = 0x08, // Framing error.
    SERIAL_LSR_BREAK_INTERRUPT          = 0x10, // Break interrupt.
    SERIAL_LSR_EMPTY_TRANS_HOLDING      = 0x20, // Empty transmitter holding register.
    SERIAL_LSR_EMPTY_DATA_HOLDING       = 0x40, // Empty data holding registers.
    SERIAL_LSR_ERROR_IN_FIFO            = 0x80  // Error in received FIFO.
};

// Modem Status Register.
enum {
    SERIAL_MSR_DELTA_CLEAR_TO_SEND          = 0x01,
    SERIAL_MSR_DELTA_DATA_SET_READY         = 0x02,
    SERIAL_MSR_TRAILING_EDGE_RING           = 0x04,
    SERIAL_MSR_DELTA_DATA_CARRIER_DETECT    = 0x08,
    SERIAL_MSR_CLEAR_TO_SEND                = 0x10,
    SERIAL_MSR_DATA_SET_READY               = 0x20,
    SERIAL_MSR_RING                         = 0x40,
    SERIAL_MSR_CARRIER_DETECT               = 0x80
};

extern bool serial_present();
extern void serial_init();
extern void serial_write(char a);
extern void serial_writes(const char* data);
extern char serial_read();

#endif