#ifndef FTIMER_H
#define FTIMER_H

#include <sys/time.h>

class ftimer {
public:

	ftimer() {
		gettimeofday(&tim, NULL);
	}

	void t(const char *name = "") {
		struct timeval newtim;
		gettimeofday(&newtim, NULL);

		u32 d = (newtim.tv_sec - tim.tv_sec) * 1000000;
		d += (newtim.tv_usec - tim.tv_usec);

		printf("%u usec for %s\n", d, name);

		tim = newtim;
	}

private:
	struct timeval tim;
};


#endif
