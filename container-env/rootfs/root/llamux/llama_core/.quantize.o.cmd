cmd_/root/Llamux/llamux/kernel/llama_core/quantize.o :=  gcc-12 -Wp,-MMD,/root/Llamux/llamux/kernel/llama_core/.quantize.o.d -nostdinc -I/usr/src/linux-headers-6.1.0-37-common/arch/x86/include -I./arch/x86/include/generated -I/usr/src/linux-headers-6.1.0-37-common/include -I./include -I/usr/src/linux-headers-6.1.0-37-common/arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I/usr/src/linux-headers-6.1.0-37-common/include/uapi -I./include/generated/uapi -include /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler-version.h -include /usr/src/linux-headers-6.1.0-37-common/include/linux/kconfig.h -include /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler_types.h -D__KERNEL__ -fmacro-prefix-map=/usr/src/linux-headers-6.1.0-37-common/= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu11 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=none -m64 -falign-jumps=1 -falign-loops=1 -mno-80387 -mno-fp-ret-in-387 -mpreferred-stack-boundary=3 -mskip-rax-setup -mtune=generic -mno-red-zone -mcmodel=kernel -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -mindirect-branch-cs-prefix -mfunction-return=thunk-extern -fno-jump-tables -mharden-sls=all -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member -O2 -fno-allow-store-data-races -Wframe-larger-than=2048 -fstack-protector-strong -Wno-main -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-dangling-pointer -ftrivial-auto-var-init=zero -fno-stack-clash-protection -pg -mrecord-mcount -mfentry -DCC_USING_FENTRY -Wvla -Wno-pointer-sign -Wcast-function-type -Wno-stringop-truncation -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -Wno-array-bounds -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -fno-strict-overflow -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -fno-builtin-wcslen -Wno-packed-not-aligned -g -Wall -Wno-unused-function -DLLAMUX_DEBUG -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -mfma  -DMODULE  -DKBUILD_BASENAME='"quantize"' -DKBUILD_MODNAME='"llama_core"' -D__KBUILD_MODNAME=kmod_llama_core -c -o /root/Llamux/llamux/kernel/llama_core/quantize.o /root/Llamux/llamux/kernel/llama_core/quantize.c   ; ./tools/objtool/objtool --hacks=jump_label --hacks=noinstr --orc --retpoline --rethunk --sls --static-call --uaccess   --module /root/Llamux/llamux/kernel/llama_core/quantize.o

source_/root/Llamux/llamux/kernel/llama_core/quantize.o := /root/Llamux/llamux/kernel/llama_core/quantize.c

deps_/root/Llamux/llamux/kernel/llama_core/quantize.o := \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler-version.h \
    $(wildcard include/config/CC_VERSION_TEXT) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kconfig.h \
    $(wildcard include/config/CPU_BIG_ENDIAN) \
    $(wildcard include/config/BOOGER) \
    $(wildcard include/config/FOO) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler_types.h \
    $(wildcard include/config/DEBUG_INFO_BTF) \
    $(wildcard include/config/PAHOLE_HAS_BTF_TAG) \
    $(wildcard include/config/HAVE_ARCH_COMPILER_H) \
    $(wildcard include/config/CC_HAS_ASM_INLINE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler_attributes.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler-gcc.h \
    $(wildcard include/config/RETPOLINE) \
    $(wildcard include/config/GCC_ASM_GOTO_OUTPUT_WORKAROUND) \
    $(wildcard include/config/ARCH_USE_BUILTIN_BSWAP) \
    $(wildcard include/config/SHADOW_CALL_STACK) \
    $(wildcard include/config/KCOV) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kernel.h \
    $(wildcard include/config/PREEMPT_VOLUNTARY_BUILD) \
    $(wildcard include/config/PREEMPT_DYNAMIC) \
    $(wildcard include/config/HAVE_PREEMPT_DYNAMIC_CALL) \
    $(wildcard include/config/HAVE_PREEMPT_DYNAMIC_KEY) \
    $(wildcard include/config/PREEMPT_) \
    $(wildcard include/config/DEBUG_ATOMIC_SLEEP) \
    $(wildcard include/config/SMP) \
    $(wildcard include/config/MMU) \
    $(wildcard include/config/PROVE_LOCKING) \
    $(wildcard include/config/TRACING) \
    $(wildcard include/config/FTRACE_MCOUNT_RECORD) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/stdarg.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/align.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/const.h \
  /usr/src/linux-headers-6.1.0-37-common/include/vdso/const.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/const.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/limits.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/limits.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/types.h \
    $(wildcard include/config/HAVE_UID16) \
    $(wildcard include/config/UID16) \
    $(wildcard include/config/ARCH_DMA_ADDR_T_64BIT) \
    $(wildcard include/config/PHYS_ADDR_T_64BIT) \
    $(wildcard include/config/64BIT) \
    $(wildcard include/config/ARCH_32BIT_USTAT_F_TINODE) \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/types.h \
  arch/x86/include/generated/uapi/asm/types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/int-ll64.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/int-ll64.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/uapi/asm/bitsperlong.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitsperlong.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/bitsperlong.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/posix_types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/stddef.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/stddef.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler_types.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/X86_32) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/uapi/asm/posix_types_64.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/posix_types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/vdso/limits.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/linkage.h \
    $(wildcard include/config/ARCH_USE_SYM_ANNOTATIONS) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/stringify.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/export.h \
    $(wildcard include/config/MODVERSIONS) \
    $(wildcard include/config/HAVE_ARCH_PREL32_RELOCATIONS) \
    $(wildcard include/config/MODULES) \
    $(wildcard include/config/TRIM_UNUSED_KSYMS) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/compiler.h \
    $(wildcard include/config/TRACE_BRANCH_PROFILING) \
    $(wildcard include/config/PROFILE_ALL_BRANCHES) \
    $(wildcard include/config/OBJTOOL) \
  arch/x86/include/generated/asm/rwonce.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/rwonce.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kasan-checks.h \
    $(wildcard include/config/KASAN_GENERIC) \
    $(wildcard include/config/KASAN_SW_TAGS) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kcsan-checks.h \
    $(wildcard include/config/KCSAN) \
    $(wildcard include/config/KCSAN_WEAK_MEMORY) \
    $(wildcard include/config/KCSAN_IGNORE_ATOMICS) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/linkage.h \
    $(wildcard include/config/X86_64) \
    $(wildcard include/config/X86_ALIGNMENT_16) \
    $(wildcard include/config/RETHUNK) \
    $(wildcard include/config/SLS) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/ibt.h \
    $(wildcard include/config/X86_KERNEL_IBT) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/container_of.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/build_bug.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/err.h \
  arch/x86/include/generated/uapi/asm/errno.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/errno.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/errno-base.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/bitops.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/bits.h \
  /usr/src/linux-headers-6.1.0-37-common/include/vdso/bits.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/typecheck.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/kernel.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/sysinfo.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/generic-non-atomic.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/barrier.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/alternative.h \
    $(wildcard include/config/MITIGATION_ITS) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/asm.h \
    $(wildcard include/config/KPROBES) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/extable_fixup_types.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/bug.h \
    $(wildcard include/config/GENERIC_BUG) \
    $(wildcard include/config/DEBUG_BUGVERBOSE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/instrumentation.h \
    $(wildcard include/config/NOINSTR_VALIDATION) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/objtool.h \
    $(wildcard include/config/FRAME_POINTER) \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bug.h \
    $(wildcard include/config/BUG) \
    $(wildcard include/config/GENERIC_BUG_RELATIVE_POINTERS) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/once_lite.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/panic.h \
    $(wildcard include/config/PANIC_TIMEOUT) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/printk.h \
    $(wildcard include/config/MESSAGE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_QUIET) \
    $(wildcard include/config/EARLY_PRINTK) \
    $(wildcard include/config/PRINTK) \
    $(wildcard include/config/PRINTK_INDEX) \
    $(wildcard include/config/DYNAMIC_DEBUG) \
    $(wildcard include/config/DYNAMIC_DEBUG_CORE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/init.h \
    $(wildcard include/config/STRICT_KERNEL_RWX) \
    $(wildcard include/config/STRICT_MODULE_RWX) \
    $(wildcard include/config/LTO_CLANG) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kern_levels.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/ratelimit_types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/param.h \
  arch/x86/include/generated/uapi/asm/param.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/param.h \
    $(wildcard include/config/HZ) \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/asm-generic/param.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/spinlock_types_raw.h \
    $(wildcard include/config/DEBUG_SPINLOCK) \
    $(wildcard include/config/DEBUG_LOCK_ALLOC) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/spinlock_types.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/qspinlock_types.h \
    $(wildcard include/config/NR_CPUS) \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/qrwlock_types.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/uapi/asm/byteorder.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/byteorder/little_endian.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/byteorder/little_endian.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/swab.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/swab.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/uapi/asm/swab.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/byteorder/generic.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/lockdep_types.h \
    $(wildcard include/config/PROVE_RAW_LOCK_NESTING) \
    $(wildcard include/config/LOCKDEP) \
    $(wildcard include/config/LOCK_STAT) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/dynamic_debug.h \
    $(wildcard include/config/JUMP_LABEL) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/jump_label.h \
    $(wildcard include/config/HAVE_ARCH_JUMP_LABEL_RELATIVE) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/jump_label.h \
    $(wildcard include/config/HAVE_JUMP_LABEL_HACK) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/nops.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/barrier.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/bitops.h \
    $(wildcard include/config/X86_CMOV) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/rmwcc.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/sched.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/arch_hweight.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/cpufeatures.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/required-features.h \
    $(wildcard include/config/X86_MINIMUM_CPU_FAMILY) \
    $(wildcard include/config/MATH_EMULATION) \
    $(wildcard include/config/X86_PAE) \
    $(wildcard include/config/X86_CMPXCHG64) \
    $(wildcard include/config/X86_P6_NOP) \
    $(wildcard include/config/MATOM) \
    $(wildcard include/config/PARAVIRT_XXL) \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/disabled-features.h \
    $(wildcard include/config/X86_UMIP) \
    $(wildcard include/config/X86_INTEL_MEMORY_PROTECTION_KEYS) \
    $(wildcard include/config/X86_5LEVEL) \
    $(wildcard include/config/PAGE_TABLE_ISOLATION) \
    $(wildcard include/config/CPU_UNRET_ENTRY) \
    $(wildcard include/config/INTEL_IOMMU_SVM) \
    $(wildcard include/config/X86_SGX) \
    $(wildcard include/config/INTEL_TDX_GUEST) \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/const_hweight.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/instrumented-atomic.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/instrumented.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kmsan-checks.h \
    $(wildcard include/config/KMSAN) \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/instrumented-non-atomic.h \
    $(wildcard include/config/KCSAN_ASSUME_PLAIN_WRITES_ATOMIC) \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/instrumented-lock.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/le.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/kstrtox.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/log2.h \
    $(wildcard include/config/ARCH_HAS_ILOG2_U32) \
    $(wildcard include/config/ARCH_HAS_ILOG2_U64) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/math.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/div64.h \
  /usr/src/linux-headers-6.1.0-37-common/include/asm-generic/div64.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/minmax.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/static_call_types.h \
    $(wildcard include/config/HAVE_STATIC_CALL) \
    $(wildcard include/config/HAVE_STATIC_CALL_INLINE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/instruction_pointer.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/string.h \
    $(wildcard include/config/BINARY_PRINTF) \
    $(wildcard include/config/FORTIFY_SOURCE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/errno.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/errno.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/overflow.h \
  /usr/src/linux-headers-6.1.0-37-common/include/uapi/linux/string.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/string.h \
  /usr/src/linux-headers-6.1.0-37-common/arch/x86/include/asm/string_64.h \
    $(wildcard include/config/KASAN) \
    $(wildcard include/config/ARCH_HAS_UACCESS_FLUSHCACHE) \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/fortify-string.h \
  /usr/src/linux-headers-6.1.0-37-common/include/linux/bug.h \
    $(wildcard include/config/BUG_ON_DATA_CORRUPTION) \
  /root/Llamux/llamux/kernel/llama_core/quantize.h \
  /root/Llamux/llamux/kernel/llama_core/gguf_parser.h \

/root/Llamux/llamux/kernel/llama_core/quantize.o: $(deps_/root/Llamux/llamux/kernel/llama_core/quantize.o)

$(deps_/root/Llamux/llamux/kernel/llama_core/quantize.o):

/root/Llamux/llamux/kernel/llama_core/quantize.o: $(wildcard ./tools/objtool/objtool)
