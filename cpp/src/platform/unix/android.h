#ifndef _android_H
#define _android_H

#define pthread_setcancelstate(state, oldstate) ((void)0); *oldstate = 0
#define pthread_cancel(thread) ((void)0)

#endif