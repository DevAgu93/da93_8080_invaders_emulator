////////////////////////

//print inline
static void gprintfi(char *text, ...);
//output contents of the g_print_buffer and clear
static void global_debug_output();
#define gprintf(text, ...) gprintfi(text, __VA_ARGS__); gprintfi("\n");

int g_error = 0;
int g_last_error = 0;

enum return_t{
	return_normal = 0,
	error_invalid_memory,
	error_file_not_found,
	error_invalid_file_size
};

////////////////////////

#include "text_format.c"
#include "invaders.c"
#include "wav.c"


int running = 1;


static unsigned int   aritmetic_flag(short answer, unsigned char *flags);
static void           e8080_write(program_emu *program, unsigned int dir, unsigned char val);
static unsigned char  e8080_readb(program_emu *program, unsigned int dir);
static unsigned short e8080_reads(program_emu *program, unsigned int dir);


static unsigned char
io_inn_zero(program_emu *program) {return(program->af.ms);}
static void 
io_out_zero(program_emu *program){}


#define PAIR(pair) (unsigned short)(((pair).ms << 8) | (pair).ls)
#define REMOVE_FLAG(flags, flag) flags = (((flags)) & ~(flag))

#define COM_OFFSET 0x100

//IN AND OUT INSTRUCTIONS
static unsigned char (*f_inn)(program_emu *program) = io_inn_zero;
static void (*f_out)(program_emu *program) = 0;

//read short, byte and write byte
static unsigned short (*f_e8080_reads)(program_emu *program, unsigned int dir)          = e8080_reads;
static unsigned char (*f_e8080_readb)(program_emu *program, unsigned int dir)           = e8080_readb;
static void (*f_e8080_write)(program_emu *program, unsigned int dir, unsigned char val) = e8080_write;

//S 	Z 	0 	AC 	0 	P 	1 	C 	Flags

enum flag_bits
{
    carry           = 0x01, //(c) set if the last addition operation resulted in a carry or if the last subtraction operation required a borrow
	ALWAYS_ON       = 0x02,
    parity          = 0x04, //(p) set if the number of 1 bits in the result is even.
	//                0x08 always 0
	ac              = 0x10,
	//                0x20 always 0
    zero            = 0x40, //(z) set if the result is zero.
    sign            = 0x80, //(s) set if the result is negative.
};

enum opcodes_e
{
	NOP = 0x00,
	LXI_B = 0x01,
	STAX_B = 0x02,
	INX_B = 0x03,
	INR_B = 0x04,
	DCR_B = 0x05,
	MVI_B = 0x06,
	RLC = 0x07,

	DAD_B = 0x09,
	LDAX_B = 0x0a,
	DCX_B = 0x0b,
	INR_C = 0x0c,
	DCR_C = 0x0d,
	MVI_C_D8 = 0x0e,
	RRC   = 0x0f,

	LXI_D_D16 = 0x11,
	STAX_D = 0x12,
	INX_D = 0x13,
	INR_D = 0x14,
	DCR_D = 0x15,
	MVI_D_D8 = 0x16,
	RAL = 0x17,

	DAD_D = 0x19,
	LDAX_D = 0x1a,
	DCX_D = 0x1b,
	INR_E = 0x1c,
	DCR_E = 0x1d,
	MVI_E_D8 = 0x1e,
	RAR   = 0x1f,
	RIM   = 0x20,
	LXI_H_D16 = 0x21,
	SHLD_adr = 0x22,
	INX_H = 0x23,
	INR_H = 0x24,
	DCR_H = 0x25,
	MVI_H_D8 = 0x26,
	DAA_1 = 0x27,

	DAD_H = 0x29,
	LHLD_adr = 0x2a,
	DCX_H = 0x2b,
	INR_L = 0x2c,
	DCR_L = 0x2d,
	MVI_L_D8 = 0x2e,
	CMA = 0x2f,
	NOP_30 = 0x30,//	SIM = 0x30,
	LXI_SP_D16 = 0x31,
	STA_adr = 0x32,
	INX_SP = 0x33,
	INR_M = 0x34,
	DCR_M = 0x35,
	MVI_M_D8 = 0x36,
	STC = 0x37,

	DAD_SP = 0x39,
	LDA_adr = 0x3a,
	DCX_SP = 0x3b,
	INR_A = 0x3c,
	DCR_A = 0x3d,
	MVI_A_D8 = 0x3e,
	CMC = 0x3f,
	MOV_B_B = 0x40,
	MOV_B_C = 0x41,
	MOV_B_D = 0x42,
	MOV_B_E = 0x43,
	MOV_B_H = 0x44,
	MOV_B_L = 0x45,
	MOV_B_M = 0x46,
	MOV_B_A = 0x47,
	MOV_C_B = 0x48,
	MOV_C_C = 0x49,
	MOV_C_D = 0x4a,
	MOV_C_E = 0x4b,
	MOV_C_H = 0x4c,
	MOV_C_L = 0x4d,
	MOV_C_M = 0x4e,
	MOV_C_A = 0x4f,
	MOV_D_B = 0x50,
	MOV_D_C = 0x51,
	MOV_D_D = 0x52,
	MOV_D_E = 0x53,
	MOV_D_H = 0x54,
	MOV_D_L = 0x55,
	MOV_D_M = 0x56,
	MOV_D_A = 0x57,
	MOV_E_B = 0x58,
	MOV_E_C = 0x59,
	MOV_E_D = 0x5a,
	MOV_E_E = 0x5b,
	MOV_E_H = 0x5c,
	MOV_E_L = 0x5d,
	MOV_E_M = 0x5e,
	MOV_E_A = 0x5f,
	MOV_H_B = 0x60,
	MOV_H_C = 0x61,
	MOV_H_D = 0x62,
	MOV_H_E = 0x63,
	MOV_H_H = 0x64,
	MOV_H_L = 0x65,
	MOV_H_M = 0x66,
	MOV_H_A = 0x67,
	MOV_L_B = 0x68,
	MOV_L_C = 0x69,
	MOV_L_D = 0x6a,
	MOV_L_E = 0x6b,
	MOV_L_H = 0x6c,
	MOV_L_L = 0x6d,
	MOV_L_M = 0x6e,
	MOV_L_A = 0x6f,
	MOV_M_B = 0x70,
	MOV_M_C = 0x71,
	MOV_M_D = 0x72,
	MOV_M_E = 0x73,
	MOV_M_H = 0x74,
	MOV_M_L = 0x75,
	HLT = 0x76,
	MOV_M_A = 0x77,
	MOV_A_B = 0x78,
	MOV_A_C = 0x79,
	MOV_A_D = 0x7a,
	MOV_A_E = 0x7b,
	MOV_A_H = 0x7c,
	MOV_A_L = 0x7d,
	MOV_A_M = 0x7e,
	MOV_A_A = 0x7f,
	ADD_B = 0x80,
	ADD_C = 0x81,
	ADD_D = 0x82,
	ADD_E = 0x83,
	ADD_H = 0x84,
	ADD_L = 0x85,
	ADD_M = 0x86,
	ADD_A = 0x87,
	ADC_B = 0x88,
	ADC_C = 0x89,
	ADC_D = 0x8a,
	ADC_E = 0x8b,
	ADC_H = 0x8c,
	ADC_L = 0x8d,
	ADC_M = 0x8e,
	ADC_A = 0x8f,
	SUB_B = 0x90,
	SUB_C = 0x91,
	SUB_D = 0x92,
	SUB_E = 0x93,
	SUB_H = 0x94,
	SUB_L = 0x95,
	SUB_M = 0x96,
	SUB_A = 0x97,
	SBB_B = 0x98,
	SBB_C = 0x99,
	SBB_D = 0x9a,
	SBB_E = 0x9b,
	SBB_H = 0x9c,
	SBB_L = 0x9d,
	SBB_M = 0x9e,
	SBB_A = 0x9f,
	ANA_B = 0xa0,
	ANA_C = 0xa1,
	ANA_D = 0xa2,
	ANA_E = 0xa3,
	ANA_H = 0xa4,
	ANA_L = 0xa5,
	ANA_M = 0xa6,
	ANA_A = 0xa7,
	XRA_B = 0xa8,
	XRA_C = 0xa9,
	XRA_D = 0xaa,
	XRA_E = 0xab,
	XRA_H = 0xac,
	XRA_L = 0xad,
	XRA_M = 0xae,
	XRA_A = 0xaf,
	ORA_B = 0xb0,
	ORA_C = 0xb1,
	ORA_D = 0xb2,
	ORA_E = 0xb3,
	ORA_H = 0xb4,
	ORA_L = 0xb5,
	ORA_M = 0xb6,
	ORA_A = 0xb7,
	CMP_B = 0xb8,
	CMP_C = 0xb9,
	CMP_D = 0xba,
	CMP_E = 0xbb,
	CMP_H = 0xbc,
	CMP_L = 0xbd,
	CMP_M = 0xbe,
	CMP_A = 0xbf,
	RNZ   = 0xc0,
	POP_B = 0xc1,
	JNZ_adr = 0xc2,
	JMP = 0xc3,
	CNZ_adr = 0xc4,
	PUSH_B  = 0xc5,
	ADI_D8  = 0xc6,
	RST_0 = 0xc7,
	RZ = 0xc8,
	RET_1   = 0xc9,
	JZ_adr  = 0xca,

	CZ_adr = 0xcc,
	CALL_adr = 0xcd,
	ACI_D8 = 0xce,
	RST_1 = 0xcf,
	RNC   = 0xd0,
	POP_D = 0xd1,
	JNC_adr = 0xd2,
	OUTT = 0xd3,
	CNC_adr = 0xd4,
	PUSH_D = 0xd5,
	SUI_D8 = 0xd6,
	RST_2 = 0xd7,
	RC = 0xd8,

	JC_adr = 0xda,
	INN    = 0xdb,
	CC_adr = 0xdc,

	SBI_D8 = 0xde,
	RST_3 = 0xdf,
	RPO   = 0xe0,
	POP_H = 0xe1,
	JPO_adr = 0xe2,
	XTHL = 0xe3,
	CPO_adr = 0xe4,
	PUSH_H = 0xe5,
	ANI_D8 = 0xe6,
	RST_4 = 0xe7,
	RPE = 0xe8,
	PCHL = 0xe9,
	JPE_adr = 0xea,
	XCHG = 0xeb,
	CPE_adr = 0xec,

	XRI_D8 = 0xee,
	RST_5 = 0xef,
	RP   = 0xf0,
	POP_PSW = 0xf1,
	JP_adr = 0xf2,
	DI = 0xf3,
	CP_adr = 0xf4,
	PUSH_PSW = 0xf5,
	ORI_D8 = 0xf6,
	RST_6 = 0xf7,
	RM = 0xf8,
	SPHL = 0xf9,
	JM_adr = 0xfa,
	EI = 0xfb,
	CM_adr = 0xfc,

	CPI_D8 = 0xfe,
	RST_7 = 0xff,

	OPCODE_COUNT
};

struct{
	char *coden;
	short bytes;
	short cycles;
}opcodes[] = {

 { .coden = "NOP"      , .bytes = 1,  .cycles = 4},
 { .coden = "LXI B"    , .bytes = 3,  .cycles = 10},
 { .coden = "STAX B"   , .bytes = 1,  .cycles = 7},
 { .coden = "INX B"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "INR B"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "DCR B"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "MVI B, D8", .bytes = 2,  .cycles = 7} ,
 { .coden = "RLC"      , .bytes = 1,  .cycles = 4} ,
 { .coden = "-"        , .bytes = 1,  .cycles = 4},			
 { .coden = "DAD B"    , .bytes = 1,  .cycles = 10} ,
 { .coden = "LDAX B"   , .bytes = 1,  .cycles = 7} ,
 { .coden = "DCX B"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "INR C"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "DCR C"    , .bytes = 1,  .cycles = 5} ,
 { .coden = "MVI C,D8" , .bytes = 2,  .cycles = 7} ,
 { .coden = "RRC"      , .bytes = 1,  .cycles = 4} ,

 { .coden = "-"        , .bytes = 1,  .cycles = 4},
 { .coden = "LXI D,D16", .bytes = 3,  .cycles = 10},
 { .coden = "STAX D", .bytes = 1,     .cycles = 7},
 { .coden = "INX D", .bytes = 1,      .cycles = 5} ,
 { .coden = "INR D", .bytes = 1,      .cycles = 5} ,
 { .coden = "DCR D", .bytes = 1,      .cycles = 5} ,
 { .coden = "MVI D, D8", .bytes = 2,  .cycles = 7} ,
 { .coden = "RAL", .bytes = 1,        .cycles = 4} ,
 { .coden = "-", .bytes = 1,          .cycles = 4},			
 { .coden = "DAD D", .bytes = 1,      .cycles = 10} ,
 { .coden = "LDAX D", .bytes = 1,     .cycles = 7} ,
 { .coden = "DCX D", .bytes = 1,      .cycles = 5} ,
 { .coden = "INR E", .bytes = 1,      .cycles = 5} ,
 { .coden = "DCR E", .bytes = 1,      .cycles = 5} ,
 { .coden = "MVI E,D8", .bytes = 2,   .cycles = 7} ,
 { .coden = "RAR", .bytes = 1,        .cycles = 4} ,

 { .coden = "RIM", .bytes = 1,        .cycles = 4} ,
 { .coden = "LXI H,D16", .bytes = 3,  .cycles = 10} ,
 { .coden = "SHLD adr", .bytes = 3,   .cycles = 16} ,
 { .coden = "INX H", .bytes = 1,      .cycles = 5} ,
 { .coden = "INR H", .bytes = 1,      .cycles = 5} ,
 { .coden = "DCR H", .bytes = 1,      .cycles = 5} ,
 { .coden = "MVI H,D8", .bytes = 2,   .cycles = 7} ,
 { .coden = "DAA", .bytes = 1,        .cycles = 4} ,
 { .coden = "-", .bytes = 1,          .cycles = 4},			
 { .coden = "DAD H", .bytes = 1,      .cycles = 10} ,
 { .coden = "LHLD adr", .bytes = 3,   .cycles = 16} ,
 { .coden = "DCX H", .bytes = 1,      .cycles = 5} ,
 { .coden = "INR L", .bytes = 1,      .cycles = 5} ,
 { .coden = "DCR L", .bytes = 1,      .cycles = 5} ,
 { .coden = "MVI L, D8", .bytes = 2,  .cycles = 7} ,
 { .coden = "CMA", .bytes = 1,        .cycles = 4} ,

 { .coden = "SIM", .bytes = 1,        .cycles = 4} ,
 { .coden = "LXI SP, D16", .bytes = 3,.cycles = 10} ,
 { .coden = "STA adr", .bytes = 3,    .cycles = 13} ,
 { .coden = "INX SP", .bytes = 1,     .cycles = 5} ,
 { .coden = "INR M", .bytes = 1,      .cycles = 10} ,
 { .coden = "DCR M", .bytes = 1,      .cycles = 10} ,
 { .coden = "MVI M,D8", .bytes = 2,   .cycles = 10} ,
 { .coden = "STC", .bytes = 1,        .cycles = 4} ,
 { .coden = "-", .bytes = 1,          .cycles = 4},			
 { .coden = "DAD SP", .bytes = 1,     .cycles = 10} ,
 { .coden = "LDA adr", .bytes = 3,    .cycles = 13} ,
 { .coden = "DCX SP", .bytes = 1,     .cycles = 5 } ,
 { .coden = "INR A", .bytes = 1,      .cycles = 5} ,
 { .coden = "DCR A", .bytes = 1,      .cycles = 5} ,
 { .coden = "MVI A,D8", .bytes = 2,   .cycles = 7} ,
 { .coden = "CMC", .bytes = 1,        .cycles = 4} ,

 { .coden = "MOV B,B", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV B,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV B,A", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,B", .bytes = 1,    .cycles = 5},	 
 { .coden = "MOV C,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV C,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV C,A", .bytes = 1,    .cycles = 5},

 { .coden = "MOV D,B", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV D,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV D,A", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,B", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV E,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV E,A", .bytes = 1,    .cycles = 5},

 { .coden = "MOV H,B", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV H,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV H,A", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,B", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,C", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,D", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,E", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,H", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,L", .bytes = 1,    .cycles = 5},
 { .coden = "MOV L,M", .bytes = 1,    .cycles = 7},
 { .coden = "MOV L,A", .bytes = 1,    .cycles = 5},

 { .coden = "MOV M,B", .bytes = 1, .cycles = 7},
 { .coden = "MOV M,C", .bytes = 1, .cycles = 7},
 { .coden = "MOV M,D", .bytes = 1, .cycles = 7},
 { .coden = "MOV M,E", .bytes = 1, .cycles = 7},
 { .coden = "MOV M,H", .bytes = 1, .cycles = 7},
 { .coden = "MOV M,L", .bytes = 1, .cycles = 7},
 { .coden = "HLT", .bytes = 1,     .cycles = 7},
 { .coden = "MOV M,A", .bytes = 1, .cycles = 7},
 { .coden = "MOV A,B", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,C", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,D", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,E", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,H", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,L", .bytes = 1, .cycles = 5},
 { .coden = "MOV A,M", .bytes = 1, .cycles = 7},
 { .coden = "MOV A,A", .bytes = 1, .cycles = 5},

 { .coden = "ADD B", .bytes = 1, .cycles = 4},
 { .coden = "ADD C", .bytes = 1, .cycles = 4},
 { .coden = "ADD D", .bytes = 1, .cycles = 4},
 { .coden = "ADD E", .bytes = 1, .cycles = 4},
 { .coden = "ADD H", .bytes = 1, .cycles = 4},
 { .coden = "ADD L", .bytes = 1, .cycles = 4},
 { .coden = "ADD M", .bytes = 1, .cycles = 7},
 { .coden = "ADD A", .bytes = 1, .cycles = 4},
 { .coden = "ADC B", .bytes = 1, .cycles = 4},
 { .coden = "ADC C", .bytes = 1, .cycles = 4},
 { .coden = "ADC D", .bytes = 1, .cycles = 4},
 { .coden = "ADC E", .bytes = 1, .cycles = 4},
 { .coden = "ADC H", .bytes = 1, .cycles = 4},
 { .coden = "ADC L", .bytes = 1, .cycles = 4},
 { .coden = "ADC M", .bytes = 1, .cycles = 7},
 { .coden = "ADC A", .bytes = 1, .cycles = 4},

 { .coden = "SUB B", .bytes = 1, .cycles = 4},
 { .coden = "SUB C", .bytes = 1, .cycles = 4},
 { .coden = "SUB D", .bytes = 1, .cycles = 4},
 { .coden = "SUB E", .bytes = 1, .cycles = 4},
 { .coden = "SUB H", .bytes = 1, .cycles = 4},
 { .coden = "SUB L", .bytes = 1, .cycles = 4},
 { .coden = "SUB M", .bytes = 1, .cycles = 7},
 { .coden = "SUB A", .bytes = 1, .cycles = 4},
 { .coden = "SBB B", .bytes = 1, .cycles = 4},
 { .coden = "SBB C", .bytes = 1, .cycles = 4},
 { .coden = "SBB D", .bytes = 1, .cycles = 4},
 { .coden = "SBB E", .bytes = 1, .cycles = 4},
 { .coden = "SBB H", .bytes = 1, .cycles = 4},
 { .coden = "SBB L", .bytes = 1, .cycles = 4},
 { .coden = "SBB M", .bytes = 1, .cycles = 7},
 { .coden = "SBB A", .bytes = 1, .cycles = 4},

 { .coden = "ANA B", .bytes = 1, .cycles = 4},
 { .coden = "ANA C", .bytes = 1, .cycles = 4},
 { .coden = "ANA D", .bytes = 1, .cycles = 4},
 { .coden = "ANA E", .bytes = 1, .cycles = 4},
 { .coden = "ANA H", .bytes = 1, .cycles = 4},
 { .coden = "ANA L", .bytes = 1, .cycles = 4},
 { .coden = "ANA M", .bytes = 1, .cycles = 7},
 { .coden = "ANA A", .bytes = 1, .cycles = 4},
 { .coden = "XRA B", .bytes = 1, .cycles = 4},
 { .coden = "XRA C", .bytes = 1, .cycles = 4},
 { .coden = "XRA D", .bytes = 1, .cycles = 4},
 { .coden = "XRA E", .bytes = 1, .cycles = 4},
 { .coden = "XRA H", .bytes = 1, .cycles = 4},
 { .coden = "XRA L", .bytes = 1, .cycles = 4},
 { .coden = "XRA M", .bytes = 1, .cycles = 7},
 { .coden = "XRA A", .bytes = 1, .cycles = 4},

 { .coden = "ORA B", .bytes = 1, .cycles = 4},
 { .coden = "ORA C", .bytes = 1, .cycles = 4},
 { .coden = "ORA D", .bytes = 1, .cycles = 4},
 { .coden = "ORA E", .bytes = 1, .cycles = 4},
 { .coden = "ORA H", .bytes = 1, .cycles = 4},
 { .coden = "ORA L", .bytes = 1, .cycles = 4},
 { .coden = "ORA M", .bytes = 1, .cycles = 7},
 { .coden = "ORA A", .bytes = 1, .cycles = 4},
 { .coden = "CMP B", .bytes = 1, .cycles = 4},
 { .coden = "CMP C", .bytes = 1, .cycles = 4},
 { .coden = "CMP D", .bytes = 1, .cycles = 4},
 { .coden = "CMP E", .bytes = 1, .cycles = 4},
 { .coden = "CMP H", .bytes = 1, .cycles = 4},
 { .coden = "CMP L", .bytes = 1, .cycles = 4},
 { .coden = "CMP M", .bytes = 1, .cycles = 7},
 { .coden = "CMP A", .bytes = 1, .cycles = 4},

 { .coden = "RNZ",      .bytes = 1, .cycles = 5},
 { .coden = "POP B",    .bytes = 1, .cycles = 10},
 { .coden = "JNZ adr",  .bytes = 3, .cycles = 10},
 { .coden = "JMP adr",  .bytes = 3, .cycles = 10},
 { .coden = "CNZ adr",  .bytes = 3, .cycles = 11},
 { .coden = "PUSH B",   .bytes = 1, .cycles = 11},
 { .coden = "ADI D8",   .bytes = 2, .cycles = 7},
 { .coden = "RST",      .bytes = 0, .cycles = 11},
 { .coden = "RZ",       .bytes = 1, .cycles = 5},
 { .coden = "RET",      .bytes = 1, .cycles = 10},
 { .coden = "JZ adr",   .bytes = 3, .cycles = 10},
 { .coden = "-",        .bytes = 1, .cycles = 10},
 { .coden = "CZ adr",   .bytes = 3, .cycles = 11},
 { .coden = "CALL adr", .bytes = 3, .cycles = 17},
 { .coden = "ACI D8",   .bytes = 2, .cycles = 7},
 { .coden = "RST",      .bytes = 1, .cycles = 11},

 { .coden = "RNC",     .bytes = 1, .cycles = 5},
 { .coden = "POP D",   .bytes = 1, .cycles = 10},
 { .coden = "JNC adr", .bytes = 3, .cycles = 10},
 { .coden = "OUT D8",  .bytes = 2, .cycles = 10},
 { .coden = "CNC adr", .bytes = 3, .cycles = 11},
 { .coden = "PUSH D",  .bytes = 1, .cycles = 11},
 { .coden = "SUI D8",  .bytes = 2, .cycles = 7},
 { .coden = "RST",     .bytes = 2, .cycles = 11},
 { .coden = "RC",      .bytes = 1, .cycles = 5},
 { .coden = "-",       .bytes = 1, .cycles = 10},
 { .coden = "JC adr",  .bytes = 3, .cycles = 10},
 { .coden = "IN D8",   .bytes = 2, .cycles = 10},
 { .coden = "CC adr",  .bytes = 3, .cycles = 11},
 { .coden = "-",       .bytes = 1, .cycles = 17},
 { .coden = "SBI D8",  .bytes = 2, .cycles = 7},
 { .coden = "RST",     .bytes = 3, .cycles = 11},

 { .coden = "RPO",     .bytes = 1, .cycles = 5},
 { .coden = "POP H",   .bytes = 1, .cycles = 10},
 { .coden = "JPO adr", .bytes = 3, .cycles = 10},
 { .coden = "XTHL",    .bytes = 1, .cycles = 18},
 { .coden = "CPO adr", .bytes = 3, .cycles = 11},
 { .coden = "PUSH H",  .bytes = 1, .cycles = 11},
 { .coden = "ANI D8",  .bytes = 2, .cycles = 7},
 { .coden = "RST",     .bytes = 4, .cycles = 11},
 { .coden = "RPE",     .bytes = 1, .cycles = 5},
 { .coden = "PCHL",    .bytes = 1, .cycles = 5},
 { .coden = "JPE adr", .bytes = 3, .cycles = 10},
 { .coden = "XCHG",    .bytes = 1, .cycles = 4},
 { .coden = "CPE adr", .bytes = 3, .cycles = 11},
 { .coden = "-",       .bytes = 1, .cycles = 17},   		
 { .coden = "XRI D8",  .bytes = 2, .cycles = 7},
 { .coden = "RST",     .bytes = 5, .cycles = 11},

 { .coden = "RP",       .bytes = 1, .cycles = 5},
 { .coden = "POP PSW",  .bytes = 1, .cycles = 10},
 { .coden = "JP adr",   .bytes = 3, .cycles = 10},
 { .coden = "DI",       .bytes = 1, .cycles = 4},
 { .coden = "CP adr",   .bytes = 3, .cycles = 11},
 { .coden = "PUSH PSW", .bytes = 1, .cycles = 11},
 { .coden = "ORI D8",   .bytes = 2, .cycles = 7},
 { .coden = "RST",      .bytes = 6, .cycles = 11},
 { .coden = "RM",       .bytes = 1, .cycles = 5},
 { .coden = "SPHL",     .bytes = 1, .cycles = 5},
 { .coden = "JM adr",   .bytes = 3, .cycles = 10},
 { .coden = "EI",       .bytes = 1, .cycles = 4},
 { .coden = "CM adr",   .bytes = 3, .cycles = 11},
 { .coden = "-",        .bytes = 1, .cycles = 17},   		
 { .coden = "CPI D8",   .bytes = 2, .cycles = 7},
 { .coden = "RST",      .bytes = 7, .cycles = 11},
};


static void
memory_clear(unsigned long long size, void *mem)
{
	unsigned char *mem8 = mem;

	for(int m = 0; m < size; m++)
	{
		mem8[m] = 0;
	}
}

static int
parity_c(short num)
{
	int bitc = 0;
	for(int i = 0; i < 8; i++)
	{
		bitc += num & 0x1;
		num >>= 1;
	}

	return(bitc);
}

static unsigned short
dad(unsigned short v0, unsigned short v1, unsigned char *flags)
{
	unsigned int result = (unsigned int)v0 + v1;

	if((result >> 16) & 0x01)
	{
		*flags |= carry; 
	}
	else 
	{
		*flags &= ~(carry);
	}

	return( (unsigned short)result );
}

static unsigned int
aritmetic_flag(short answer, unsigned char *flags)
{
	unsigned int result = *flags;

	//sign bit is active
	result = (answer & 0x80) ? result | sign : result & ~(sign);
	//zero bit
	result = ((answer & 0xff) == 0) ? result | zero : result & ~(zero);
	//parity, check if it's even
	result = ((parity_c(answer) & 1) == 0) ? result | parity : result & ~(parity);

	*flags = result;

	return(result);
}

static inline unsigned char
aritmetic_op(unsigned short answer, unsigned char *flags)
{
	*flags = ALWAYS_ON;

	//sign bit is active
	*flags |= (answer & 0x80) ? sign : 0;
	//zero bit. If there is a carry and the resulting byte is 0, the flag is set.
	*flags |= ((answer & 0xff) == 0) ? zero : 0;
	//parity, check if it's even
	*flags |= ((parity_c(answer) & 1) == 0) ? parity : 0;
	//carry
	*flags |= (answer > 0xff) ? carry : 0;

	return( (unsigned char) answer);
}


static unsigned char
log_and(unsigned char v0, unsigned char v1, unsigned char *flags)
{
	unsigned char answer = v0 & v1;

	*flags = ALWAYS_ON;

	aritmetic_flag(answer, flags);

	if((v0 | v1) & 0x08)
	{
		*flags |= ac;
	}

	return(answer);
}

static inline unsigned char
aritmetic_add(unsigned short v0, unsigned short v1, unsigned short cf, unsigned char *flags)
{
	unsigned short result = v0 + v1 + cf;

	aritmetic_op(result, flags);

//	if((v0 & 0xf) + (v1 & 0xf) > 0xf)
	if((v0 ^ v1 ^ result) & 0x10)
	{
		*flags |= ac;
	}

	return((unsigned char)result);
}

static inline unsigned char
aritmetic_sub(unsigned short v0, unsigned short v1, unsigned short _carry, unsigned char *flags)
{

	unsigned short result = v0 - (v1 + _carry);

	aritmetic_op(result, flags);

	//if there was a carry, subtraction inverts it.

	if((v0 ^ v1 ^ result) & 0x100)
	{
		*flags |= carry;
	}
	else
	{
		*flags &= ~(carry);
	}

	//if((v0 & 0x8) - (v1 & 0x8) < 0x8)
	if(~(v0 ^ v1 ^ result) & 0x10)
	{
		*flags |= ac;
	}
	else
	{
		*flags &= ~(ac);
	}

	return((unsigned char)result);
}

static void
setp(regp *pair, unsigned short val)
{
	pair->ms = (val >> 8);
	pair->ls = val & 0xff;
}

static inline unsigned char
aritmetic_inr(unsigned short v0, unsigned short v1, unsigned char *flags)
{
	unsigned short result = v0 + v1;

	aritmetic_flag(result, flags);

	if((v0 & 0xf) + (v1 & 0xf) > 0xf)
	{
		*flags |= ac;
	}
	else
	{
		*flags &= ~(ac);
	}

	return((unsigned char)result);
}

static inline unsigned char
aritmetic_dcr(unsigned short v0, unsigned short v1, unsigned char *flags)
{

	unsigned short result = v0 - v1;

	aritmetic_flag(result, flags);

	//if((v0 & 0x8) - (v1 & 0x8) < 0x8)
	if(~(v0 ^ result ^ v1) & 0x10)
	{
		*flags |= ac;

	}
	else
	{
		*flags &= ~(ac);
	}

	return((unsigned char)result);
}

//val0 is mostly the A register (accumulator)
static inline void
cmpf(unsigned short val0, unsigned short val1, unsigned char *flags)
{
//	*flags = ALWAYS_ON;

	unsigned short val = val0 - val1;

	*flags = ALWAYS_ON;
	aritmetic_flag(val, flags);

	if(val0 == val1)
	{
		*flags |= zero;
	}
	else
	{
		REMOVE_FLAG(*flags, zero);
	}


	//val0 + val1 & ~carry
	//if((val0 & 0xf) + (val1 & 0xf) <= 0x100)
//	if(!val1 || ((val0 & 0x0fff) - (val1 & 0x0fff) <= 0))
	if(~(val0 ^ val ^ val1) & 0x10)
	{
		*flags |= ac;
	}


//	val1 = (~val1 & 0xff) + 1;
//	if((val0 + val1) & 0x100)
	if(!(val >> 8))
	{
		REMOVE_FLAG(*flags, carry);
	}
	else
	{
		*flags |= carry;
	}
}

static unsigned short
stack_pop(unsigned char *program, unsigned short *stack_pointer)
{
	unsigned short val = *(unsigned short *)(program + *stack_pointer);
	*stack_pointer = *stack_pointer + 2;

	return( val );
}

static int
stack_push(unsigned char *program, unsigned short ip, unsigned short *stack_pointer)
{

	*stack_pointer -= 2;

	program[(*stack_pointer)] = (unsigned char)(ip & 0xff);
	program[(*stack_pointer) + 1] = (unsigned char)(ip >> 8);

	return(0);
}

static void global_debug_output()
{
	win32_consolew(g_print_buffer_used, g_print_buffer);
	memory_clear(PRINT_BUFFER_SIZE, g_print_buffer);
	g_print_buffer_used = 0;
}

static void
gprintfi(char *text, ...)
{
	static char buffer[1024] = {0};

	//clear

    va_list args;
    va_start_m(args, text);

    unsigned int text_size = format_text_list(buffer, 1024, text, args);

    va_end_m(args);

	if(text_size) 
	{
		//don't count the null terminated character
		text_size--;
		if(g_print_buffer_used + text_size>= PRINT_BUFFER_SIZE)
		{
			global_debug_output();
		}

		for(unsigned int c = 0; c < text_size; c++)
		{
			g_print_buffer[g_print_buffer_used + c] = buffer[c];
		}
		g_print_buffer_used += text_size;

	}
}

static void
e8080_write(program_emu *program, unsigned int dir, unsigned char val)
{
	unsigned char *program8 = program->memory;
	*(program8 + dir) = val;
}

static unsigned char
e8080_readb(program_emu *program, unsigned int dir)
{

	unsigned char result = program->memory[dir];

	return(result);
}

static unsigned short
e8080_reads(program_emu *program, unsigned int dir)
{
	unsigned short result = *(unsigned short *)(program->memory + dir);

	return(result);
}


static int
run_8080(struct program_emu *program_emu)
{
	unsigned char *program8 = program_emu->memory;
	//registers
	program_emu->af.ls |= ALWAYS_ON;
	regp af = program_emu->af;
	regp bc = program_emu->bc;
	regp de = program_emu->de;
	regp hl = program_emu->hl;
	unsigned short ip = program_emu->ip;
	unsigned short sp = program_emu->sp;
	unsigned int cycles = program_emu->cycles;

	unsigned short opcode = 0;

	//frame counter. I used it for debugging and testint
	static unsigned int frame_c = 0;
	int loc_running = 1;

	while(loc_running--)
	{

		opcode = *(program8 + ip);
		int interrupt = 0;

		if(program_emu->proc_interrupts && program_emu->interrupts_on == 1)
		{
			program_emu->proc_interrupts = 0;
			program_emu->interrupts_on = 0;
			program_emu->halted = 0;
			opcode = program_emu->interrupt_requested;

			interrupt = 1;
		}
		if(program_emu->interrupts_on == 2)
		{
			program_emu->interrupts_on = 1;
		}

		if(program_emu->halted)
		{
			break;
		}



		//STOP
//		if(frame_c == 42041)
//		{
//			int s = 0;
//		}


#if 0

		unsigned short d0 = *(program8 + ip);
		unsigned short d1 = *(program8 + ip + 1);
		unsigned short d2 = *(program8 + ip + 2);
		unsigned short d3 = *(program8 + ip + 3);
		if(frame_c > 3000)
		gprintf("ip: {%04x}, cycles: {%u}, AF: {%04x}, BC {%04x} DE: {%04x}, HL: {%04x}, sp: {%04x}, opcode: {%x}  %s (%x, %x, %x, %x)",
				ip, cycles, PAIR(af), PAIR(bc), PAIR(de), PAIR(hl), sp, opcode, opcodes[opcode].coden,
				d0, d1, d2, d3);
#endif


		cycles += opcodes[opcode].cycles;
		frame_c++;

		switch(opcode)
		{
					   //- NOP
			case 0x00: case 0x08: case 0x10: case 0x20: case 0x18: case 0x28: case 0x38: case 0xcb: case 0xdd: case 0xed: case 0xfd: case 0xd9: case 0x30:
					   {
					   }break;

					   //DAD Double add register B, D, H, SP
			case 0x09: setp(&hl, dad( PAIR(hl), PAIR(bc) , &af.ls)); break;
			case 0x19: setp(&hl, dad( PAIR(hl), PAIR(de) , &af.ls)); break;
			case 0x29: setp(&hl, dad( PAIR(hl), PAIR(hl) , &af.ls)); break;
			case 0x39: setp(&hl, dad( PAIR(hl), sp , &af.ls)); break;

					   //MVI B,C,D,E,H,L,M,A
			case 0x06: bc.ms = f_e8080_readb(program_emu, ip + 1); break;
			case 0x0e: bc.ls = f_e8080_readb(program_emu, ip + 1); break;
			case 0x16: de.ms = f_e8080_readb(program_emu, ip + 1); break;
			case 0x1e: de.ls = f_e8080_readb(program_emu, ip + 1); break;
			case 0x26: hl.ms = f_e8080_readb(program_emu, ip + 1); break;
			case 0x2e: hl.ls = f_e8080_readb(program_emu, ip + 1); break;
			case 0x36: f_e8080_write(program_emu, PAIR(hl), f_e8080_readb(program_emu, ip + 1)); break;
			case 0x3e: af.ms = f_e8080_readb(program_emu, ip + 1); break;

					   //INR increment register
					   //B, C, D, E, H, L, M, A
			case 0x04: bc.ms = aritmetic_inr(bc.ms, 1, &af.ls); break;
			case 0x0c: bc.ls = aritmetic_inr(bc.ls, 1, &af.ls); break;
			case 0x14: de.ms = aritmetic_inr(de.ms, 1, &af.ls); break;
			case 0x1c: de.ls = aritmetic_inr(de.ls, 1, &af.ls); break;
			case 0x24: hl.ms = aritmetic_inr(hl.ms, 1, &af.ls); break;
			case 0x2c: hl.ls = aritmetic_inr(hl.ls, 1, &af.ls); break;
			case 0x3c: af.ms = aritmetic_inr(af.ms, 1, &af.ls); break;
			case 0x34: { 
						   unsigned char val = f_e8080_readb(program_emu, PAIR(hl)); 
						   f_e8080_write(program_emu, PAIR(hl), aritmetic_inr(val, 1, &af.ls));
					   } break;

					   //DCR decrement register
			case 0x05: bc.ms = aritmetic_dcr(bc.ms, 1, &af.ls); break;
			case 0x0d: bc.ls = aritmetic_dcr(bc.ls, 1, &af.ls); break;
			case 0x15: de.ms = aritmetic_dcr(de.ms, 1, &af.ls); break;
			case 0x1d: de.ls = aritmetic_dcr(de.ls, 1, &af.ls); break;
			case 0x25: hl.ms = aritmetic_dcr(hl.ms, 1, &af.ls); break;
			case 0x2d: hl.ls = aritmetic_dcr(hl.ls, 1, &af.ls); break;
			case 0x3d: af.ms = aritmetic_dcr(af.ms, 1, &af.ls); break;
			case 0x35: { unsigned char val = f_e8080_readb(program_emu, PAIR(hl));
						   f_e8080_write(program_emu, PAIR(hl), aritmetic_dcr(val, 1, &af.ls)); 
					   } break;

					   //INX B, D, H, SP
			case 0x03: setp(&bc, PAIR(bc) + 1); break;
			case 0x13: setp(&de, PAIR(de) + 1); break;
			case 0x23: setp(&hl, PAIR(hl) + 1); break;
			case 0x33: sp++; break;


					   //LDAX_B
			case 0x0a: af.ms = f_e8080_readb(program_emu, PAIR(bc)); break;
					   //LDAX_D
			case 0x1a: af.ms = f_e8080_readb(program_emu, PAIR(de)); break;
					   //LDA load from adress to register A
			case 0x3a: af.ms = f_e8080_readb(program_emu,  f_e8080_reads(program_emu, ip + 1) ); break;


					   //LXI_B
//			case 0x01: bc.ms = *(program8 + ip + 2); bc.ls = *(program8 + ip + 1); break;
//			case 0x11: de.ms = *(program8 + ip + 2); de.ls = *(program8 + ip + 1); break;
//			case 0x21: hl.ms = *(program8 + ip + 2); hl.ls = *(program8 + ip + 1); break;
//			case 0x31: sp    = (*(program8 + ip + 2) << 8) | *(program8 + ip + 1); break;

			case 0x01: bc.ms = e8080_readb(program_emu, ip + 2); bc.ls = e8080_readb(program_emu, ip + 1); break;
			case 0x11: de.ms = e8080_readb(program_emu, ip + 2); de.ls = e8080_readb(program_emu, ip + 1); break;
			case 0x21: hl.ms = e8080_readb(program_emu, ip + 2); hl.ls = e8080_readb(program_emu, ip + 1); break;
			case 0x31: sp    = (e8080_readb(program_emu, ip + 2) << 8) | e8080_readb(program_emu, ip + 1); break;

					   //mov b (b, c, d, e, h, l, m, a
			case 0x40: bc.ms = bc.ms; break;
			case 0x41: bc.ms = bc.ls; break;
			case 0x42: bc.ms = de.ms; break;
			case 0x43: bc.ms = de.ls; break;
			case 0x44: bc.ms = hl.ms; break;
			case 0x45: bc.ms = hl.ls; break;
			case 0x46: bc.ms = f_e8080_readb(program_emu, PAIR(hl)); break;
			case 0x47: bc.ms = af.ms; break;

			case 0x48: bc.ls = bc.ms; break;
			case 0x49: bc.ls = bc.ls; break;
			case 0x4a: bc.ls = de.ms; break;
			case 0x4b: bc.ls = de.ls; break;
			case 0x4c: bc.ls = hl.ms; break;
			case 0x4d: bc.ls = hl.ls; break;
			case 0x4e: bc.ls = f_e8080_readb(program_emu, PAIR(hl)); break;
			case 0x4f: bc.ls = af.ms; break;

			case MOV_D_B: de.ms = bc.ms; break;
			case MOV_D_C: de.ms = bc.ls; break;
			case MOV_D_D: de.ms = de.ms; break;
			case MOV_D_E: de.ms = de.ls; break;
			case MOV_D_H: de.ms = hl.ms; break;
			case MOV_D_L: de.ms = hl.ls; break;
			case MOV_D_M: de.ms = f_e8080_readb(program_emu, PAIR(hl)); break;
			case MOV_D_A: de.ms = af.ms; break;

			case MOV_E_B: de.ls = bc.ms; break;
			case MOV_E_C: de.ls = bc.ls; break;
			case MOV_E_D: de.ls = de.ms; break;
			case MOV_E_E: de.ls = de.ls; break;
			case MOV_E_H: de.ls = hl.ms; break;
			case MOV_E_L: de.ls = hl.ls; break;
			case MOV_E_M: de.ls = f_e8080_readb(program_emu, PAIR(hl)); break;
			case MOV_E_A: de.ls = af.ms; break;

			case MOV_H_B: hl.ms = bc.ms; break;
			case MOV_H_C: hl.ms = bc.ls; break;
			case MOV_H_D: hl.ms = de.ms; break;
			case MOV_H_E: hl.ms = de.ls; break;
			case MOV_H_H: hl.ms = hl.ms; break;
			case MOV_H_L: hl.ms = hl.ls; break;
			case MOV_H_M: hl.ms = f_e8080_readb(program_emu, PAIR(hl)); break;
			case MOV_H_A: hl.ms = af.ms; break;

			case MOV_L_B: hl.ls = bc.ms; break;
			case MOV_L_C: hl.ls = bc.ls; break;
			case MOV_L_D: hl.ls = de.ms; break;
			case MOV_L_E: hl.ls = de.ls; break;
			case MOV_L_H: hl.ls = hl.ms; break;
			case MOV_L_L: hl.ls = hl.ls; break;
			case MOV_L_M: hl.ls = f_e8080_readb(program_emu, PAIR(hl)); break;
			case MOV_L_A: hl.ls = af.ms; break;


						  //RLC rotate accumulator left
			case 0x07:
						  {
							  //most significant bit is active
							  af.ls = (af.ls & 0xfe) | (af.ms >> 7);
							  af.ms = (af.ms >> 7) | (af.ms << 1);

						  }break;

						  //RRC
						  //shift accumulator right
						  //carry = accumulator least significant
						  //set most significant bit the same as carry
			case 0x0f:
						  {
							  //least significant bit is active
							  if(af.ms & 0x01)
							  {
								  af.ls |= carry;
							  }
							  else
							  {
								  REMOVE_FLAG(af.ls, carry);
							  }

							  af.ms = (af.ms >> 1) | (af.ms << 7);

						  }break;

						  //shift accumulator right
						  //transfer the carry flag to the most significant bit
						  //the carry flag transfers to the least significant bit
						  //RAR
			case 0x1f:
						  {
							  //low order bit
							  unsigned char ls = af.ms & 0x01;

							  af.ms = ( af.ms >> 1 ) | ((af.ls & carry) << 7);
							  //remove the carry flag, and transfer the low order bit of the accumulator
							  //before shifting
							  af.ls = ( af.ls & 0xfe ) | ls;


						  }break;

						  //RAL
			case 0x17:
						  {
							  //high order bit
							  unsigned char ls = af.ms & 0x80;

							  af.ms = ( af.ms << 1 ) | (af.ls & carry);
							  //remove the carry flag, and transfer the high order bit of the accumulator
							  //before shifting
							  af.ls = ( af.ls & 0xfe ) | (ls >> 7);
						  }break;


						  // XCHG Exchange PAIR with DE registers
			case 0xeb:
						  {
							  unsigned char hc = hl.ms;
							  unsigned char lc = hl.ls;

							  hl.ms = de.ms;
							  hl.ls = de.ls;

							  de.ms = hc;
							  de.ls = lc;

						  }break;

						  // POP B, D, H, PSW

			case 0xc1: { unsigned short val = stack_pop(program8, &sp); 
						   setp(&bc, val);   } break;
			case 0xd1: { unsigned short val = stack_pop(program8, &sp); 
						   setp(&de, val);   } break;
			case 0xe1: { unsigned short val = stack_pop(program8, &sp); 
						   setp(&hl, val);   } break;
			case 0xf1: { 
						   unsigned short val = stack_pop(program8, &sp); 
						   af.ms = (val >> 8) & 0xff;
						   af.ls = val & 0xd7;
						   af.ls |= ALWAYS_ON; 
					   } break;

					   /*JC Jump if carry
						 JNZ
						 JZ
						 JNC
						 JPE
						 JPO
						 JM
						 JP
						 */
			case 0xc2: if(!(af.ls & zero))   { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xca: if(af.ls & zero)      { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xd2: if(!(af.ls & carry))  { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xda: if(af.ls & carry)     { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xe2: if(!(af.ls & parity)) { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xea: if(af.ls & parity)    { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xf2: if(!(af.ls & sign))   { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;
			case 0xfa: if(af.ls & sign)      { ip = f_e8080_reads(program_emu, ip + 1); continue; }break;


			case PCHL: ip = PAIR(hl); continue;

			case RST_0: stack_push(program8, ip + !interrupt, &sp); ip = 0x00;continue;
			case 0xcf:  stack_push(program8, ip + !interrupt, &sp); ip = 0x08;continue;
			case RST_2: stack_push(program8, ip + !interrupt, &sp); ip = 0x10;continue;
			case RST_3: stack_push(program8, ip + !interrupt, &sp); ip = 0x18;continue;
			case RST_4: stack_push(program8, ip + !interrupt, &sp); ip = 0x20;continue;
			case RST_5: stack_push(program8, ip + !interrupt, &sp); ip = 0x28;continue;
			case RST_6: stack_push(program8, ip + !interrupt, &sp); ip = 0x30;continue;
			case RST_7: stack_push(program8, ip + !interrupt, &sp); ip = 0x38;continue;

			case JMP:
						{
							unsigned int adr = f_e8080_reads(program_emu, ip + 1);
							ip = adr;

							continue;

						}break;


			case 0xc5: //push B
						{
							stack_push(program8, PAIR(bc), &sp);
						}break;

			case 0xd5: //push D
						{
							stack_push(program8, PAIR(de), &sp);
						}break;

			case 0xe5: //push H
						{
							stack_push(program8, PAIR(hl), &sp);
						}break;


			case 0xc9: //RET
						{

							ip = stack_pop(program8, &sp);

							continue;
						}break;


						//CALL address
			case 0xcd:
						{

							stack_push(program8, (unsigned short)ip + (unsigned short)opcodes[opcode].bytes, &sp);

							unsigned int adr = f_e8080_reads(program_emu, ip + 1);
							//endian swap 16
							//						adr = ((adr >> 8) & 0xff) | ((adr << 8) & 0xff00);
							ip = adr;

							continue;

						}break;

						/*
						   CNZ call if not zero
						   CZ  call if zero
						   CNC
						   CC
						   CPC
						   CPO
						   CPE 
						   CP
						   CM
						   */
			case 0xc4: if(!(af.ls & zero)  ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xcc: if(af.ls & zero     ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xd4: if(!(af.ls & carry) ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xdc: if(af.ls & carry    ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xe4: if(!(af.ls & parity)) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xec: if((af.ls & parity )) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xf4: if(!(af.ls & sign)  ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;
			case 0xfc: if(af.ls & sign     ) { stack_push(program8, ip + opcodes[opcode].bytes, &sp); ip = *(unsigned short *)(program8 + ip + 1); cycles += 6; continue; } break;


						   //add with b, c, d, e, h, l, m, a
			case 0x80: af.ms = aritmetic_add(af.ms, bc.ms, 0, &af.ls); break;
			case 0x81: af.ms = aritmetic_add(af.ms, bc.ls, 0, &af.ls); break;
			case 0x82: af.ms = aritmetic_add(af.ms, de.ms, 0, &af.ls); break;
			case 0x83: af.ms = aritmetic_add(af.ms, de.ls, 0, &af.ls); break;
			case 0x84: af.ms = aritmetic_add(af.ms, hl.ms, 0, &af.ls); break;
			case 0x85: af.ms = aritmetic_add(af.ms, hl.ls, 0, &af.ls); break;
			case 0x86: af.ms = aritmetic_add(af.ms, f_e8080_readb(program_emu, PAIR(hl)), 0, &af.ls); break;
			case 0x87: af.ms = aritmetic_add(af.ms, af.ms, 0, &af.ls); break;

					   //adc with b, c, d, e, h, l, m, a
			case 0x88: af.ms = aritmetic_add(af.ms, (unsigned short )bc.ms, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x89: af.ms = aritmetic_add(af.ms, (unsigned short )bc.ls, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x8a: af.ms = aritmetic_add(af.ms, (unsigned short )de.ms, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x8b: af.ms = aritmetic_add(af.ms, (unsigned short )de.ls, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x8c: af.ms = aritmetic_add(af.ms, (unsigned short )hl.ms, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x8d: af.ms = aritmetic_add(af.ms, (unsigned short )hl.ls, (unsigned short )(af.ls & carry), &af.ls); break;
			case 0x8e: af.ms = aritmetic_add(af.ms, f_e8080_readb(program_emu, PAIR(hl)), (unsigned short )(af.ls & carry), &af.ls); break;

//				case 0x8e: af.ms = aritmetic_add(af.ms, (unsigned short )*(program8 + PAIR(hl)), (unsigned short )(af.ls & carry), &af.ls); break;

			case 0x8f: af.ms = aritmetic_add(af.ms, (unsigned short )af.ms, (unsigned short )(af.ls & carry), &af.ls); break;

					   //ADI
			case 0xc6:
					   {
						   unsigned char val = f_e8080_readb(program_emu, ip + 1);
						   af.ms = aritmetic_add(af.ms, val, 0, &af.ls);
					   }break;


					   //MOV m, c, d, e, h, l, m
			case 0x70: f_e8080_write(program_emu, PAIR(hl), bc.ms); break;
			case 0x71: f_e8080_write(program_emu, PAIR(hl), bc.ls); break;
			case 0x72: f_e8080_write(program_emu, PAIR(hl), de.ms); break;
			case 0x73: f_e8080_write(program_emu, PAIR(hl), de.ls); break;
			case 0x74: f_e8080_write(program_emu, PAIR(hl), hl.ms); break;
			case 0x75: f_e8080_write(program_emu, PAIR(hl), hl.ls); break;
					   //HLT
			case 0x76: program_emu->halted = 1; break;

					   //MOV a
			case 0x77: f_e8080_write(program_emu, PAIR(hl), af.ms); break;

					   //MOV
			case 0x78: af.ms = bc.ms; break;
			case 0x79: af.ms = bc.ls; break;
			case 0x7a: af.ms = de.ms; break;
			case 0x7b: af.ms = de.ls; break;
			case 0x7c: af.ms = hl.ms; break;
			case 0x7d: af.ms = hl.ls; break;
			case 0x7e: af.ms = f_e8080_readb(program_emu, PAIR(hl)); break;
			case 0x7f: af.ms = af.ms; break;

					   //SUB
			case 0x90: af.ms = aritmetic_sub(af.ms, bc.ms, 0, &af.ls); break;
			case 0x91: af.ms = aritmetic_sub(af.ms, bc.ls, 0, &af.ls); break;
			case 0x92: af.ms = aritmetic_sub(af.ms, de.ms, 0, &af.ls); break;
			case 0x93: af.ms = aritmetic_sub(af.ms, de.ls, 0, &af.ls); break;
			case 0x94: af.ms = aritmetic_sub(af.ms, hl.ms, 0, &af.ls); break;
			case 0x95: af.ms = aritmetic_sub(af.ms, hl.ls, 0, &af.ls); break;
			case 0x96: af.ms = aritmetic_sub(af.ms, f_e8080_readb(program_emu, PAIR(hl)), 0, &af.ls); break;
			case 0x97: af.ms = aritmetic_sub(af.ms, af.ms, 0, &af.ls); break;


					   //SBB b, c, d, e, h, l, m, a
			case 0x98: af.ms = aritmetic_sub(af.ms, bc.ms, (af.ls & carry), &af.ls); break;
			case 0x99: af.ms = aritmetic_sub(af.ms, bc.ls, (af.ls & carry), &af.ls); break;
			case 0x9a: af.ms = aritmetic_sub(af.ms, de.ms, (af.ls & carry), &af.ls); break;
			case 0x9b: af.ms = aritmetic_sub(af.ms, de.ls, (af.ls & carry), &af.ls); break;
			case 0x9c: af.ms = aritmetic_sub(af.ms, hl.ms, (af.ls & carry), &af.ls); break;
			case 0x9d: af.ms = aritmetic_sub(af.ms, hl.ls, (af.ls & carry), &af.ls); break;
			case 0x9e: af.ms = aritmetic_sub(af.ms, f_e8080_readb(program_emu, PAIR(hl)), (af.ls & carry), &af.ls); break;
			case 0x9f: af.ms = aritmetic_sub(af.ms, af.ms, (af.ls & carry), &af.ls); break;

					   //ANI And with immediate
			case 0xe6:
					   {
						   unsigned char val1 = f_e8080_readb(program_emu, ip + 1);
						   unsigned short a_cpy = af.ms;

						   af.ms = aritmetic_op(af.ms & val1, &af.ls);
						   REMOVE_FLAG(af.ls, carry);
						   /*
							  The 8080 logical AND instructions set the flag to reflect the logical OR of bit 3
							  of the values involved in the AND operation.
							  */

						   REMOVE_FLAG(af.ls, ac);
						   if((a_cpy | val1) & 0x08)
						   {
							   af.ls |= ac;
						   }
					   } break;

					   //ANA
			case 0xa0: af.ms = log_and(af.ms, bc.ms, &af.ls); break;
			case 0xa1: af.ms = log_and(af.ms, bc.ls, &af.ls); break;
			case 0xa2: af.ms = log_and(af.ms, de.ms, &af.ls); break;
			case 0xa3: af.ms = log_and(af.ms, de.ls, &af.ls); break;
			case 0xa4: af.ms = log_and(af.ms, hl.ms, &af.ls); break;
			case 0xa5: af.ms = log_and(af.ms, hl.ls, &af.ls); break;
			case 0xa6: af.ms = log_and(af.ms, f_e8080_readb(program_emu, PAIR(hl)), &af.ls); break;
			case 0xa7: af.ms = log_and(af.ms, af.ms, &af.ls); break;

					   //XRA
			case 0xa8: af.ms = aritmetic_op(af.ms ^ bc.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xa9: af.ms = aritmetic_op(af.ms ^ bc.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xaa: af.ms = aritmetic_op(af.ms ^ de.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xab: af.ms = aritmetic_op(af.ms ^ de.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xac: af.ms = aritmetic_op(af.ms ^ hl.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xad: af.ms = aritmetic_op(af.ms ^ hl.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xae: af.ms = aritmetic_op(af.ms ^ f_e8080_readb(program_emu, PAIR(hl)), &af.ls); REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xaf: af.ms = aritmetic_op(af.ms ^ af.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;

					   //ORA
			case 0xb0: af.ms = aritmetic_op(af.ms | bc.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb1: af.ms = aritmetic_op(af.ms | bc.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb2: af.ms = aritmetic_op(af.ms | de.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb3: af.ms = aritmetic_op(af.ms | de.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb4: af.ms = aritmetic_op(af.ms | hl.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb5: af.ms = aritmetic_op(af.ms | hl.ls, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb6: af.ms = aritmetic_op(af.ms | f_e8080_readb(program_emu, PAIR(hl)), &af.ls); REMOVE_FLAG(af.ls, carry | ac); break;
			case 0xb7: af.ms = aritmetic_op(af.ms | af.ms, &af.ls);                  REMOVE_FLAG(af.ls, carry | ac); break;

					   //CMP
			case 0xb8: cmpf(af.ms, bc.ms, &af.ls); break;
			case 0xb9: cmpf(af.ms, bc.ls, &af.ls); break;
			case 0xba: cmpf(af.ms, de.ms, &af.ls); break;
			case 0xbb: cmpf(af.ms, de.ls, &af.ls); break;
			case 0xbc: cmpf(af.ms, hl.ms, &af.ls); break;
			case 0xbd: cmpf(af.ms, hl.ls, &af.ls); break;
			case 0xbe: cmpf(af.ms, f_e8080_readb(program_emu, PAIR(hl)), &af.ls); break;
			case 0xbf: cmpf(af.ms, af.ms, &af.ls); break;


					   /* RNZ return if not zero
						  RZ  return if zero
						  RNC return if not carry
						  RC  return if carry
						  RPO return if parity odd
						  RPE return if parity even
						  */
			case 0xc0: if(!(af.ls & zero)    ) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xc8: if((af.ls & zero)     ) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xd0: if(!(af.ls & carry)   ) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xd8: if((af.ls & carry)    ) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xe0: if((!(af.ls & parity))) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xe8: if((af.ls & parity   )) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xf0: if((!(af.ls & sign)  )) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;
			case 0xf8: if((af.ls & sign     )) { ip = stack_pop(program8, &sp); cycles += 6; continue; } break;

						   //ACI
			case 0xce: af.ms = aritmetic_add(af.ms, f_e8080_readb(program_emu, ip + 1), (af.ls & carry), &af.ls); break;

					   //SBI
			case 0xde: af.ms = aritmetic_sub(af.ms, f_e8080_readb(program_emu, ip + 1), (af.ls & carry), &af.ls); break;
					   //SUI
			case 0xd6: af.ms = aritmetic_sub(af.ms, f_e8080_readb(program_emu, ip + 1), 0, &af.ls); break;

					   //ORI
			case 0xf6: af.ms = aritmetic_op(af.ms | f_e8080_readb(program_emu, ip + 1), &af.ls ); REMOVE_FLAG(af.ls, carry | ac); break;
					   //XRI
			case 0xee: af.ms = aritmetic_op(af.ms ^ f_e8080_readb(program_emu, ip + 1), &af.ls ); REMOVE_FLAG(af.ls, carry | ac); break;

					   //CPI_D8
			case 0xfe: //compare immediate with accumulator
					   cmpf(af.ms, f_e8080_readb(program_emu, ip + 1), &af.ls); break;

			case DI: program_emu->interrupts_on = 0; break;
			case EI: program_emu->interrupts_on = 2; break;

					 //DAA
			case 0x27:
					 {
						 int has_ac = af.ls & ac;

						 //4 less significant bits
						 unsigned short l4 = af.ms;

						 if(has_ac || ((l4 & 0xf) > 9))
						 {
							 if((l4 & 0xf) + 6 > 0xf)
							 {
								 af.ls |= ac;
							 }
							 else
							 {
								 af.ls &= ~ac;
							 }

							 l4 += 6;

							 //check carry after the add
							 if(l4 & 0xff00)
							 {
								 af.ls |= carry;
							 }

						 }

						 unsigned short m4 = (l4 >> 4);
						 if((af.ls & carry) || (m4 > 9))
						 {
							 m4 += 6;

							 if(m4 > 0xf)
							 {
								 af.ls |= carry;
							 }

						 }

						 af.ms = (l4 & 0xf) | ((m4 << 4) & 0xf0);
						 aritmetic_flag(af.ms, &af.ls);
					 }break;
					 //CMA
			case 0x2f: af.ms = ~af.ms; break;
					   //						 CMC
			case 0x3f: af.ls ^= 1; break;

			case STC: af.ls |= carry; break;

					  //ROM dependent IN
			case 0xdb:
					  {

						  program_emu->af.ms = f_inn(program_emu);
						  af = program_emu->af;

					  }break;

					   //OUT
			case 0xd3:
					   {
						   f_out(program_emu);
					   }break;


					  //DCX B, D, H, SP
			case 0x0b: setp(&bc, PAIR(bc) - 1); break;
			case 0x1b: setp(&de, PAIR(de) - 1); break;
			case 0x2b: setp(&hl, PAIR(hl) - 1); break;
			case 0x3b: sp--; break;




			case 0xf5: //push PSW
					   {
						   stack_push(program8, (af.ms << 8 | af.ls), &sp);
					   }break;

			case 0xf9: //SPHL
					   {
						   //stack_push(program8, (af.ms << 8 | af.ls), &sp);
						   sp = PAIR(hl);
					   }break;

					   //LHLD load h and l direct
			case 0x2a: {
						   unsigned short adr = f_e8080_reads(program_emu, ip + 1);
						   hl.ls = f_e8080_readb(program_emu, adr);
						   hl.ms = f_e8080_readb(program_emu, adr + 1); 
					   }break;


					   //SHLD
			case 0x22: {
						   unsigned short adr = f_e8080_reads(program_emu, ip + 1);
						   f_e8080_write(program_emu, adr, hl.ls);
						   f_e8080_write(program_emu, adr + 1, hl.ms);
//						   *(program8 + adr)     = hl.ls;
//						   *(program8 + adr + 1) = hl.ms;
					   }break;

					   //STAX Store accumulator in adress B, D, or param
			case 0x02: f_e8080_write(program_emu, PAIR(bc), af.ms); break;
			case 0x12: f_e8080_write(program_emu, PAIR(de), af.ms); break;
			case 0x32: f_e8080_write(program_emu, *(unsigned short*)(program8 + (ip + 1)), af.ms); break;

					   //XTHL
			case 0xe3: {
						   unsigned short hl_c = PAIR(hl);
						   setp(&hl, *(unsigned short *)(program8 + sp));

						   //*(unsigned short *)(program8 + sp) = hl_c;

						   //*(program8 + sp) = hl_c & 0xff;
						   //*(program8 + sp + 1) = (hl_c >> 8);
						   f_e8080_write(program_emu, sp, (hl_c & 0xff));
						   f_e8080_write(program_emu, sp + 1, (hl_c >> 8));

					   } break;

			default:
					   {
						   gprintf("ERROR! CODE %x NOT IMPLEMENTED (ip: %u, frame_c %d)", opcode, ip, frame_c);
						   return(0);
					   }break;
		}

		ip += opcodes[opcode].bytes;

		g_error = ip >= PROGRAM_SIZE ? error_invalid_memory : g_error;
	}

	switch(g_error)
	{

		case error_invalid_memory:
			{
				gprintf("ERROR! INVALID MEMORY DIRECTION! (ip: %u, frame_c: %d)", ip, frame_c);

			}return(0);
	}


	program_emu->af = af;
	program_emu->bc = bc; 
	program_emu->de = de;
	program_emu->hl = hl;
	program_emu->sp = sp;
	program_emu->cycles = cycles;
	program_emu->last_opcode = opcode;
	program_emu->ip = ip;

	return(1);

}


static unsigned int
read_file_mem(unsigned short start_offset, void *file_mem, char *path_and_name)
{
	gprintfi("\n");

	void *handle = win32_open_file(path_and_name);
	unsigned int rom_size = 0;


	if(!handle)
	{
		gprintfi("COULD NOT OPEN OR FIND \"%s\"", path_and_name);
		g_last_error = error_file_not_found;
	}
	else
	{

		unsigned long long file_size = win32_get_file_size(handle);
		rom_size = (unsigned int)file_size;

		//file is empty
		if(!file_size)
		{
			gprintfi("FILE SIZE ERROR!");
			g_last_error = error_invalid_file_size;

			return(0);
		}

		if(file_size + start_offset >= PROGRAM_SIZE)
		{
			gprintf("NOT ENOUGH MEMORY");
			g_last_error = error_invalid_file_size;
			return(0);
		}

		unsigned long long bytes_read = win32_read_from_file(0, file_size, (unsigned char *)file_mem + start_offset, handle);

		//could not read file
		if(!bytes_read)
		{
			gprintf("COULD NOT READ FILE");

			return(0);
		}

		gprintf("FILE \"%s\" OPENED", path_and_name);
		win32_close_file(handle);

	}



	gprintfi("\n");

	return(rom_size);
}

static void
test_out(program_emu *program)
{
	unsigned char *program8 = program->memory;
	unsigned short ip = program->ip;
	regp bc = program->bc;
	regp de = program->de;

	//CP/M output 
	//Before calling 5, the program places the number of the syscall
	//it wants to start into register c and the parameters to the syscall
	//in register DE

	unsigned char port = *(program8 + ip + 1);

	if(port == 0)
	{
		running = 0;
	}
	else if(port == 1)
	{
		int op = bc.ls;

		//This syscall prints the character that corresponds to the ASCII code stored in register e.
		if(op == 2)
		{
			gprintfi("%c", de.ls);
		}
		//This syscall prints all characters stored in memory starting from address (DE),
		//and ending when the character "$" is found.
		else if(op == 9)
		{
			unsigned short adr = PAIR(de);

			while(*(program8 + adr) != '$')
			{
				gprintfi("%c", *(program8 + adr++));
			}
			gprintfi("\n");
		}
	}
}

//Do one allocation for the whole program
static void
program_alloc(struct program_emu *program, unsigned int usr_size)
{

	unsigned char *program_usr = win32_alloc(PROGRAM_SIZE + usr_size);
	memory_clear(PROGRAM_SIZE + usr_size, program_usr);

	program->memory = program_usr;
	if(usr_size)
	{
		program->user = program_usr + PROGRAM_SIZE;
	}
}


static int
run_test(struct program_emu *program)
{

	static char *paths[] =
	{
		"cpu_tests/TST8080.COM",
		"cpu_tests/8080PRE.COM",
		"cpu_tests/CPUTEST.COM",
		"cpu_tests/8080EXM.COM",
	};


	if(!program->loaded)
	{
		program_alloc(program, 0);
		
		//Com files: Since it lacks relocation information, it is loaded by the operating 
		//system at a pre-set address, at offset 0100h immediately

		program->loaded = 1;
		program->ip = COM_OFFSET;

		unsigned char *program8 = program->memory;

		//To quit
		program8[0x00] = 0xD3;
		program8[0x01] = 0x00;

		//call
		program8[0x05] = 0xD3;
		program8[0x06] = 0x01;
		program8[0x07] = 0xC9;

		program8[0x08] = 0x00;
		program8[0x09] = 0x00;

	}

	static int test_i = 0;
	static int reload = 1;

	while(reload)
	{
		if(read_file_mem(COM_OFFSET, program->memory, paths[test_i++]))
		{
			program->ip = COM_OFFSET;
			reload = 0;
		}
		//try with the next file
		else if(test_i < 4)
		{
			reload = 1;
		}
		else
		{
			running = 0;
			return(0);
		}

	}


	if(program->loaded)
	{
		f_inn = io_inn_zero;
		f_out = test_out;

		running = 1;
		run_8080(program);

		if(!running && test_i < 4)
		{
			running = 1;
			reload = 1;
		}

	}


	return(running);

}

static int
game(struct program_emu *program)
{

	if(!program->loaded)
	{
		//first goes struct that contains the game data
		unsigned int extra_size = sizeof(invaders_s);
		unsigned int offset_to_audio_data = 0;

		void *sound_file        = win32_open_file(SOUND_FILE);
		void *entire_sound_file = 0;
		//I know it's ~64 kb.
		unsigned int entire_sound_file_size = 1024 * 65;

		audio_slot temp_audio_slots[INVADERS_AUDIO_COUNT] = {0};
		//sound file was found, add its size to extra_size
		if(sound_file)
		{

			BY_HANDLE_FILE_INFORMATION w32_file_information = {0};
			//fill info.
			GetFileInformationByHandle(sound_file, &w32_file_information);

			if(w32_file_information.nFileSizeLow < entire_sound_file_size)
			{
				entire_sound_file = win32_alloc(entire_sound_file_size);

				win32_read_from_file(0, w32_file_information.nFileSizeLow, entire_sound_file, sound_file);

				unsigned int required_audio_size = 0;

				unsigned char *file_at = entire_sound_file;
				for(int w = 0; w < INVADERS_AUDIO_COUNT; w++)
				{
					unsigned int file_size = *(unsigned int *)file_at;
					file_at += sizeof(unsigned int);

					wave_sound_data sound_data = wave_read_sound_data_from_mem(file_at);

					if(sound_data.total_sound_size)
					{
						file_at += file_size;

						temp_audio_slots[w].sample_count = sound_data.sample_count;
						temp_audio_slots[w].offset_to_audio_data = offset_to_audio_data;

						extra_size += sound_data.total_sound_size;
						offset_to_audio_data += sound_data.total_sound_size;
					}
					else
					{
						gprintf("Error while reading sound file.");
						break;
					}
				}
			}
			else
			{
				gprintf("INVALID SOUND FILE!.");
			}

		}
		else
		{
			gprintf("invaders.sounds NOT FOUND");
		}

		unsigned int audio_buffer_size = GAME_AUDIO_BUFFER_SIZE;

		program_alloc(program, extra_size + audio_buffer_size);
		//open and read the entire rom file
		if(!(program->rom_size = read_file_mem(0x00, program->memory, "invaders.rom")))
		{
			g_error = g_last_error;
			gprintf("The program expects an \"invaders.rom\" file. Instructions are on the readme.");
			running = 0;
			return(0);
		}

		invaders_s *invaders = program->user;
		invaders->audio_buffer_size = audio_buffer_size; 
		invaders->audio_slots_buffer = (unsigned char *)invaders + sizeof(invaders_s) + invaders->audio_buffer_size;

		/*
		   Sound file is valid, read all the samples from the wavs
		*/
		if(entire_sound_file)
		{
			//read the entire sound file after the rom memory.
			invaders->audio_avadible = 1;
			invaders->audio_buffer = (unsigned char *)invaders + sizeof(invaders_s);

			//audio data goes after the invaders struct in memory.
			unsigned char *audio_buffer = invaders->audio_slots_buffer;

			unsigned int audio_buffer_write_offset = 0;

			unsigned char *file_at = entire_sound_file;

			for(int a = 0; a < INVADERS_AUDIO_COUNT; a++)
			{
				invaders->audio_slots[a] = temp_audio_slots[a];
				//read all the audio data after this header.

				//wav file size
				unsigned int current_wav_file_size = *(unsigned int*)(file_at);
				file_at += sizeof(int);

				//read samples.
				int success = wav_read(file_at, audio_buffer + invaders->audio_slots[a].offset_to_audio_data);

				if(!success)
				{
					invaders->audio_avadible = 0;

					gprintf("ERROR WHILE TRYING TO READ SAMPLES!");

					break;
				}

				//skip to the next wav file
				file_at += current_wav_file_size;
			}

			VirtualFree(entire_sound_file, 0, MEM_RELEASE);
		}

		program->loaded = 1;

		f_inn = io_in;
		f_out = io_out;
		f_e8080_readb = invaders_readb;
		f_e8080_reads = invaders_reads;
		f_e8080_write = invaders_write;
	}

	if(program->loaded)
	{
		running = 1;
		run_8080(program);

	}


	return(running);

}
