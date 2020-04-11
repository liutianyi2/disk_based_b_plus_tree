#ifndef BPLUS_COMMON_H
#define BPLUS_COMMON_H
#define OFFSET_NULL 0xdeadbeef
#define BPLUS_ROUND_UP(ori, r) ((ori & (r - 1)) ? ((ori & ~(r - 1)) + r): ori)
#endif
