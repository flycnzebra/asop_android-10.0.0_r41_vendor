
#include <stdlib.h>
#include <stdio.h>

#include <utils/SystemClock.h>


extern  "C" int64_t myUptimeMillis();
//extern int64_t uptimeMillis();

int64_t myUptimeMillis(){

	int64_t startTime =  android::uptimeMillis();
	return startTime;
}

/*int main()
{

	printf("aadsdda   -- %llu",myUptimeMillis());
return 0;
}*/

