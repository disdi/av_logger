The main thread (function main()) creates/initializes and launches three threads which run simultaneously.

They are:

watch_for_user_keypress_thread, which goes into a loop and wait for user input and suspend/resume other three threads according to user keypress. This thread also handles terminal settings (i.e. tty console settings).

func_one_thread invokes ffmpeg
 
func_two_thread shows data received on the serial port.

Inputs to the program are:

1 – is a toggle switch to suspend/resume thread_one for ffmpeg

2 – is a toggle switch to suspend/resume thread_two fro serial reception

0 – ends watch_for_user_keypress and signals all the threads to stop their execution and quit

Other keypress – nothing happens

Anothe file com.c is responsible for displaying data recieved on serial port ttyS0. It has been hard-coded to work at 57600 Baud rate.	

