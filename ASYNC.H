typedef enum {Com1,Com2,Com3,Com4} ComPort;       /*  Asynchronous ports  */
typedef enum {B300,B600,B1200,B2400,B4800,B9600,B19200,B38400,B57600,                       B115200} ComSpeed;                  /* Bit Rates            */
typedef enum {NoParity,EvenParity,OddParity} ComParity; /* parity         */
#define              ComBufSize 4096              /*  4K buffers */
 
#define ComData      0         /*  Communications port data xmit/recv       */
#define ComEnable    1         /*  Communications port interrupt enable     */
#define ComEnableRD  1         /*    data received                          */
#define ComEnableTX  2         /*    transmit register empty                */
#define ComEnableLS  4         /*    line status                            */
#define ComEnableMS  8         /*    modem status                           */
#define ComIdent     2         /*  Communications port interrupt identity   */
#define ComIdentNot  1         /*    interrupt not pending                  */
#define ComIdentMS   0         /*    modem status interrupt                 */
#define ComIdentTX   2         /*    transmit register empty                */
#define ComIdentRD   4         /*    data received                          */
#define ComIdentLS   6         /*    line status interrupt                  */
#define ComLineCtl   3         /*  Communications port line control         */
#define ComLineBreak 64        /*  Communications Line Send Break           */
#define ComLineDLAB  128       /*  Communications Divisor Latch Access Bit  */
#define ComModemCtl  4         /*  Communications port modem control        */
#define ComModemDTR  1         /*    data terminal ready                    */
#define ComModemRTS  2         /*    request to send                        */
#define ComModemOUT1 4         /*    Out 1 signal (enables ring)            */
#define ComModemOUT2 8         /*    Out 2 signal (enables data receive)    */
#define ComLineStat  5         /*  Communications port line status          */
#define ComLineOR    2         /*    overrun                                */
#define ComLinePE    4         /*    parity error                           */
#define ComLineFE    8         /*    framing error                          */
#define ComLineBI    16        /*    break interrupt                        */
#define ComLineTHRE  32        /*    transmit holding register empty        */
#define ComModemStat 6         /*  Communications port modem status         */
#define ComModemCTS  16        /*    clear to send                          */
#define ComModemDSR  32        /*    data set ready                         */
#define ComModemRI   64        /*    phone ring                             */
#define ComModemCD   128       /*    carrier detect                         */
#define false          0
#define true           1
 
/*  This turns the communications interrupts on or off  */
/*  It is called automatically by ComClose */
void         Set_8259(ComPort ComDev,char Sw);
/*  This sets the communications baud rate  */
/*  It is called automatically by ComOpen */
void         SetSpeed(ComPort ComDev, ComSpeed ComRate);
/*  This modifies the line control register for parity & data bits  */
/*  It is called automatically by ComOpen */
void         SetFormat(ComPort ComDev,ComParity ComFormat);
/*  This modifies the modem control register for DTR, RTS, etc.  */
/*  It is called automatically by ComOpen */
void         SetModem(ComPort ComDev,char ModemCtl);
/*  This modifies the communications interrupt enable register  */
/*  It is called automatically by ComOpen & ComClose */
void         SetEnable(ComPort ComDev,char Enable);
/*  This procedure MUST be called before doing any I/O.  */
char         ComOpen(ComPort ComDev,ComSpeed ComRate,ComParity ComFormat);
/*  This procedure shuts-down communications  */
void         ComClose(ComPort ComDev);
/*  This Function returns the modem status  */
char         ComStat(ComPort ComDev);
/*  This procedure tells you if there's any input  */
char         ComInReady(ComPort ComDev);
/*  This procedure reads input from the ring buffer  */
/*  It will wait if no characters are available */
char         ComIn(ComPort ComDev);
/*  This procedure writes output, filling the ring buffer if necessary.  */
void         ComOut(ComPort ComDev,char Data);
