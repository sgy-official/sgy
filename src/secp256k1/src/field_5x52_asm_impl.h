



#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint64_t *r, const uint64_t *a, const uint64_t * SECP256K1_RESTRICT b) {

  uint64_t tmp1, tmp2, tmp3;
__asm__ __volatile__(
    "movq 0(%%rsi),%%r10\n"
    "movq 8(%%rsi),%%r11\n"
    "movq 16(%%rsi),%%r12\n"
    "movq 24(%%rsi),%%r13\n"
    "movq 32(%%rsi),%%r14\n"

    
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "movq %%rax,%%rcx\n"
    "movq %%rdx,%%r15\n"
    
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "shrdq $52,%%r9,%%r8\n"
    
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    "movq %%rsi,%q1\n"
    
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq %%r8,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    
    "movq %%rsi,%%rax\n"
    "shrq $48,%%rax\n"
    "movq %%rax,%q3\n"
    
    "movq $0xffffffffffff,%%rax\n"
    "andq %%rax,%%rsi\n"
    "movq %%rsi,%q2\n"
    
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq %%rcx,%%rsi\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rsi\n"
    
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    
    "shlq $4,%%rsi\n"
    "movq %q3,%%rax\n"
    "orq %%rax,%%rsi\n"
    
    "movq $0x1000003d1,%%rax\n"
    "mulq %%rsi\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,0(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq %%rcx,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "shrdq $52,%%r15,%%rcx\n"
    "xorq %%r15,%%r15\n"
    
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,8(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "movq 0(%%rbx),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq 8(%%rbx),%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq 16(%%rbx),%%rax\n"
    "mulq %%r10\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %q2,%%rsi\n"
    "movq %q1,%%r10\n"
    
    "movq 24(%%rbx),%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq 32(%%rbx),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rcx\n"
    "adcq %%rdx,%%r15\n"
    
    "movq %%rcx,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "shrdq $52,%%r15,%%rcx\n"
    
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,16(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "addq %%r10,%%r8\n"
    
    "movq %%rcx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r8,%%rax\n"
    "movq $0xfffffffffffff,%%rdx\n"
    "andq %%rdx,%%rax\n"
    "movq %%rax,24(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    
    "addq %%rsi,%%r8\n"
    
    "movq %%r8,32(%%rdi)\n"
: "+S"(a), "=m"(tmp1), "=m"(tmp2), "=m"(tmp3)
: "b"(b), "D"(r)
: "%rax", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "cc", "memory"
);
}

SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint64_t *r, const uint64_t *a) {

  uint64_t tmp1, tmp2, tmp3;
__asm__ __volatile__(
    "movq 0(%%rsi),%%r10\n"
    "movq 8(%%rsi),%%r11\n"
    "movq 16(%%rsi),%%r12\n"
    "movq 24(%%rsi),%%r13\n"
    "movq 32(%%rsi),%%r14\n"
    "movq $0xfffffffffffff,%%r15\n"

    
    "leaq (%%r10,%%r10,1),%%rax\n"
    "mulq %%r13\n"
    "movq %%rax,%%rbx\n"
    "movq %%rdx,%%rcx\n"
    
    "leaq (%%r11,%%r11,1),%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%r14,%%rax\n"
    "mulq %%r14\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "shrdq $52,%%r9,%%r8\n"
    
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    "movq %%rsi,%q1\n"
    
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    
    "addq %%r14,%%r14\n"
    
    "movq %%r10,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "leaq (%%r11,%%r11,1),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%r12,%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%r8,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    
    "movq %%rsi,%%rax\n"
    "shrq $48,%%rax\n"
    "movq %%rax,%q3\n"
    
    "movq $0xffffffffffff,%%rax\n"
    "andq %%rax,%%rsi\n"
    "movq %%rsi,%q2\n"
    
    "movq %%r10,%%rax\n"
    "mulq %%r10\n"
    "movq %%rax,%%r8\n"
    "movq %%rdx,%%r9\n"
    
    "movq %%r11,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "leaq (%%r12,%%r12,1),%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%rbx,%%rsi\n"
    "andq %%r15,%%rsi\n"
    
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    
    "shlq $4,%%rsi\n"
    "movq %q3,%%rax\n"
    "orq %%rax,%%rsi\n"
    
    "movq $0x1000003d1,%%rax\n"
    "mulq %%rsi\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,0(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "addq %%r10,%%r10\n"
    
    "movq %%r10,%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r12,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%r13,%%rax\n"
    "mulq %%r13\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%rbx,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "shrdq $52,%%rcx,%%rbx\n"
    "xorq %%rcx,%%rcx\n"
    
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,8(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "movq %%r10,%%rax\n"
    "mulq %%r12\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %q2,%%rsi\n"
    "movq %q1,%%r10\n"
    
    "movq %%r11,%%rax\n"
    "mulq %%r11\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r13,%%rax\n"
    "mulq %%r14\n"
    "addq %%rax,%%rbx\n"
    "adcq %%rdx,%%rcx\n"
    
    "movq %%rbx,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "shrdq $52,%%rcx,%%rbx\n"
    
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,16(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    "xorq %%r9,%%r9\n"
    
    "addq %%r10,%%r8\n"
    
    "movq %%rbx,%%rax\n"
    "movq $0x1000003d10,%%rdx\n"
    "mulq %%rdx\n"
    "addq %%rax,%%r8\n"
    "adcq %%rdx,%%r9\n"
    
    "movq %%r8,%%rax\n"
    "andq %%r15,%%rax\n"
    "movq %%rax,24(%%rdi)\n"
    
    "shrdq $52,%%r9,%%r8\n"
    
    "addq %%rsi,%%r8\n"
    
    "movq %%r8,32(%%rdi)\n"
: "+S"(a), "=m"(tmp1), "=m"(tmp2), "=m"(tmp3)
: "D"(r)
: "%rax", "%rbx", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "cc", "memory"
);
}

#endif








