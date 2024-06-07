#define _RIFF_CODE 'FFIR'
#define _WAVE_CODE 'EVAW'
#define _FMT_CODE  ' tmf'
#define _DATA_CODE 'atad'

#pragma pack(push)

/*
The WAVE file format start with RIFF header. Then it consists of two subsequent sections
called “fmt ” subchunks and “data” subchunks. The “fmt ” subchunks keeps the information
about the audio data format. The “data” subchunks describes the size information and the
actual audio data.
*/
typedef struct{
	unsigned int chunk_id; //RIFF
	unsigned int chunk_size; //file size - 8
	unsigned int format; //WAVE
	//fmt sub chunk
	//describes sound data format
	unsigned int sub_chunk1_id;
	unsigned int sub_chunk1_size; // 16 for PCM
	unsigned short audio_format; // 1 for PCM
	unsigned short num_channels;
	unsigned int sample_rate; //hz
	unsigned int byte_rate; // sample_rate * num_channels * (bits_per_sample / 8)
	unsigned short block_align; // num_channels * bits_per_sample / 8
	unsigned short bits_per_sample; //8, or 16, or 32...
}wave_header;

/*
   TO SAVE:
   -Crear wave_header e insertar el chunk data debajo
   -Almacenar los samples en 16 bit cada uno.
*/

typedef struct{
	unsigned int id;
	unsigned int size;

	/*//data sub chunk
	unsigned int id; //"data"
	unsigned int size; //NumSamples * NumChannels * BitsPerSample/8
			  //   This is the number of bytes in the data.
			  //   You can also think of this as the size
			  //   of the read of the subchunk following this 
			  //   number.

			  DATA CONTAINS THE SIZE OF THE SAMPLES * NUMBER OF CHANNELS
			  */
}wave_chunk;

typedef struct{

	unsigned short num_channels;
	unsigned int sample_rate;
	unsigned int byte_rate; // sample_rate * num_channels * bits_per_sample / 8
	unsigned short bits_per_sample; //8, or 16, or 32...

	unsigned int total_sound_size;
	unsigned int sound_offset;
	unsigned int sample_count;
}wave_sound_data;
// 8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
//16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767
#pragma pack(pop)

static wave_sound_data 
wave_read_sound_data_from_mem(
	   void *file_mem)	
{
	unsigned char *file_mem8 = file_mem;

	wave_header header = *(wave_header *)file_mem8;
	wave_sound_data result  = {0};

	//invalid wav file
	if(header.chunk_id != _RIFF_CODE)
	{
		return(result);
	}
	//skip chunks until the data chunk

	unsigned int data_offset = sizeof(header);
	wave_chunk chunk = *(wave_chunk *)(file_mem8 + data_offset);
	while(chunk.id != _DATA_CODE)
	{
		//really needed ?
//		if(chunk.size & 1)
//		{
//			chunk.size++;
//		}
		data_offset += chunk.size + sizeof(chunk);
		chunk = *(wave_chunk *)(file_mem8 + data_offset);
	}

	//////////////////////////////////
	//data and fmt chunk found
	if(header.sub_chunk1_id == _FMT_CODE && chunk.id == _DATA_CODE)
	{
		result.num_channels     = header.num_channels;
		result.sample_rate      = header.sample_rate;
		result.byte_rate        = header.byte_rate;
		result.bits_per_sample  = header.bits_per_sample;
		result.total_sound_size = chunk.size;
		result.sound_offset     = data_offset + sizeof(chunk);
		result.sample_count     = chunk.size / ((header.bits_per_sample/8) * header.num_channels);
	}

	return(result);
}

static int
wav_read(void *contents, void *dest)
{
	wave_header header = {0};
	unsigned int data_offset = 0;
	unsigned char *contents8 = contents;
	header = *(wave_header *)(contents8 + data_offset);
	data_offset += sizeof(header);

	//non-PCM
	if(header.audio_format != 1) 
		return(0);
	//only mono or stereo.
	if(header.num_channels > 2)
	{
		return(0);
	}

	//id_check.v = header.chunk_id;
	//format_check.v = header.chunk_id;
	int valid_id = header.chunk_id == _RIFF_CODE; 
	int valid_format = header.format == _WAVE_CODE; 
	int valid_size = 1;

	if(!valid_id || !valid_format || !valid_size) 
		return(0);

	//check fmt sub chunk
	int valid_fmt = header.sub_chunk1_id == _FMT_CODE; 
	if(!valid_fmt)
		return(0);

	wave_chunk chunk = *(wave_chunk *)(contents8 + data_offset);
	//skip chunks until the data chunk
	//Handle other headers?
	while(chunk.id != _DATA_CODE)
	{
		data_offset += chunk.size + sizeof(chunk);
		chunk = *(wave_chunk *)(contents8 + data_offset);
	}
	data_offset += sizeof(chunk);

	/////////////////////////////////////////////

	unsigned int sample_count =  chunk.size / ((header.bits_per_sample/8) * header.num_channels);

	//samples are stored with a size of 16 bit for the files of this game. Load them as such
	if(header.bits_per_sample > 8)
	{
		short *data = (short *)(contents8 + data_offset);
		short *dest16 = dest;
		unsigned int index = 0;
		//NOTA: bits_per_sample debe ser multiplo de 8

		if(header.num_channels == 1)
		{
			short *ch = dest16;

			for(unsigned int s = 0; s < sample_count; s++)
			{
				*ch++ = data[index++];
			}
		}
		else
		{
			short *lc = dest16;
			short *rc = dest16 + (sample_count / 2);

			for(unsigned int s = 0; s < sample_count; s++)
			{
				*lc++ = data[index++];
				*rc++ = data[index++];
			}
		}
	}
	else
	{
		unsigned char *data = contents8 + data_offset;
		unsigned char *dest16 = dest;
		unsigned int index = 0;
		//NOTA: bits_per_sample debe ser multiplo de 8
		if(header.num_channels == 1)
		{
			unsigned char *ch = dest16;

			for(unsigned int s = 0; s < sample_count; s++)
			{
				*ch++ = data[index++];
			}
		}
		else
		{
			unsigned char *lc = dest16;
			unsigned char *rc = dest16 + (sample_count / 2);

			for(unsigned int s = 0; s < sample_count; s++)
			{
				*lc++ = data[index++];
				*rc++ = data[index++];
			}
		}
	}

	return(1);
}
