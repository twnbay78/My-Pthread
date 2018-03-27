C	=       gcc
FLAGS   =       -Wall -g -Werror 
COMPILE =       $(CC) $(FLAGS)

all     :	my_pthread malloc

test2	:	test2.c
		$(COMPILE) -pthread -o test2 test2.c structures/stringStack.c structures/direntStack.c

test3	:	Test/test3.c
	 	$(COMPILE) -pthread -o Test/test3 Test/test3.c

test5	:	Test/test5.c
	 	$(COMPILE) -pthread -o Test/test5 Test/test5.c Structures/direntStack.c

my_pthread:	my_pthread.c
		$(COMPILE) -o my_pthread my_pthread.c my_pthread_t.h 

malloc	:	malloc.c
		$(COMPILE) -o malloc malloc.c malloc.h 


clean   :
		rm -rf *.o my_pthread malloc
