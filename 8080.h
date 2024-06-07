typedef struct{

		unsigned char ls;
		unsigned char ms;
//	u16 pair;
}regp;

typedef struct program_emu {
	int loaded;
	//The whole program memory
	unsigned char *memory;
	void *user; //other user data

	//Program counter or instruction pointer
	unsigned short ip;
	regp af;
	regp bc;
	regp de;
	regp hl;
	//stack pointer
	unsigned short sp;

	int proc_interrupts;
	int interrupts_on;
	unsigned char next_interrupt;
	unsigned char interrupt_requested;
	unsigned int cycles;
	unsigned int total_cycles;
	unsigned short last_opcode;
	int halted;

	unsigned int rom_size;

} program_emu;

#define PRINT_BUFFER_SIZE (1024 * 64)
static char g_print_buffer[ PRINT_BUFFER_SIZE ];
static unsigned int g_print_buffer_used;

static unsigned int PROGRAM_SIZE = 1024 * 65;
