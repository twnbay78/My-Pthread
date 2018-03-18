#include <stdio.h>
#include <signal.h>
#include <ucontext.h>

int x = 0;
ucontext_t context;
ucontext_t *cp = &context;

void func(void){
	x++;
	setcontext(cp);
}

// saves main context with getcontext, returns to the main context in func() with setcontext
int main(int argc, char* argv[]){
	getcontext(cp);
	if(x == 0){
		printf("getcontext has been called\n");
		func();
	}else{
		printf("setcontext has been called\n");
	}
	
	return 0;
}
