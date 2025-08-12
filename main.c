#include <stdio.h>
#include "leo/engine.h"

int main(void) {
    int a = 5, b = 7;
    int s = leo_sum(a, b);
    printf("leo_sum(%d, %d) = %d\n", a, b, s);
    return (s == a + b) ? 0 : 1;
}
