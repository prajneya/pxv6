#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[])
{	

	if(argc != 3){
	    fprintf(2, "Usage: %s priority command\n", argv[0]);
	    exit(1);
	}
	else{
		int prev_priority = set_priority(atoi(argv[1]), atoi(argv[2]));
    	printf("The priority of %d has changed from %d to %d \n",atoi(argv[1]), prev_priority, atoi(argv[2]));
    	exit(0);
	}
    
}