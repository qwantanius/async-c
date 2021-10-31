  /*
**********************************************************************
*                     ASYNCHRONOUS I/O ROUTINES                      *
*                       Version 1.01, 4/8/85                         *
*                                                                    *
* The following are data definitions and procedures which perform    *
* I/O functions for IBM PC asynchronous communications adapters.     *
* Comments here and there, below, explain what's what.  Enjoy!       *
*                                                                    *
* Written and placed in the public domain by:                        *
*                       Glen F. Marshall                             *
*                       1006 Gwilym Circle                           *
*                       Berwyn, PA 19312                             *
*                                                                    *
* Rewritten in Turbo C by:                                           *
*                       John A. Fagerberg                            *
*                       13216 Johnny Moore Ln.                       *
*                       Clifton, VA 22024                            *
*                                                                    *
**********************************************************************
 */
#include <ados.h>
#include <alloc.h>
#include <conio.h>

typedef enum {Com1,Com2,Com3,Com4} ComPort;       /*  Asynchronous ports  */
typedef enum {B300,B600,B1200,B2400,B4800,B9600,B19200,B38400,B57600,
              B115200} ComSpeed;                  /* Bit Rates            */
typedef enum {NoParity,EvenParity,OddParity} ComParity; /* parity         */
#define              ComBufSize 4096              /*  4K buffers, power 2 */
#define              ComBufMask ComBufSize-1      /*  4K buffers          */
typedef char         ComInBuffer[ComBufSize];     /*  In ring buffer      */
typedef char         ComOutBuffer[ComBufSize];    /*  Out ring buffer     */
typedef struct {
                   ComSpeed     ComCtlSpeed;      /*  current baud rate   */
                   ComParity    ComCtlParity;     /*  current parity      */
                   char         LastComIdent;     /*  interrupt reason    */
                   char         LastComLineStat;  /*  line status         */
                   char         LastComModemStat; /*  modem status        */
                   void far     *SavedVector;     /*  pre-open vector     */
                   int          InputInIx;        /*  ring buffer index   */
                   int          InputOutIx;       /*  ring buffer index   */
                   int          OutputOutIx;      /*  ring buffer index   */
                   int          OutputInIx;       /*  ring buffer index   */
                   ComInBuffer  InputBuffer;      /*  input ring buffer   */
                   ComOutBuffer OutputBuffer;     /*  output ring buffer  */
                }  ComCtlRec;
 
char    ComInt[Com4+1] = {0xC,0xB,0xC,0xB}; /** note Com3 & 4 need vector */
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
char    ComLineInit[OddParity+1] = {0x03,0x1A,0x0A};
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
#define ComDivLo     0         /*  Communications port baud-rate divisor 1  */
#define ComDivHi     1         /*  Communications port baud-rate divisor 2  */
int     ComBaudDiv[B115200+1]={0x180,0xC0,0x60,0x30,0x18,0xC,0x6,0x3,0x2,0x1};
#define Cmd_8259     0x20      /*  8259 interrupt controller command port   */
#define EOI_8259     0x20      /*    "End Of Interrupt" command             */
#define RIL_8259     0x0B      /*    "Report Interrupt Level" command       */
#define Msk_8259     0x21      /*  8259 interrupt controller mask port      */
char    MskVal_8259[Com4+1] = {0xEF,0xF7,0xF7,0xF7};/** Com3 & 4 need masks */
 
int     ComBase[Com4+1] = {0x3F8,0x2F8,0x3E8,0x2E8};/*  Ptr, port addresses */
typedef ComCtlRec    *ComCtlPtr;                    /*  Ptr, port control   */
ComCtlPtr            ComPtr[Com4+1];                /*  for each comm port  */
#define false        0
#define true         1
 

/*  Communications interrupt handler  */

void interrupt AsyncInterrupt()
  {
ComPort       ComDev;
char          id;
ComCtlPtr register ptr;
int           com;

    outportb(Cmd_8259,RIL_8259);   /*  find out which COM port interrupted  */
    if ((inportb(Cmd_8259)&0x10) != 0) ComDev = Com1;
    else ComDev = Com2;            /* Having deduced the port, find out why */
 
    ptr = ComPtr[ComDev];
    com = ComBase[ComDev];
 
    (*ptr).LastComIdent = id = inportb(com+ComIdent)&0x06;
    if (id) {                      /* Interrupt to process */
        switch (id) {
 
         case ComIdentLS: (*ptr).LastComLineStat = inportb(com+ComLineStat);
             break;
 
         case ComIdentMS: (*ptr).LastComModemStat = inportb(com+ComModemStat);
             break;
 
         case ComIdentRD: {
 
              if (((*ptr).InputInIx+1)&ComBufMask != (*ptr).InputOutIx)
                 {          /*store input in buffer */
                 (*ptr).InputBuffer[(*ptr).InputInIx] = inportb(com+ComData);
                 (*ptr).InputInIx = ((*ptr).InputInIx+1)&ComBufMask;
                 }
 
              else          /*  buffer overrun  */
                 (*ptr).LastComLineStat = (*ptr).LastComLineStat|ComLineOR;
             } break;
 
         case ComIdentTX: {
 
              if ((*ptr).OutputOutIx != (*ptr).OutputInIx)
                 {
                 /*  write output character from ring buffer  */
                 outportb(com+ComData,(*ptr).OutputBuffer[(*ptr).OutputOutIx]);
                 (*ptr).OutputOutIx = ((*ptr).OutputOutIx+1)&ComBufMask;
                 }
             } break;
        }
    }
    outportb(Cmd_8259,EOI_8259);

  }  /* AsyncInterrupt */


/*  This modifies the communications interrupt vector  */

void         SetInterrupt(ComPort ComDev,char Sw)
  {
union         REGS regs;
struct        SREGS sregs;
ComCtlPtr     ptr;
void far *AsyncPtr = &AsyncInterrupt;

    disable();   /* stop interrupts while we do this */
    ptr = ComPtr[ComDev];

    switch (Sw) {

      case true:   {
              (*ptr).SavedVector = getvect(ComInt[ComDev]);
              setvect(ComInt[ComDev],AsyncPtr);
              }
          break;

      case false: setvect(ComInt[ComDev],(*ptr).SavedVector);
          break;
    }
    enable();   /* reenable interrupts */

  }   /* SetInterrupt */
 
 
/*  This turns the communications interrupts on or off  */
 
void         Set_8259(ComPort ComDev,char Sw)
  {
 
    disable();   /* stop interrupts while we do this */
    switch (Sw) {
 
      case true:  outportb(Msk_8259,inportb(Msk_8259)&MskVal_8259[ComDev]);
          break;
 
      case false: outportb(Msk_8259,
                             inportb(Msk_8259)|(0xFF^MskVal_8259[ComDev]));
          break;
    }
    enable();   /* reenable interrupts */
 
  }  /* Set_8259
 
 
/*  This sets the communications baud rate  */
 
void         SetSpeed(ComPort ComDev, ComSpeed ComRate)
  {
int register com;
 
    disable();   /* stop interrupts while we do this */
    com = ComBase[ComDev];
    (*ComPtr[ComDev]).ComCtlSpeed = ComRate;
    outportb(com+ComLineCtl,inportb(com+ComLineCtl)|ComLineDLAB);
    outport(com+ComDivLo,ComBaudDiv[ComRate]);
    outportb(com+ComLineCtl,inportb(com+ComLineCtl)&0x7F);
    enable();   /* reenable interrupts */
 
  }   /* SetSpeed */
 
 
/*  This modifies the line control register for parity & data bits  */
 
void         SetFormat(ComPort ComDev,ComParity ComFormat)
  {
 
    disable();   /* stop interrupts while we do this */
    (*ComPtr[ComDev]).ComCtlParity = ComFormat;
    outportb(ComBase[ComDev]+ComLineCtl,ComLineInit[ComFormat]);
    enable();   /* reenable interrupts */
 
  }   /* SetFormat */
 
 
/*  This modifies the modem control register for DTR, RTS, etc.  */
 
void         SetModem(ComPort ComDev,char ModemCtl)
  {
 
    disable();   /* stop interrupts while we do this */
    outportb(ComBase[ComDev]+ComModemCtl,ModemCtl);
    enable();   /* reenable interrupts */
 
  }   /* SetModem */
 
 
/*  This modifies the communications interrupt enable register  */
 
void         SetEnable(ComPort ComDev,char Enable)
  {
 
    disable();   /* stop interrupts while we do this */
    outportb(ComBase[ComDev]+ComEnable,Enable);
    enable();   /* reenable interrupts */
 
  }   /* SetModem */
 
 
/*  This procedure MUST be called before doing any I/O.  */
 
char         ComOpen(ComPort ComDev,ComSpeed ComRate,ComParity ComFormat)
  {
ComCtlPtr register ptr;
 
    Set_8259(ComDev,false);
    SetEnable(ComDev,0x00);
    ComPtr[ComDev] = ptr = malloc(sizeof(ComCtlRec));
    if (ptr == NULL) return(false);
    (*ptr).LastComIdent = 1;
    (*ptr).InputInIx = 0;
    (*ptr).InputOutIx = 0;
    (*ptr).OutputOutIx = 0;
    (*ptr).OutputInIx = 0;
    SetInterrupt(ComDev,true);
    SetEnable(ComDev,ComEnableRD+ComEnableTX+ComEnableLS+ComEnableMS);
    SetModem(ComDev,ComModemDTR+ComModemRTS+ComModemOUT1+ComModemOUT2);
    SetSpeed(ComDev,ComRate);
    SetFormat(ComDev,ComFormat);
    (*ptr).LastComLineStat = inportb(ComBase[ComDev]+ComLineStat);
    (*ptr).LastComModemStat = inportb(ComBase[ComDev]+ComModemStat);
    Set_8259(ComDev,true);
    return(true);
 
  }   /* ComOpen */
 
 
/*  This procedure shuts-down communications  */
 
void         ComClose(ComPort ComDev)
  {
 
    Set_8259(ComDev,false);
    SetEnable(ComDev,0x00);
    SetInterrupt(ComDev,false);
    SetModem(ComDev,0x00);
    free(ComPtr[ComDev]);
 
  }   /* ComClose */
 
 
/*  This Function returns the modem status  */
 
char         ComStat(ComPort ComDev)
  {
 
    return(inportb(ComBase[ComDev]+ComModemStat));
 
  }   /* ComStat */
 
 
/*  This procedure tells you if there's any input  */
 
char         ComInReady(ComPort ComDev)
  {
ComCtlPtr register ptr;
 
    ptr = ComPtr[ComDev];
    return(!((*ptr).InputInIx == (*ptr).InputOutIx));
 
  }   /* ComInReady */
 
 
/*  This procedure reads input from the ring buffer  */
 
char         ComIn(ComPort ComDev)
  {
char               ret;
ComCtlPtr register ptr;
 
    ptr = ComPtr[ComDev];
    while (!ComInReady(ComDev));
    ret = (*ptr).InputBuffer[(*ptr).InputOutIx];
    (*ptr).InputOutIx = ((*ptr).InputOutIx+1)&ComBufMask;
    return(ret);
 
  }   /* ComIn */
 
 
/*  This procedure writes output, filling the ring buffer if necessary.  */
 
void         ComOut(ComPort ComDev,char Data)
  {
ComCtlPtr register ptr;
 
    ptr = ComPtr[ComDev];
    if  ((inportb(ComBase[ComDev]+ComLineStat)&ComLineTHRE)&&
                ((*ptr).OutputInIx == (*ptr).OutputOutIx))
        outportb(ComBase[ComDev]+ComData,Data);
 
    else
        {
        while ((*ptr).OutputInIx+1&ComBufMask == (*ptr).OutputOutIx);
        (*ptr).OutputBuffer[(*ptr).OutputInIx] = Data;
        (*ptr).OutputInIx = ((*ptr).OutputInIx+1)&ComBufMask;
        }
 
  }   /* ComOut */
