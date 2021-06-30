#ifndef DEFINES_HPP
#define DEFINES_HPP

//Endianness
#define SB_LITTLE_ENDIAN 1
#define SB_BIG_ENDIAN 2

#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define SB_BYTE_ORDER SB_LITTLE_ENDIAN
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define SB_BYTE_ORDER SB_BIG_ENDIAN
    #else
        #error "Unknown endianness"
    #endif
#elif defined(__LITTLE_ENDIAN__)
    #define SB_BYTE_ORDER SB_BIG_ENDIAN
#elif defined(__BIG_ENDIAN__)
    #define SB_BYTE_ORDER SB_LITTLE_ENDIAN
#elif defined(_MSC_VER) || defined(__i386__) || defined(__x86_64__)
    #define SB_BYTE_ORDER SB_LITTLE_ENDIAN
#else
    #error "Unable to detect endianness"
#endif


#endif //DEFINES_HPP