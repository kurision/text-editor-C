/***include section ***/
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>

/***defines */
#define CTRL_KEY(k) ((k) & 0x1f)
/***data */
struct editorConfig{
	int screenrows;
	int screencols;
	struct termios orig_termios;
};
struct editorConfig E;

/*** terminal */
void die(const char *s){
	write(STDIN_FILENO,"\x1n[2J",4);
	write(STDIN_FILENO,"\x1b[H",3);
	perror(s);
	exit(1);
}

void disableRawMode(){
	if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios)==-1)
		die("tcsetattr");
	
}
void enableRawMode(){
	if(tcgetattr(STDIN_FILENO,&E.orig_termios)== -1) die("tcgetattr");
	atexit(disableRawMode);
	struct termios raw=E.orig_termios;
	raw.c_iflag &=~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_iflag &=~(ICRNL | IXON);
	raw.c_oflag &=~(OPOST);
	raw.c_oflag &=~(CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN]=0;
	raw.c_cc[VTIME]=1;
	
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)== -1) die("tcsetattr");

}

char editorReadKey(){
	int nread;
	char c;
	while((nread=read(STDIN_FILENO,&c,1))!=1){
		if(nread==-1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getCursorPosition(int *rows,int *cols){
	if(write(STDOUT_FILENO,"\x1b[6n",4)!=4) return -1;

	printf("\r\n");
	char c;
	while(read(STDIN_FILENO,&c,1)==1){
		if(iscntrl(c)){
			printf("%d\r\n",c);
		}else{
			printf("%d ('%c')\r\n",c,c);
		}

	}
	editorReadKey();
	return -1;
}
int getWindowSize(int *rows,int *cols){
	struct winsize ws;
	if(1 || ioctl(STDIN_FILENO,TIOCGWINSZ,&ws)==-1 || ws.ws_col == 0){
		if(write(STDIN_FILENO,"\x1b[999C\x1b[999B",12)!=12) return -1;
		return getCursorPosition(rows,cols);
	}else{
		*cols= ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/***output */

void editorDrawRows(){
	int y;
	for(y=0;y<E.screenrows;y++){
		write(STDIN_FILENO,"~\r\n",3);
	}
}

void editorRefreshscreen(){
	write(STDIN_FILENO,"\x1n[2J",4);
	write(STDIN_FILENO,"\x1b[H",3);

	editorDrawRows();
	write(STDIN_FILENO,"\x1b[H",3);
}
/***input */
void editorProcessKeypress(){
	char c = editorReadKey();
	switch(c){
		case CTRL_KEY('q'):
			write(STDIN_FILENO,"\x1n[2J",4);
			write(STDIN_FILENO,"\x1b[H",3);
			exit(0);
			break;
	}
}

/***init */

void initEditor(){
	if(getWindowSize(&E.screenrows,&E.screencols)== -1) die("getWindowsSize");
}


int main(){
	enableRawMode();
	initEditor();
	while(1){	
		editorProcessKeypress();
	};	
	return 0;
}
