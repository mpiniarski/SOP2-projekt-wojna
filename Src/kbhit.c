#include"kbhit.h"

#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>


int kbhit(){
    struct termios old;
    struct termios tmp;
    if (tcgetattr(STDIN_FILENO, &old)) return -1;
    memcpy(&tmp, &old, sizeof(old));
    tmp.c_lflag &= ~ICANON & ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios*) &tmp)) return -1;

    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

    tcsetattr(STDIN_FILENO, TCSANOW, (const struct termios*) &old);
    return FD_ISSET(STDIN_FILENO, &fds);
}
