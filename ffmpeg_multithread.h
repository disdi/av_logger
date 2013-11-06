/**
* \ffmpeg_multithread.h
*/


#define TRUE    1
#define FALSE   0

int echo_on();
int echo_off();
void restore_terminal_settings(void);
void disable_waiting_for_enter(void);
void sig_handler(int signo);
void *watch_for_user_keypress();
void *func_one();
void *func_two();
