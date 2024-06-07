#define INV_SCREENW 224
#define INV_SCREENH 256

//the name of the sound file
#define SOUND_FILE "invaders.sound"
//10 kb
#define GAME_AUDIO_BUFFER_SIZE (1024 * 10)

enum invaders_audios{

	ia_explosion,
	ia_fastinvader1,
	ia_fastinvader2,
	ia_fastinvader3,
	ia_fastinvader4,
	ia_invader_killed,
	ia_shoot,
	ia_ufo,
	ia_ufo_hit,

	INVADERS_AUDIO_COUNT,
};

typedef struct{
	int run;
	float sample_index;
	int sample_count;
	int delay;
	int loop;
	unsigned int offset_to_audio_data;
}audio_slot;

typedef struct{
	union{
		unsigned char shift_r8[2];
		unsigned short shift_r;
	};


	unsigned char iport1; // inputs
	unsigned char iport2; // inputs
	unsigned char iport3; //where the value of the shift register goes

	//out port 2, filled in io_in
	unsigned char shift_amount;

	int audio_avadible;
	audio_slot audio_slots[INVADERS_AUDIO_COUNT];
	void *audio_slots_buffer;

	unsigned int audio_buffer_size;
	void *audio_buffer;

}invaders_s;

//out ports
/*

   Space invaders uses the IN and OUT instructions for I/O. It has 8 input and output ports.
   The requested port is the byte that follows the instruction.
   ·IN 3 loads the value of port 3 to the A register
   ·OUT 3 stores the value in port 3 to the A register
*/

static void
run_audio(invaders_s *invaders, int index)
{
	audio_slot *as = invaders->audio_slots + index; 
	/*
	   Loop is only active for the UFO, for non-looping ones the game will
	   keep requesting them, so a "delay" variable is used to prevent these audio
	   effects repeating more than once.

	*/
	if(as->loop)
	{
		as->run = 1;
	}
	else if(!as->delay)
	{
		as->run = 1;
	}
	

	//This variable will decrease by one on the samples loop.
	//A delay of 0 means that this is the first request.
	as->delay = 2;
}

static void
run_audio_loop(invaders_s *invaders, int index)
{
	audio_slot *as = invaders->audio_slots + index; 
	as->run = 1;
}

static void
io_out(program_emu *program)
{
	unsigned char *program8 = program->memory;
    unsigned short ip = program->ip;

	unsigned char port = *(program8 + ip + 1);
	unsigned char valu = *(program8 + ip + 2);

	invaders_s *invaders = program->user;


	invaders->audio_slots[ia_ufo].run = 0;
	if(invaders->audio_slots[ia_ufo].delay == 0)
	{
		invaders->audio_slots[ia_ufo].sample_index = 0;
	}

	switch(port)
	{
		case 2: //SHFTAMNT shift amount. the first 3 bits indicate a shift amount
			{
				invaders->shift_amount = valu & 0x7;
			}break;

			//sound
		case 3:
			{

				if(invaders->audio_avadible == 0)
					break;


				/*
Port 3: (discrete sounds)
 bit 0=UFO (repeats)        SX0 0.raw
 bit 1=Shot                 SX1 1.raw
 bit 2=Flash (player die)   SX2 2.raw
 bit 3=Invader die          SX3 3.raw
 bit 4=Extended play        SX4
 bit 5= AMP enable          SX5
 bit 6= NC (not wired)
 bit 7= NC (not wired)
 Port 4: (discrete sounds)
 bit 0-7 shift data (LSB on 1st write, MSB on 2nd)

Port 5:
 bit 0=Fleet movement 1     SX6 4.raw
 bit 1=Fleet movement 2     SX7 5.raw
 bit 2=Fleet movement 3     SX8 6.raw
 bit 3=Fleet movement 4     SX9 7.raw
 bit 4=UFO Hit              SX10 8.raw
 bit 5= NC (Cocktail mode control ... to flip screen)
 bit 6= NC (not wired)
 bit 7= NC (not wired)
				*/
				//UFO
				if(program->af.ms & 0x01)
				{
					invaders->audio_slots[ia_ufo].loop = 1;
					run_audio(invaders, ia_ufo);
				}
				//SHOT
				if(program->af.ms & 0x02)
				{
					run_audio(invaders, ia_shoot);
				}
				//player dies
				if(program->af.ms & 0x04)
				{
					run_audio(invaders, ia_explosion);
				}
				//invader die
				if(program->af.ms & 0x08)
				{
					run_audio(invaders, ia_invader_killed);
				}

			}break;

		case 4: //SHFT_DATA
			{
				invaders->shift_r8[0] = invaders->shift_r8[1];
				invaders->shift_r8[1] = valu;
			}break;

		case 5: //sound 2
			{
				if(invaders->audio_avadible == 0)
					break;

				//Invaders movement
				if(program->af.ms & 0x01)
				{
					run_audio(invaders, ia_fastinvader1);
				}
				if(program->af.ms & 0x02)
				{
					run_audio(invaders, ia_fastinvader2);
				}
				if(program->af.ms & 0x04)
				{
					run_audio(invaders, ia_fastinvader3);
				}
				if(program->af.ms & 0x08)
				{
					run_audio(invaders, ia_fastinvader4);
				}
				//UFO HIT
				if(program->af.ms & 0x10)
				{
					run_audio(invaders, ia_ufo_hit);
				}
			}break;

		case 6: //WATCHDOG
			break;

		default:
			{
//				gprintf("INVALID PORT {%u} ERROR", port);
			}break;

	}
}

//in ports
static unsigned char
io_in(program_emu *program)
{

	unsigned char *program8 = program->memory;
    unsigned short ip = program->ip;

	unsigned char port = *(program8 + ip + 1);
	unsigned char valu = *(program8 + ip + 2);

	unsigned char result = program->af.ms;

	invaders_s *invaders = program->user;

	switch(port)
	{
		case 0: //Never used ?
			{
			}break;
		case 1:
			{
				result = invaders->iport1;
			}break;

		case 2:
			{
				result = invaders->iport2;
			}break;

		case 3:
			{
				invaders->iport3 = ( (invaders->shift_r << invaders->shift_amount) >> 8 ) & 0xff;
			}break;
	}

	return(result);
}


static void
invaders_input(int key_code, int is_down, int was_down, invaders_s *invaders)
{
#define AD_BIT(port, active, bit) (active) ? ((port) | (bit)) : ((port) & ~(bit))

	switch(key_code)
	{

		//coin
		case kc_6:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x01);
			}break;

		//2 start
		case kc_2:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x02);
			}break;
		//1 start
		case kc_1:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x04);
			}break;

			//1 left
		case kc_a:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x20);
			}break;
			//1 right
		case kc_d:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x40);
			}break;

		case kc_comma:
			{
				invaders->iport1 = AD_BIT(invaders->iport1, is_down, 0x10);
			}break;

		default:
			{
//				gprintf("KEY {%d} NOT REGISTERED", key_code);
			}break;
	}
}

static void
invaders_write(program_emu *program, unsigned int dir, unsigned char val)
{
	//If the program tries to write to rom memory, nothing should happen.
	if(dir >= 0x2000 && dir < 0x4000)
	{
		unsigned char *program8 = program->memory;

		*(program8 + dir) = val;
	}
}

static unsigned char
invaders_readb(program_emu *program, unsigned int dir)
{
	//chao
	if(dir >= 0x6000)
	{
		return(0);
	}
	// Ram mirror. Ram starts at 0x2000
	// I think the game never does this.
	if(dir >= 0x4000 && dir < 0x6000)
	{
		dir = (dir & (0x2000 - 1)) + 0x2000;
	}

	unsigned char result = program->memory[dir];

	return(result);
}

static unsigned short
invaders_reads(program_emu *program, unsigned int dir)
{
	if(dir >= 0x6000)
	{
		return(0);
	}
	if(dir >= 0x4000 && dir < 0x6000)
	{
		dir = (dir & (0x2000 - 1)) + 0x2000;
	}

	unsigned short result = *(unsigned short *)(program->memory + dir);

	return(result);
}
