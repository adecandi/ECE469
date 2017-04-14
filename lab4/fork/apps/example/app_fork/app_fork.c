#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
	int baby_proc = 0;			// lol sounds like baby croc (this is a variable for the forked process)
	int rand_var = 1994;	// test variable

	//Print pid from original process
	Printf("app_fork: Hello world, i'm the original process. My PID is %d!\n", getpid());
	Printf("app_fork: My variable rand_var = %d\n", rand_var);
	baby_proc = fork();

	if(baby_proc == 0) {
		//child process
		Printf("app_fork: Yo i'm the kid. My id is %d :)\n", getpid());
		Printf("app_fork: My variable rand_var was %d. Child & id = %d\n", rand_var, getpid());
		rand_var = 2017;
		Printf("app_fork: My variable rand_var now is %d. Child & id = %d\n", rand_var, getpid());
	} else {
		//parent 
		Printf("app_fork: Hello i'm the respectable parent. My id is %d :)\n", getpid());
		Printf("app_fork: My variable rand_var was %d. parent & id = %d\n", rand_var, getpid());
		rand_var = 1985;
		Printf("app_fork: My variable rand_var now is %d. parent & id = %d\n", rand_var, getpid());
	}

	Printf("app_fork: %d is out.\n", getpid());
}