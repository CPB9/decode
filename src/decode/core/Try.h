#pragma once

#define TRY(expr) \
    do { \
        if (!expr) { \
            return 0; \
        } \
    } while(0);
