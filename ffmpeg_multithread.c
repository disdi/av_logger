/**
* \ffmpeg_multithread.c
*/
 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <signal.h> 
#include <fcntl.h>
#include "ffmpeg_multithread.h"
 
#define BAUDRATE B57600
// Current tty console settings variable
static struct termios tty_state_current;
int wait_flag=TRUE;   
volatile int STOP=FALSE;
// Threads 1, 2 and 3 declared as global
pthread_t func_one_thread, func_two_thread, func_three_thread;
 
// Thread conditions
pthread_cond_t thread_cond_one, thread_cond_two, thread_cond_three;
 
// Thread mutex to go with thread conditions
pthread_mutex_t mutex_flag;
 
// Boolean flags
short tflag_one_on, tflag_two_on, tflag_three_on, quit_flag = FALSE;
 
// Counters for threads 1 and 2
int count_one, count_two = 0;

void sig_handler(int signo);
 
// tty console ECHO ON
int echo_on() {
  struct termios tty_state;
 
  if(tcgetattr(0, &tty_state) < 0)
    return -1;
  tty_state.c_lflag |= ECHO;
  return tcsetattr(0, TCSANOW, &tty_state);
}
 
// tty console ECHO OFF
int echo_off() {
  struct termios tty_state;
 
  if(tcgetattr(0, &tty_state) < 0)
    return -1;
  tty_state.c_lflag &= ~ECHO;
  return tcsetattr(0, TCSANOW, &tty_state);
}
 
// Restore tty console settings to its previous original state
void restore_terminal_settings(void)
{
  tcsetattr(0, TCSANOW, &tty_state_current);
}
 
// Disable waiting for ENTER and accept only one keypress as input
void disable_waiting_for_enter(void)
{
  struct termios tty_state;
 
  tcgetattr(0, &tty_state_current);  // Get current tty console settings
  tty_state = tty_state_current;
  tty_state.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &tty_state);
  atexit(restore_terminal_settings); // Very handy function to restore settings
}

void sig_handler(int signo)
{
    printf("received signal.\n");
    if (signo == SIGINT)
    {
     tflag_one_on = FALSE;
     tflag_two_on = FALSE; 	    
     tflag_three_on = FALSE;
    }
    else if (signo == SIGIO)	
    {
      printf("received SIGIO signal.\n");	
      wait_flag = FALSE;
    }
}
 
// Take user keypress and suspend/resume other threads accordingly
void *watch_for_user_keypress()
{
  char getKeyPress;
 
  disable_waiting_for_enter();
  echo_off();
 
  do {
    getKeyPress = getchar();
    switch (getKeyPress) {
      case 49://detecting keypress 1
        if(!tflag_one_on) {
          tflag_one_on = TRUE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_one);
          pthread_mutex_unlock(&mutex_flag);
        } else {
          tflag_one_on = FALSE;
        }
        break;
      case 50://detecting keypress 2
        if(!tflag_two_on) {
          tflag_two_on = TRUE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_two);
          pthread_mutex_unlock(&mutex_flag);
        } else {
          tflag_two_on = FALSE;
        }
        break;
      case 51: //detecting keypress 3
        if(!tflag_three_on) {
          tflag_three_on = TRUE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_three);
          pthread_mutex_unlock(&mutex_flag);
        } else {
          tflag_three_on = FALSE;
        }
    }
  } while(getKeyPress != 48); //detecting keypress 0
 
  quit_flag = TRUE;
 
  pthread_cond_broadcast(&thread_cond_one);
  pthread_cond_broadcast(&thread_cond_two);
  pthread_cond_broadcast(&thread_cond_three);
 
  pthread_exit(NULL);
}
 
void *func_one() 
{
  pthread_mutex_lock(&mutex_flag);
  pthread_cond_wait(&thread_cond_one, &mutex_flag);
  pthread_mutex_unlock(&mutex_flag);
 
  if(quit_flag) pthread_exit(NULL);
 
  while(TRUE)
  {
    
    if(!tflag_one_on) 
    {
      pthread_mutex_lock(&mutex_flag);
      pthread_cond_wait(&thread_cond_one, &mutex_flag);
      pthread_mutex_unlock(&mutex_flag);
    }
    
    if(quit_flag)
         break;
    
    system("ffmpeg -f video4linux2 -r 25 -i /dev/video0 -f  alsa -i plughw:PCH -ar 22050 -ab 64k -strict experimental  -map 0:v:0 -map 1:a:0 -y webcam1.mp4");
    tflag_one_on = FALSE;
    sleep(1);
  
  }
 
  pthread_exit(NULL);
}
 
void *func_two()
{
  pthread_mutex_lock(&mutex_flag);
  pthread_cond_wait(&thread_cond_two, &mutex_flag);
  pthread_mutex_unlock(&mutex_flag);
 
  if(quit_flag)
     pthread_exit(NULL);
 
  while(TRUE)
  {
    
    if(!tflag_two_on)
    {
      pthread_mutex_lock(&mutex_flag);
      pthread_cond_wait(&thread_cond_two, &mutex_flag);
      pthread_mutex_unlock(&mutex_flag);
    }
    
    if(quit_flag) 
       break;
    
    if (signal(SIGINT, sig_handler) == SIG_ERR)
       printf("can’t catch SIGINT\n");

    system("./com");    
    tflag_two_on = FALSE; 
    sleep(1);
  
  }
 
  pthread_exit(NULL);
}
 
int main(int argc, char *argv[])
{
  pthread_t watch_for_user_keypress_thread;
 
  // Thread mutex initialization
  pthread_mutex_init(&mutex_flag, NULL);
 
  // Thread condtions initialization
  pthread_cond_init(&thread_cond_one, NULL);
  pthread_cond_init(&thread_cond_two, NULL);
 
  // Thread creation
  pthread_create(&watch_for_user_keypress_thread, NULL, watch_for_user_keypress, NULL);
  pthread_create(&func_one_thread, NULL, func_one, NULL);
  pthread_create(&func_two_thread, NULL, func_two, NULL);
 
  // main() launches these four threads and starts waiting for their completion
  pthread_join(watch_for_user_keypress_thread, NULL);
  pthread_join(func_one_thread, NULL);
  pthread_join(func_two_thread, NULL);
 
  // All threads exited normally
  printf("\n");
 
  return 0;
}
