#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1         //POSIX compliant source
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

void signal_handler_IO (int status);    //definition of signal handler
int wait_flag=TRUE;                     //TRUE while no signal received
char devicename[80];
long Baud_Rate = 38400;         // default Baud Rate (110 through 38400)
long BAUD;                      // derived baud rate from command line
long DATABITS;
long STOPBITS;
long PARITYON;
long PARITY;
int Data_Bits = 8;              // Number of data bits
int Stop_Bits = 1;              // Number of stop bits
int Parity = 0;                 // Parity as follows:
                  // 00 = NONE, 01 = Odd, 02 = Even, 03 = Mark, 04 = Space
int Format = 4;
FILE *input;
FILE *output;
int status;


void main(int argc, char *argv[])
{
   char message[90];

   int fd, tty, c, res, i, error;
   char In1, Key;
   struct termios oldtio, newtio;       //for old and new port settings for serial port
   struct termios oldkey, newkey;       //for old and new port settings for keyboard 
   struct sigaction saio;               //definition of signal action
   char buf[255];                       //buffer for where data is put
   
   input = fopen("/dev/tty", "r");      //open the terminal keyboard
   output = fopen("/dev/tty", "w");     //open the terminal screen

   if (!input || !output)
   {
      fprintf(stderr, "Unable to open /dev/tty\n");
      exit(1);
   }

   error=0;
   
   //Hardcoded the serial parameters
   strncpy(devicename, "/dev/ttyS0", 100);
   Baud_Rate = 57600;	
   Data_Bits = 8;
   Stop_Bits = 1;
   Parity = 0;
   Format = 5;

   //run the program
   tty = open("/dev/tty", O_RDWR | O_NOCTTY | O_NONBLOCK); //set the user console port up
   tcgetattr(tty,&oldkey); // save current port settings   
     
   // set new port settings for non-canonical input processing
   newkey.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
   newkey.c_iflag = IGNPAR;
   newkey.c_oflag = 0;
   newkey.c_lflag = 0;       //ICANON;
   newkey.c_cc[VMIN]=1;
   newkey.c_cc[VTIME]=0;
  
   tcflush(tty, TCIFLUSH);
   tcsetattr(tty,TCSANOW,&newkey);

      switch (Baud_Rate)
      {
         case 38400:
         default:
            BAUD = B38400;
            break;
         case 19200:
            BAUD  = B19200;
            break;
         case 9600:
            BAUD  = B9600;
            break;
         case 4800:
            BAUD  = B4800;
            break;
         case 2400:
            BAUD  = B2400;
            break;
         case 1800:
            BAUD  = B1800;
            break;
         case 1200:
            BAUD  = B1200;
            break;
         case 600:
            BAUD  = B600;
            break;
         case 300:
            BAUD  = B300;
            break;
         case 200:
            BAUD  = B200;
            break;
         case 150:
            BAUD  = B150;
            break;
         case 134:
            BAUD  = B134;
            break;
         case 110:
            BAUD  = B110;
            break;
         case 75:
            BAUD  = B75;
            break;
         case 50:
            BAUD  = B50;
            break;
      }  //end of switch baud_rate
      switch (Data_Bits)
      {
         case 8:
         default:
            DATABITS = CS8;
            break;
         case 7:
            DATABITS = CS7;
            break;
         case 6:
            DATABITS = CS6;
            break;
         case 5:
            DATABITS = CS5;
            break;
      }  //end of switch data_bits
      switch (Stop_Bits)
      {
         case 1:
         default:
            STOPBITS = 0;
            break;
         case 2:
            STOPBITS = CSTOPB;
            break;
      }  //end of switch stop bits
      switch (Parity)
      {
         case 0:
         default:                       //none
            PARITYON = 0;
            PARITY = 0;
            break;
         case 1:                        //odd
            PARITYON = PARENB;
            PARITY = PARODD;
            break;
         case 2:                        //even
            PARITYON = PARENB;
            PARITY = 0;
            break;
      }  //end of switch parity
       
      //open the device(com port) to be non-blocking (read will return immediately)
      fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
      if (fd < 0)
      {
         perror(devicename);
         exit(-1);
      }

      //install the serial handler before making the device asynchronous
      saio.sa_handler = signal_handler_IO;
      sigemptyset(&saio.sa_mask);   //saio.sa_mask = 0;
      saio.sa_flags = 0;
      saio.sa_restorer = NULL;
      sigaction(SIGIO,&saio,NULL);

      fcntl(fd, F_SETOWN, getpid());// allow the process to receive SIGIO
      fcntl(fd, F_SETFL, FASYNC);// Make the file descriptor asynchronous 
      
      tcgetattr(fd,&oldtio); // save current port settings 

      // set new port settings for canonical input processing 
      newtio.c_cflag = BAUD | CRTSCTS | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      newtio.c_lflag = 0;       //ICANON;
      newtio.c_cc[VMIN]=1;
      newtio.c_cc[VTIME]=0;

      tcflush(fd, TCIFLUSH);
      tcsetattr(fd,TCSANOW,&newtio);

      // loop while waiting for input. normally we would do something useful here
      while (STOP==FALSE)
      {
         status = fread(&Key,1,1,input);
         if (status==1)  //if a key was hit
         {
            switch (Key)
            { /* branch to appropiate key handler */
               case 0x1b: /* Esc */
                  STOP=TRUE;
                  break;
               default:
                  fputc((int) Key,output);
                  write(fd,&Key,1);          //write 1 byte to the port
                  break;
            }  //end of switch key
         }  //end if a key was hit
         
         // after receiving SIGIO, wait_flag = FALSE, input is available and can be read
         if (wait_flag==FALSE)  //if input is available
         {
            res = read(fd,buf,255);
            if (res>0)
            {
               for (i=0; i<res; i++)  //for all chars in string
               {
                  In1 = buf[i];
                  switch (Format)
                  {
                     case 1:         //hex
                        sprintf(message,"%x ",In1);
                        fputs(message,output);
                        break;
                     case 2:         //decimal
                        sprintf(message,"%d ",In1);
                        fputs(message,output);
                        break;
                     case 3:         //hex and asc
                        if ((In1<32) || (In1>125))
                        {
                           sprintf(message,"%x",In1);
                           fputs(message,output);
                        }
                        else fputc ((int) In1, output);
                        break;
                     case 4:         //decimal and asc
                     default:
                        if ((In1<32) || (In1>125))
                        {
                           sprintf(message,"%d",In1);
                           fputs(message,output);
                        }
                        else fputc ((int) In1, output);
                        break;
                     case 5:         //asc
                        fputc ((int) In1, output);
                        break;
                  }  //end of switch format
               }  //end of for all chars in string
            }  //end if res>0
            wait_flag = TRUE;     
         }  //end if wait flag == FALSE

      // restore old port settings
      tcsetattr(fd,TCSANOW,&oldtio);
      tcsetattr(tty,TCSANOW,&oldkey);
      close(tty);
      close(fd);        //close the com port
   }  //end if command line entrys were correct
   fclose(input);
   fclose(output);
}  //end of main

/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/

void signal_handler_IO (int status)
{
//    printf("received SIGIO signal.\n");
   wait_flag = FALSE;
}

