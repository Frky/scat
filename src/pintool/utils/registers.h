#ifndef REGISTERS_H_
#define REGISTERS_H_

#include "pin.H"
#include <stdexcept>

/**
 * Register families
 *
 * A set of register corresponding to the same "register memory space"
 * (e.g: REG_RAX, REG_EAX, REG_AX, REG_AH, REG_AL) with different
 * sizes forms a family.
 * Since we mostly care about which space get read & written, this helps
 * to streamline the code.
 */
typedef enum {
    /* ERROR REGISTER */
    REGF_NULL,

    /* Return register */
    REGF_AX,
    REGF_FIRST = REGF_AX,

    /* Parameter registers */
    REGF_DI,
    REGF_SI,
    REGF_DX,
    REGF_CX,
    REGF_R8,
    REGF_R9,

    /* Float registers */
    REGF_XMM0, /* Also a return register */
    REGF_XMM1,
    REGF_XMM2,
    REGF_XMM3,
    REGF_XMM4,
    REGF_XMM5,
    REGF_XMM6,
    REGF_XMM7,
    REGF_LAST = REGF_XMM7,
    REGF_COUNT
} REGF;

#define regf_is_float(regf__) ((regf__) >= REGF_XMM0)

REGF regf(REG reg) {
    switch (reg) {
    case REG_RAX:
    case REG_EAX:
    case REG_AX:
    case REG_AH:
    case REG_AL:
        return REGF_AX;
    case REG_RDI:
    case REG_EDI:
    case REG_DI:
    case REG_DIL:
        return REGF_DI;
    case REG_RSI:
    case REG_ESI:
    case REG_SI:
    case REG_SIL:
        return REGF_SI;
    case REG_RDX:
    case REG_EDX:
    case REG_DX:
    case REG_DH:
    case REG_DL:
        return REGF_DX;
    case REG_RCX:
    case REG_ECX:
    case REG_CX:
    case REG_CH:
    case REG_CL:
        return REGF_CX;
    case REG_R8:
    case REG_R8D:
    case REG_R8W:
    case REG_R8B:
        return REGF_R8;
    case REG_R9:
    case REG_R9D:
    case REG_R9W:
    case REG_R9B:
        return REGF_R9;

    case REG_XMM0:
        return REGF_XMM0;
    case REG_XMM1:
        return REGF_XMM1;
    case REG_XMM2:
        return REGF_XMM2;
    case REG_XMM3:
        return REGF_XMM3;
    case REG_XMM4:
        return REGF_XMM4;
    case REG_XMM5:
        return REGF_XMM5;
    case REG_XMM6:
        return REGF_XMM6;
    case REG_XMM7:
        return REGF_XMM7;
    default:
        // throw new runtime_error("Invalid register");
        // TODO handle this properly
        return REGF_NULL;
    }
}

UINT32 reg_size(REG reg) {
    switch (reg) {
    case REG_RAX:
    case REG_RDI:
    case REG_RSI:
    case REG_RDX:
    case REG_RCX:
    case REG_R8:
    case REG_R9:
        return 64;
    case REG_EAX:
    case REG_EDI:
    case REG_ESI:
    case REG_EDX:
    case REG_ECX:
    case REG_R8D:
    case REG_R9D:
        return 32;
    case REG_AX:
    case REG_DI:
    case REG_SI:
    case REG_DX:
    case REG_CX:
    case REG_R8W:
    case REG_R9W:
        return 16;
    case REG_AH:
    case REG_CH:
    case REG_DH:
    case REG_DIL:
    case REG_SIL:
    case REG_AL:
    case REG_DL:
    case REG_CL:
    case REG_R8B:
    case REG_R9B:
        return 8;
    case REG_XMM0:
    case REG_XMM1:
    case REG_XMM2:
    case REG_XMM3:
    case REG_XMM4:
    case REG_XMM5:
    case REG_XMM6:
    case REG_XMM7:
        return 128;
    default:
        // throw new runtime_error("Invalid register");
        // TODO handle this properly
        return REGF_NULL;
    }
}

string regf_string(REGF regf) {
    switch (regf) {
        case REGF_AX: return "REGF_AX";
        case REGF_DI: return "REGF_DI";
        case REGF_SI: return "REGF_SI";
        case REGF_DX: return "REGF_DX";
        case REGF_CX: return "REGF_CX";
        case REGF_R8: return "REGF_R8";
        case REGF_R9: return "REGF_R9";
        case REGF_XMM0: return "REGF_XMM0";
        case REGF_XMM1: return "REGF_XMM1";
        case REGF_XMM2: return "REGF_XMM2";
        case REGF_XMM3: return "REGF_XMM3";
        case REGF_XMM4: return "REGF_XMM4";
        case REGF_XMM5: return "REGF_XMM5";
        case REGF_XMM6: return "REGF_XMM6";
        case REGF_XMM7: return "REGF_XMM7";
        default: return "Unknown";
    }
}

REG param_reg(unsigned int pid) {
    switch (pid) {
    case 0:
        return REG_RAX;
    case 1:
        return REG_RDI;
    case 2:
        return REG_RSI;
    case 3:
        return REG_RDX;
    case 4:
        return REG_RCX;
    case 5:
        return REG_R8;
    case 6:
        return REG_R9;
    default:
        return REG_INVALID();
    }
}

ADDRINT get_param_value(CONTEXT *ctxt, unsigned int pid) {
    if (pid <= 6)
        return PIN_GetContextReg(ctxt, param_reg(pid));
    else 
        /* TODO take stack parameters into account */
        return 0;
}

#endif
