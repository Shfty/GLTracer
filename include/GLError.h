#ifndef GLERROR_H
#define GLERROR_H

#include <iostream>

void handle_error();

#define ERROR_PER_CALL

#ifdef ERROR_PER_CALL
#define GL(call) \
    call; \
    handle_error();
#else
#define GL(call) \
    call;
#endif

#endif // GLERROR_H
