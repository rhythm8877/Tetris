#include "math.h"

int my_abs(int a){
    return (a < 0) ? -a : a;
}

int my_mul(int a, int b){
    int neg;
    int ua;
    int ub;
    int result = 0;

    if (a == 0 || b == 0)
        return 0;

    neg = ((a < 0) != (b < 0));
    ua  = my_abs(a);
    ub  = my_abs(b);
    while(ub > 0){
        if (ub & 1)  
            result += ua;  
        ua <<= 1;  
        ub >>= 1;  
    }

    return neg ? -result : result;
}

int my_div(int a, int b){
    int neg;
    int ua;
    int ub;
    int quotient;

    if (b == 0)
        return 0;

    neg = ((a < 0) != (b < 0));
    ua  = my_abs(a);
    ub  = my_abs(b);

    quotient = 0;
    for(int i = 31; i >= 0; i--){
        if ((ua >> i) >= ub){
            ua -= (ub << i);
            quotient |= (1U << i);
        }
    }

    return neg ? -quotient : quotient;
}

int my_mod(int a, int b){
    if (b == 0)
        return 0;
    return a - my_mul(my_div(a, b), b);
}

int my_max(int a, int b){
    return (a > b) ? a : b;
}

int my_min(int a, int b){
    return (a < b) ? a : b;
}

int my_clamp(int val, int lo, int hi){
    return my_max(lo, my_min(val, hi));
}

