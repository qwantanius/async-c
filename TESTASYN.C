#include <conio.h>
#include <bios.h>
#include <async.h>
 
void main()
{
char msg1[6] = "ATH\x0D\x0A";
int  i;
int  done = 0;
 
    cputs("Opening Com\n");
    if (ComOpen(Com2,B1200,NoParity)) {
       cputs("Com Open\n");
       cprintf("Modem Status %X\n",ComStat(Com2));
       do{
         for(i=0;i<=sizeof(msg1);i++) {
           ComOut(Com2,msg1[i]);
           putch(msg1[i]);
           }
         while(!ComInReady(Com2)&&!done) {
           if (bioskey(1)) done = 1;};
         while(ComInReady(Com2)) putch(ComIn(Com2));
         }
         while(!done);
       cputs("Done");
       ComClose(Com2);
       }
}
