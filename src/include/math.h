#ifndef MATH_H
#define MATH_H

#define DIVIDE_ROUND_UP(a, b) (((a - 1) / b) + 1)

static inline uint32_t divide_round_up_uint32(uint32_t a, uint32_t b) {
    uint32_t result = a / b;
    if (a % b)
        result++;
    return result;
}

#endif