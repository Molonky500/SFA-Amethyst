////////////////////////// Function attributes /////////////////////////////
// Place these in front of a function definition/declaration
// to hint about how the function should be optimized and what
// conditions to warn about.
#ifndef COLD
    //Function is rarely used. Optimize for size.
    #define COLD __attribute__((cold))
#endif

#ifndef CONST
    //Function has no side effects and uses no globals.
    //Note that memory pointed to by the function's parameters
    //is also considered global.
    #define CONST __attribute__((__const__))
#endif

#ifndef HOT
    //Function is frequently used. Optimize for speed.
    #define HOT __attribute__((hot))
#endif

#ifndef INLINE
    //Always inline this function.
    #define INLINE inline __attribute__((always_inline))
#endif

#ifndef ISR
    //Function is an Interrupt Service Routine.
    #define ISR __attribute__((interrupt))
#endif

#ifndef MALLOC
    //Function behaves like malloc: it returns either NULL or a pointer
    //which is different from any other valid pointer, and which points to
    //memory which is uninitialized or zero-filled.
    #define MALLOC __attribute__((malloc))
#endif

#ifndef MUST_CHECK
    //Warn if this function's return value isn't used.
    #define MUST_CHECK __attribute__((warn_unused_result))
#endif

#ifndef NAKED
    //Do not generate prologue for this function. (Mostly for startup)
    #define NAKED __attribute__((naked))
#endif

#ifndef NORETURN
    //This function never returns.
    #define NORETURN __attribute__((noreturn))
#endif

#ifndef PURE
    //Function has no side effects.
    #define PURE __attribute__((__pure__))
#endif


//////////////////////////// Variable attributes ///////////////////////////
#ifndef ALIGN
    //Align this variable to x bytes.
    #define ALIGN(x) __attribute__((aligned(x)))
#endif

#ifndef PACKED
    //Do not pad this struct.
    #define PACKED __attribute__((__packed__))
#endif


/////////////////////////// Generic attributes /////////////////////////////
// These can be applied to functions or variables.
#ifndef ALIAS
    //This symbol is an alias for x.
    #define ALIAS(x) __attribute__((alias(x)))
#endif

#ifndef DEPRECATED
    //Warn if this symbol is used.
    #define DEPRECATED __attribute__((deprecated))
#endif

#ifndef SECTION
    //Ask linker to place this in memory section x.
    #define SECTION(x) __attribute__((section(x)))
#endif

#ifndef UNUSED
    //This symbol may go unused. Don't warn if it does.
    #define UNUSED __attribute__((__unused__))
#endif

#ifndef USED
    //This symbol is used, even if it appears not to be.
    //Do not optimize it away.
    #define USED __attribute__((__used__))
#endif

#ifndef WEAK
    //If the host program provides another definition for this symbol,
    //use it instead of this one.
    #define WEAK __attribute__((weak))
#endif


//////////////////////////// Pseudo-statements /////////////////////////////
#ifndef UNREACHABLE
    //Tell the compiler that this statement is never reached.
    #define UNREACHABLE __builtin_unreachable()
#endif

#ifndef prefetch
    //Tell the compiler that we expect to use x soon,
    //so try to load it into cache.
    #define prefetch(x) __builtin_prefetch(x)
#endif


/////////////////////////// Condition attributes ///////////////////////////
// Use these in conditional expressions to hint which case the
// compiler should optimize for:
// if(likely(foo > 0)) do_common_thing(); else do_rare_thing();
// This example hints that "foo > 0" will usually be true, so the compiler
// should optimize based on that assumption.
#ifndef likely
    //x is probably going to be true most of the time.
    #define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
    //x is probably going to be false most of the time.
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif
