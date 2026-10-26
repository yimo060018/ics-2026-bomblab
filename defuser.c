#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

static uint32_t rol32(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

static uint32_t ror32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
}

/* ---- phase_01: p1_compute ---- */
static uint32_t p1_step0(uint32_t a, uint32_t b) { return (a * 5 + b) ^ 0x100e; }
static uint32_t p1_step1(uint32_t a, uint32_t b) { return (a * 5) ^ (b + 0x8a3d); }
static uint32_t p1_step2(uint32_t a, uint32_t b) { return (a - b) * 3 + 0xd8b9; }

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t lo = (uint32_t)challenge;
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t s2 = p1_step2(hi, lo);
    uint32_t s0 = p1_step0(lo, mid);
    values[0] = p1_step1(s0, s2);

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;

    values[0] = (uint32_t)((challenge >> 12) & 0x7f);
    for (uint32_t i = 1; i <= 5; ++i) {
        uint32_t shift = 20 + 2 * i;
        uint32_t k = (uint32_t)((challenge >> shift) & 0x1f) + i;
        values[i] = 3 * (values[i - 1] ^ k);
    }

    return write_answer(out, cap, values, count);
}

/* ---- phase_03: switch dispatch ---- */
static uint32_t p3_case(uint32_t idx, uint32_t v) {
    switch (idx) {
        case 0: return v * 5 + 0x2d2a8;           /* p3_case_7 */
        case 1: return v * 5 + 0x31a06;           /* p3_case_3 */
        case 2: return v * 5 + 0x4141;            /* p3_case_2 */
        case 3: return (v ^ 0xec1b) + (v >> 2);   /* p3_case_0 */
        case 4: return (v * 4) ^ 0xcb5d;          /* p3_case_5 */
        case 5: return v * 5 + 0x4f25;            /* p3_case_4 */
        case 6: return (v * 4) ^ 0x18bf;          /* p3_case_1 */
        default: return (v ^ 0xbdaf) + (v >> 1);  /* p3_case_6 (idx 7) */
    }
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    uint32_t idx = (uint32_t)(challenge & 7);
    uint32_t v = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = idx;
    values[1] = p3_case(idx, v);

    return write_answer(out, cap, values, count);
}

/* ---- phase_04: recursive p4_walk ---- */
static uint32_t p4_walk(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    uint32_t mid = b + (c - b) / 2;
    if (a == mid) return (mid + 0x28) ^ d;
    uint32_t t = mid + d;
    if (a < mid) {
        return (3 * p4_walk(a, b, mid - 1, d) ^ t) ^ 0x3;
    } else {
        return (3 * p4_walk(a, mid + 1, c, d) ^ t) ^ 0xb;
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t a = (uint32_t)(challenge & 0x7f);
    uint32_t d = (uint32_t)((challenge >> 7) ^ (challenge >> 32));
    values[0] = p4_walk(a, 0, 0x7f, d);

    return write_answer(out, cap, values, count);
}

/* ---- phase_05: linked list traversal ---- */
int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;

    /* node index: next, v1, v2 */
    static const uint32_t next[12] = {
        3, 7, 6, 2, 8, 0, 1, 4, 9, 10, 11, 0xffffffffu
    };
    static const uint32_t v1[12] = {
        0x416, 0x347, 0x50, 0xa2d, 0x96d, 0x683, 0x517, 0xb63,
        0x71c, 0x204, 0x132, 0x86f
    };
    static const uint32_t v2[12] = {
        0x20c7, 0x6092, 0xdfbb, 0x8c19, 0x3494, 0xa390, 0xe3ba, 0x5f95,
        0x42eb, 0xae76, 0xc4b0, 0x52b3
    };
    static const uint32_t table[8] = {5, 0, 3, 2, 6, 1, 7, 4};

    uint32_t node = table[challenge & 7];
    uint32_t acc = (uint32_t)((challenge >> 6) ^ (challenge >> 32));
    acc ^= 0x26e2;
    acc *= 3;
    acc += v2[node];
    uint32_t last = v1[node];
    acc ^= v1[node];

    uint32_t n = (uint32_t)((challenge >> 3) & 3);
    for (uint32_t i = 1; i <= n; ++i) {
        node = next[node];
        acc *= 3;
        acc += v2[node];
        acc ^= v1[node];
        last = v1[node];
    }

    values[0] = acc;
    values[1] = last;

    return write_answer(out, cap, values, count);
}

/* ---- phase_06: function pointer dispatch ---- */
static uint32_t p6_op(uint32_t idx, uint32_t v) {
    switch (idx) {
        case 0: return rol32(v, 7) ^ 0x2af0;            /* p6_op_0 */
        case 1: return (v + 0xa18) ^ (v >> 5);          /* p6_op_4 */
        case 2: return (v * 4 + 0x27b5) ^ (v * 7);      /* p6_op_5 */
        case 3: return (v ^ 0x137f) + 2 * v;            /* p6_op_2 */
        case 4: return (v ^ 0x137f) + 2 * v;            /* p6_op_2 */
        case 5: return (v ^ 0x137f) + 2 * v;            /* p6_op_2 */
        case 6: return ror32(v, 5);                     /* p6_op_1 */
        default: return (v ^ (v >> 7)) + 0x62b5;        /* p6_op_3 (idx 7) */
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;

    uint32_t v = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t idx1 = (uint32_t)(challenge & 7);
    uint32_t idx2 = (uint32_t)((challenge >> 3) & 7);

    uint32_t op1 = p6_op(idx1, v) ^ 0x8f4;
    values[0] = p6_op(idx2, op1) + 0xa287;

    return write_answer(out, cap, values, count);
}

/* ---- secret (Honor) ---- */
/*
 * secret_gate 把输入行 strcpy 到 176 字节的缓冲区，然后：
 *   - 检查 buffer[32..35] == "OPEN"；
 *   - 把 buffer[36..] 交给 secret_phase 解析 "%u %u %u"。
 * 因此第七行 = 32 个任意字节（未被校验）+ "OPEN" + 三个整数。
 */
static const char SECRET_PREFIX[] = "abcdefghijklmnopqrstuvwxyz012345";

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t ch_lo = (uint32_t)challenge;
    uint32_t ch_hi = (uint32_t)(challenge >> 32);
    uint32_t ctx_lo = (uint32_t)context;
    uint32_t ctx_hi = (uint32_t)(context >> 32);

    uint32_t k = (uint32_t)(context & 3);
    uint32_t c16 = (uint32_t)(context >> 16);

    uint32_t chx = ch_hi ^ ch_lo;
    uint32_t ctx_x = ctx_lo ^ ctx_hi;
    uint32_t x = chx ^ ctx_x;
    uint32_t y = (k * 0x8b69) ^ c16;

    uint32_t f1 = k;
    uint32_t f2 = (ror32(x, 1) ^ 0x1f2f) ^ y;
    uint32_t f3 = f2 ^ ((chx >> 1) + ctx_x + k + 0x9446);

    int n = snprintf(out, cap, "%sOPEN %" PRIu32 " %" PRIu32 " %" PRIu32,
                     SECRET_PREFIX, f1, f2, f3);
    if (n < 0 || (size_t)n >= cap) return 1;
    return 0;
}
