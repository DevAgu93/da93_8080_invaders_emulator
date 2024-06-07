
#define INITGUID

#include <windows.h>


#include <audioclient.h>
#include <mmdeviceapi.h>

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator,  0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

static void * win32_alloc(unsigned long long size);
static void * win32_open_file(char *path_and_name);
static void * win32_create_file(char *path_and_name);
static unsigned long long win32_get_file_size(void *handle);
static unsigned int win32_read_from_file(unsigned long long offset, unsigned long long size, void *dst, void *handle);
static int win32_close_file(void *file_handle);

static int get_area_scale(int wind_w, int wind_h, int gamew, int gameh);

static void win32_consolew(int output_count, char *output);

#define ASSERT(condition) if(!(condition)) { *(int *)0 = 0; } 

#include "platform.h"
#include "render.h"
#include "8080.h"
#include "8080.c"
#include "d3d11.c"

typedef unsigned int u32; 

unsigned long long global_performance_frequency = 0;
HANDLE g_console_handle = 0;

int global_win32_client_w = 0;
int global_win32_client_h = 0;
int global_window_scale = 0;


int f1_was_down = 0;
int global_color_on = 1;

/*
   I did this because I planned to implement input remapping
*/
#define g_win32_kc_max 0xff
static unsigned char g_win32_kc[g_win32_kc_max] = 
{
	[0] = kc_null,
	[VK_ESCAPE] = kc_null,
	[VK_UP]    = kc_up,
	[VK_DOWN]  = kc_down,
	[VK_LEFT]  = kc_left,
	[VK_RIGHT] = kc_right,

	[0x30] = kc_0,
	[0x31] = kc_1,
	[0x32] = kc_2,
	[0x33] = kc_3,
	[0x34] = kc_4,
	[0x35] = kc_5,
	[0x36] = kc_6,
	[0x37] = kc_7,
	[0x38] = kc_8,
	[0x39] = kc_9,

 	[0x41] = kc_a,
	[0x42] = kc_b,
	[0x43] = kc_c,
	[0x44] = kc_d,
	[0x45] = kc_e,
	[0x46] = kc_f,
	[0x47] = kc_g,
	[0x48] = kc_h,
	[0x49] = kc_i,
	[0x4A] = kc_j,
	[0x4B] = kc_k,
	[0x4C] = kc_k,
	[0x4D] = kc_m,
	[0x4E] = kc_n,
	[0x4F] = kc_o,
	[0x50] = kc_p,
	[0x51] = kc_q,
	[0x52] = kc_r,
	[0x53] = kc_s,
	[0x54] = kc_t,
	[0x55] = kc_u,
	[0x56] = kc_v,
	[0x57] = kc_w,
	[0x58] = kc_x,
	[0x59] = kc_y,
	[0x5A] = kc_z,
	
	[VK_OEM_COMMA] = kc_comma,
};


/*
   I used this to pack the sound files

   The sound pack is basically an integer indicating the file size, and the wav file.
*/
static void
pack_sound_file()
{
	void *pack_handle = win32_create_file(SOUND_FILE);

	if(!pack_handle)
	{
		gprintf("PACK FILE CREATION ERROR");

		return;
	}

    WIN32_FIND_DATAA w32_file_found;
    HANDLE find_handle = FindFirstFileA("invaders/sound/*.wav", &w32_file_found);

	int found = find_handle != INVALID_HANDLE_VALUE;


	char path_buffer[256] = "invaders/sound/";
	int index_to_null_terminated = 15;

	unsigned int write_offset = 0;
	//100 kb, because the sound files aren't even close to being this big
	unsigned char entire_file[1024 * 100];

	while(found)
	{

		//full path name
		char *fn = w32_file_found.cFileName;
		int i = 0;
		for(char c = fn[i]; c != '\0'; c = fn[i])
		{
			path_buffer[index_to_null_terminated + i++] = c;
		}
		path_buffer[index_to_null_terminated + i] = '\0';

		void *wav_file_handle = win32_open_file(path_buffer);

		if(wav_file_handle)
		{


			u32 bytes_read = 0;
			OVERLAPPED overlapped = {0};
			ReadFile(wav_file_handle, entire_file, w32_file_found.nFileSizeLow, &bytes_read, &overlapped);

			if(!bytes_read)
			{
				gprintf("ERROR READING FILE \"%s\" !", path_buffer);
				break;
			}

			int bytes_written = 0;
			overlapped.Offset     = write_offset; 
			//put the file size
			WriteFile(pack_handle, &w32_file_found.nFileSizeLow, sizeof(int), &bytes_written, &overlapped);
			write_offset += sizeof(unsigned int);

			//write the entire file
			overlapped.Offset = write_offset; 
			//put the file size
			WriteFile(pack_handle, entire_file, w32_file_found.nFileSizeLow, &bytes_written, &overlapped);
			write_offset += w32_file_found.nFileSizeLow;


			win32_close_file(wav_file_handle);
		}
		else
		{
			gprintf("COULD NOT OPEN FILE %s", path_buffer);
			break;
		}


		gprintf("FOUND FILE %s", w32_file_found.cFileName);
		found = FindNextFileA(find_handle, &w32_file_found);
	}

	if(find_handle)
	{
		FindClose(find_handle);


		gprintf("PACK COMPLETED!");
	}

/*
   Files were saved in the order:
   explosion.wav
   fastinvader1.wav
   fastinvader2.wav
   fastinvader3.wav
   fastinvader4.wav
   invaderkilled.wav
   shoot.wav
   ufo_highpitch.wav
   ufo_lowpitch.wav

   check invaders.c for invaders_audios
   */

	win32_close_file(pack_handle);
}


//Returns the IAudioClient interface, it allows me to get everything I need for sound.
static IAudioClient *
wasapi_init(int samples_per_sec, int channels)
{
	//Get a device enumerator by calling CoCreateInstance
	IMMDeviceEnumerator *imme = 0;
	HRESULT re = S_OK;

	const CLSID clid_mmd = CLSID_MMDeviceEnumerator;
	const IID iid_mmd = IID_IMMDeviceEnumerator;

	re = CoCreateInstance(
			&clid_mmd,
			0,
			CLSCTX_ALL, //this thing isn't on the CLSTX page...
			&iid_mmd,
			&imme);

	if(re != S_OK)
	{
		return(0);
	}

	//Represents an audio endpoint device.
	IMMDevice *imm = 0;

	//Get the default audio device.
	re = imme->lpVtbl->GetDefaultAudioEndpoint(
			imme,
			eRender,
			eConsole,
			&imm
			);

	if(re != S_OK)
	{
		return(0);
	}
#if 1
		IAudioClient *audio_client = 0;
		const IID idiac = IID_IAudioClient;
		//create an audio stream between the program and the endpoint device
		HRESULT res = imm->lpVtbl->Activate(
				imm,
				&idiac,
				CLSCTX_ALL, 
				0, //"Set to null for an IAudioClient
				&audio_client
				);
#endif

	if(re != S_OK)
	{
		return(0);
	}

    WAVEFORMATEX wavefs = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = channels,
		.nSamplesPerSec = samples_per_sec,

		.nAvgBytesPerSec = samples_per_sec * channels * sizeof(short),
		.wBitsPerSample = 16, //bit depth
		//ignored for PCM
		.cbSize = 0,

	};
/*		"If wFormatTag is WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE, nBlockAlign must be equal to 
		the product of nChannels and wBitsPerSample divided by 8 (bits per byte)."*/
	wavefs.nBlockAlign = channels * (wavefs.wBitsPerSample / 8);
		
	REFERENCE_TIME buffer_duration = 0;
	REFERENCE_TIME buffer_durationm = 0;
	audio_client->lpVtbl->GetDevicePeriod(
			audio_client, &buffer_duration, &buffer_durationm);

//	buffer_duration = 10000000;
	buffer_duration = 500000;// + 91875;


	re = audio_client->lpVtbl->Initialize(
			audio_client,
			AUDCLNT_SHAREMODE_SHARED, //share the device
			AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
			buffer_duration,
			0, //always 0 on shared mode
			&wavefs,
			0
			);

	if(re != S_OK)
	{
		return(0);
	}

	//turn on the sound.
	re = audio_client->lpVtbl->Start(audio_client);

	if(re != S_OK)
	{
		return(0);
	}

	return(audio_client);
}


static unsigned long long 
win32_get_performance_frequency()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

static unsigned long long 
win32_get_performance_counter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

inline float 
win32_get_ms_elapsed(unsigned long long start, unsigned long long end)
{
    return (float)(end - start) / (float)global_performance_frequency;
}

static void *
win32_create_file(char *path_and_name)
{
    void *result = 0;

	DWORD dw_desired_access = GENERIC_READ | GENERIC_WRITE;
	DWORD dw_share_mode = 0;
	DWORD dw_creation_disposition = CREATE_ALWAYS;


    HANDLE handle = CreateFile(path_and_name,
                               dw_desired_access, 
                               dw_share_mode,
                               0,
							   dw_creation_disposition,
                               0,
                               0);

	DWORD error = GetLastError();

    if(handle != INVALID_HANDLE_VALUE)
    {
         result = handle;
    }
    return(result);
}

static void *
win32_open_file(char *path_and_name)
{
    void *result = 0;

	DWORD dw_desired_access = GENERIC_READ;
	DWORD dw_share_mode = FILE_SHARE_READ;
	DWORD dw_creation_disposition = OPEN_EXISTING;


    HANDLE handle = CreateFile(path_and_name,
                               dw_desired_access, 
                               dw_share_mode,
                               0, //Note(Agu): For other aplications to use
							   dw_creation_disposition,
                               0,
                               0);

	//ACA ESTA LA POSIBILIDAD DE ESPECIFICAR EL FALLO AL ABRIR EL ARCHIVO, PERO
	//COMO SIMPLEMENTE USO UN DEBUGGER, NO ME IMPORTA DE MOMENTO.
	DWORD error = GetLastError();
	//platform_open_file_result open_result = 0;
    if(handle != INVALID_HANDLE_VALUE)
    {
         result = handle;
		 if(error == ERROR_FILE_EXISTS)
		 {
			 //open_result = open_file_result_overwrote_existing;
		 }
    }
    return(result);
}

static int
win32_close_file(void *file_handle)
{
    int success = CloseHandle(file_handle);
	return(success);
}

static void *
win32_alloc(unsigned long long size)
{
    void *memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	return(memory);
}

static 
unsigned long long win32_get_file_size(void *handle)
{
	BY_HANDLE_FILE_INFORMATION w32_file_information = {0};
	//fill info.
	GetFileInformationByHandle(handle, &w32_file_information);
	//get last write struct.
	unsigned long long result = ((unsigned long long)w32_file_information.nFileSizeHigh << 32) | (w32_file_information.nFileSizeLow); 

	return(result);

}

static unsigned int win32_read_from_file(unsigned long long offset, unsigned long long size, void *dst, void *handle)
{
    unsigned int bytes_read = 0;

    OVERLAPPED overlapped = {0};
    overlapped.Offset     = (unsigned int)(offset & 0xffffffff);
    overlapped.OffsetHigh = (unsigned int)((offset >> 32) & 0xffffffff);

    ReadFile(handle, dst, (unsigned int)size, &bytes_read, &overlapped);
    
    return(bytes_read);
}


static void
win32_read_msgs(MSG *msgs,
		HWND hwnd,
		program_emu *program,
		unsigned int *game_is_running)
{
     while(PeekMessage(msgs, 0, 0, 0, PM_REMOVE))
	 {

		 switch(msgs->message)
		 {
			 case WM_SYSKEYDOWN:
			 case WM_SYSKEYUP:
			 case WM_KEYDOWN:
			 case WM_KEYUP:
				 {
					 WPARAM key_code = msgs->wParam;

					 int is_down  = (msgs->lParam & (1UL << 31)) == 0;
					 int was_down = (msgs->lParam & (1UL << 30)) != 0;

					 if(is_down != was_down)
					 {
						 //switch color
						 if(key_code == VK_F1)
						 {
							 if(is_down && !f1_was_down)
							 {
								 global_color_on = !global_color_on;
							 }
							 f1_was_down = was_down;
						 }
						 f1_was_down = 0;

						 if(key_code < g_win32_kc_max)
						 {
							 invaders_input(g_win32_kc[key_code], is_down, was_down, program->user);
						 }
					 }
				 }break;

				 //LET THE OS HANDLE THE REST
			 default:
				 {
					 TranslateMessage(msgs);
					 DispatchMessage(msgs);
				 }break;
		 }
	 }
}


LRESULT CALLBACK win32_process_msg(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch(umsg)
    {
		case WM_DEVICECHANGE:
			{
				//DETECT IF THE PRIMARY AUDIO DEVICE GOT DISCONNECTED
#if 0
				if(wparam == DBT_DEVNODES_CHANGED) //0x07
				{
					//Check the audio device
					GUID audio_device = dsound_get_primary_audio_device();
					b32 device_changed = !win32_guid_equals(audio_device, global_primary_sound_device);

					//STOP THE BUFFER IF DISCONNECTED
					if(device_changed)
					{
						IDirectSoundBuffer8_Stop(global_ds);

						if(win32_guid_zero(audio_device))
						{
							global_audio_device_disconnected = 1;
						}
						else
						{
							//RESUME PLAYING
							IDirectSoundBuffer8_Play(global_ds,
									0, 0, DSBPLAY_LOOPING);
						}
					}
					else
					{
						global_audio_device_disconnected = 0;
						//RESUME PLAYING
						IDirectSoundBuffer8_Play(global_ds,
								0, 0, DSBPLAY_LOOPING);
					}
	//				DirectSoundEnumerate(dsound_enumerate_callback, &device_changed);
					
				}
#endif
			}break;

        case WM_DESTROY:
            {
                PostQuitMessage(0);
            }break;
        case WM_CLOSE:
            {
                DestroyWindow(hwnd);
            }break;
			//update the scale and the
			//client values.
        case WM_SIZE:
            {
				RECT window_r = {0};
				RECT client_size = {0};

				GetClientRect(hwnd, &client_size);

				if(client_size.right * client_size.bottom)
				{
					global_win32_client_w = client_size.right - client_size.left;
					global_win32_client_h = client_size.bottom - client_size.top;


				}

				int adjusted_w = global_win32_client_w;
				int adjusted_h = global_win32_client_h;

				if(global_win32_client_w < INV_SCREENW)
				{
					adjusted_w = INV_SCREENW;
				}
				if(global_win32_client_h < INV_SCREENH)
				{
					adjusted_h = INV_SCREENH;
				}


				global_window_scale = get_area_scale(adjusted_w, adjusted_h, INV_SCREENW, INV_SCREENH);

            }break;
			//set the minimum window size
		case WM_GETMINMAXINFO :
			{
				MINMAXINFO *nsz = ((MINMAXINFO  *)lparam);
				nsz->ptMinTrackSize.x = INV_SCREENW;
				nsz->ptMinTrackSize.y = INV_SCREENH;
			}break;
        default:
            {
              return DefWindowProc(hwnd, umsg, wparam, lparam);
            }
    }
    return 1;
}


static void
win32_software_display(platform_renderer *renderer)
{
	software_renderer *swr = (software_renderer *)renderer;

	int c0 = 0;
	int c1 = 0;
	int c2 = INV_SCREENW;
	int c3 = INV_SCREENH;

	//aspect ratio
	int gw = INV_SCREENW;
	int gh = INV_SCREENH;
	int scale =  get_area_scale(global_win32_client_w, global_win32_client_h, gw, gh);
	int v0 = 0;
	int v1 = 0;
	int v2 = gw * scale;
	int v3 = gh * scale;

	if(global_win32_client_w > gw)
	{
		int difx = global_win32_client_w - v2;
		v0 = difx / 2;
	}
	if(global_win32_client_h > gh)
	{
		int dify = global_win32_client_h - v3;
		v1 = dify / 2;
	}



	BITMAPINFO bm_info = {0};
	BITMAPINFOHEADER *bm_header = &bm_info.bmiHeader;

	bm_header->biSize          = sizeof(bm_info.bmiHeader);
	bm_header->biWidth         = INV_SCREENW;
	//if not negative, screen will display upside down
	bm_header->biHeight        = -INV_SCREENH;
	bm_header->biPlanes        = 1; //"This value must be set to 1."
	bm_header->biBitCount      = 32; //bits per pixel
	bm_header->biCompression   = BI_RGB;
	bm_header->biSizeImage     = 0; //"This can be 0 for uncompressed bitmaps"
	bm_header->biXPelsPerMeter = 0;
	bm_header->biYPelsPerMeter = 0;

	int win_w = global_win32_client_w;
	int win_h = global_win32_client_h;

	HDC hdc = swr->device;
	//clear borders.
	//sides
	PatBlt(hdc, 0, 0, v0, win_w, BLACKNESS);
	PatBlt(hdc, v0 + v2, 0, v2, win_h, BLACKNESS);
	//top and bottom
	PatBlt(hdc, 0, 0, win_w, v1, BLACKNESS);
	PatBlt(hdc, 0, v1 + v3, win_h, v3, BLACKNESS);
	SetBkColor(hdc, RGB(0, 0, 0));

	//copies source to a destination. The destination being the device (hdc).
	StretchDIBits(hdc,
			//screen
			v0,
			v1,
			v2,
			v3,
			//pixels
			c0,
			c1,
			c2,
			c3,
			swr->pixels,
			&bm_info,
			DIB_RGB_COLORS,
			/*
			   This define how the color data for the source rectangle is to be combined with
			   the color data for the destination rectangle to achieve the final color.
			   SRCCOPY just copies the pixels of the source to the destination
			*/
			SRCCOPY);

}




//get the area minimum scale value
static int
get_area_scale(int wind_w, int wind_h, int gamew, int gameh)
{
	int game_w = gamew;
	int game_h = gameh;

	int scale_w = wind_w / game_w;
	int scale_h = wind_h / game_h;

	int out = scale_w < scale_h ? scale_w : scale_h;
	//return at least 1
	out += out == 0;

	return(out);
}

static int
get_adjusted_window_scale()
{

	//Get the work area. Counts everything except the bars.
	RECT crect  = {0};
	SystemParametersInfoA(
			SPI_GETWORKAREA,
			0,
			&crect,
			0);

	int window_scale = get_area_scale(crect.right, crect.bottom, INV_SCREENW, INV_SCREENH);

	return(window_scale);
}

//compares strings until a null terminated character is found or both are equal.
static int
read_arg(char *arg, char *arg_to_cmp)
{

    int result = *arg == *arg_to_cmp;
	//These values make sure they both end at the same time.
	int at_end0 = 0;
	int at_end1 = 0;
    while(result && !at_end0 && !at_end1)
    {
        at_end0 = *arg == '\0';
		at_end1 = *arg_to_cmp == '\0';
        result &= (*arg++ == *arg_to_cmp++);
    }
    return(result);

}

static void
win32_consolew(int output_count, char *output)
{
	WriteConsole(g_console_handle,
			output,
			output_count,
			0,
			0);
	OutputDebugString(output);
}





//because CRT
#pragma function(memset)
void* __cdecl memset(void* _Dst,int _Val, size_t _Size)
{
    unsigned char *at = (unsigned char *)_Dst;
    unsigned char  v = *(unsigned char *)&_Val;
    while(_Size--) 
    {
        *at++ = v;
    }
    return _Dst;

}

int WinMainCRTStartup()
{

	char *cmd_line = GetCommandLineA();
    int result = WinMain(GetModuleHandle(0), 0, cmd_line, 0);
    ExitProcess(result);
}


int WINAPI WinMain(HINSTANCE hinstance,
                   HINSTANCE hprevinstance,
                   LPSTR pCmdLine,
                   int nCmdShow)
{

#if 0
	pack_sound_file();
	return(0);
#endif

	int argc = 0;
	int args_i = 0;
	int expected_args = 1;
	char *cmd_line = GetCommandLineA();

	{
		//read arguments information


		char *cmd_at = cmd_line;

		//skip arg
		while(*cmd_at != '\0' && argc < expected_args)
		{
			//space found, skip spaces
			if(*cmd_at == ' ')
			{
				cmd_at++;
				//skip spaces
				while(*cmd_at == ' ' && *cmd_at != '\0')
					cmd_at++;

				//found next arg
				if(*cmd_at != '\0')
				{
					args_i = (int)(cmd_at - cmd_line);
					argc++;
				}
			}
			cmd_at++;
		}
	}
	int run_tests = argc && read_arg(cmd_line + args_i, "tests");

	int success = AttachConsole(ATTACH_PARENT_PROCESS);
	if(success)
	{
		g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	HRESULT granular_time = timeBeginPeriod(1);

	const char window_name[] = "e8080";

	if(g_console_handle)
	{
	}

	////////////////WINDOW//////////////////

	WNDCLASS wndc = {0};
	//callback function for handling messages
	wndc.lpfnWndProc   = win32_process_msg;
	wndc.hInstance     = hinstance;
	wndc.lpszClassName = window_name;

	RegisterClass(&wndc);


	global_window_scale = 1;
	RECT crect = {0, 0, (LONG)(INV_SCREENW * global_window_scale), (LONG)(INV_SCREENH * global_window_scale)};
	int window_style = WS_OVERLAPPEDWINDOW;

	AdjustWindowRect(&crect, window_style, 0);

    HWND hwnd = CreateWindow(
			window_name,
			"e8080_",
			window_style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			crect.right - crect.left,
			crect.bottom - crect.top,
			0,
			0,
			hinstance,
			0);


	if(!run_tests)
		ShowWindow(hwnd, SW_SHOW);

	global_window_scale = get_area_scale(global_win32_client_w, global_win32_client_h, INV_SCREENW, INV_SCREENH);

	/////////////////////////////////////////

	////////////////AUDIO////////////////////

	int audio_samples_per_sec = 44100;
	int audio_channels = 1;
	IAudioClient *audioc = wasapi_init(audio_samples_per_sec, audio_channels);
	IAudioRenderClient *arenderc = 0;

	if(!audioc)
	{
		if(!run_tests)
			gprintf("FAILED TO INITIALIZE AUDIO");
	}
	else
	{
		audioc->lpVtbl->GetService(audioc, &IID_IAudioRenderClient, &arenderc);
	}

	/////////////////////////////////////////

	//////////////////RENDERER///////////////

	int render_api = render_api_d3d11;

	platform_renderer *renderer;
	static void (*f_renderer_draw)(platform_renderer *renderer, int game_w, int game_h, unsigned char *game_pixels) = renderer_draw_zero;
	static void (*f_renderer_swap_buffers)(platform_renderer *renderer) = renderer_swap_buffers_zero;

	if(render_api == render_api_d3d11)
	{
		renderer = (platform_renderer *)d3d11_init(hwnd);
		f_renderer_draw = d3d11_draw;
		f_renderer_swap_buffers = d3d11_swap_buffers;

		if(!renderer)
		{
			gprintf("FAILED TO INITIALIZE THE D3D11 RENDERER. SWITCHING TO SOFTWARE.");
			render_api = render_api_software;
		}
	}
	//software
	if(render_api == render_api_software)
	{
		//init_software_renderer


		//allocate the renderer and the pixels

		unsigned int pixels_size = INV_SCREENW * INV_SCREENH * 4;
		software_renderer *swr = win32_alloc(sizeof(software_renderer) + pixels_size);
		swr->device = GetDC(hwnd);
		swr->pixels = (unsigned char *)(swr + 1);

		memory_clear(pixels_size, swr->pixels);

		//f_renderer_draw = d3d11_draw;
		f_renderer_draw = software_renderer_draw;
		f_renderer_swap_buffers = win32_software_display;

		renderer = (platform_renderer *) swr;

		unsigned int *pixels_at = (unsigned int *)swr->pixels;

		for(int p = 0; p < 256 * 224; p++)
		{
			unsigned int x = p % 224;
			unsigned int y = p / 256;

			unsigned int pixel = x >= 127 ? 0xffffffff : 0;

			*pixels_at++ = pixel;

		}
	}

	if(!renderer)
	{
		ASSERT(0);
	}

	renderer->rec_x0 = 0;
	renderer->rec_y0 = 0;
	renderer->rec_x1 = INV_SCREENW;
	renderer->rec_y1 = INV_SCREENH;


	//////////////////////////////////
	global_performance_frequency = win32_get_performance_frequency();

    unsigned long long last_performance_counter = win32_get_performance_counter();
    unsigned long long last_interrupt_t = win32_get_performance_counter();

    unsigned long long time_end = win32_get_performance_counter();
    unsigned long long time_lst = win32_get_performance_counter();
	unsigned long long ms = 0;

	float target_ms = 1.0f / 30.0f;
	float delta_time = target_ms;


	////////////////GAME//////////////////

	struct program_emu program = {0};

	program.proc_interrupts = 1;
	program.next_interrupt = 0xcf;
	//bit 2 is always on
	program.af.ls = 0x02;



	unsigned char game_pixels[256 * 224] = {0};
	static unsigned int vram_addr = 0x2400;

	//////////////////////////////////


	int running = 1;


	static int invaders_clock = 2000000;
	static int invaders_vblank = 60;
	unsigned int vblank_t = (invaders_clock / invaders_vblank);


	//RUN TESTS
	if(run_tests)
	{
		gprintf("RUNNING TESTS");
		global_debug_output();

		program.proc_interrupts = 0;
		while(running)
		{

			running = run_test(&program);

			if(g_print_buffer_used)
			{
				global_debug_output();
			}
		}
	}

	while(running)
	{

		//read message events from windows
		MSG msgs = {0};
		win32_read_msgs(&msgs,
				hwnd,
				&program,
				&running);

		//stop game
		if(msgs.message == WM_QUIT)
		{
			running = 0;
			break;
		}

#if 1 //GAME

		//////////////////////UPDATE AND RENDER/////////////////////

		while(running)
		{
			running = game(&program);

			if(program.cycles >= (vblank_t / 2))
			{
				program.proc_interrupts = 1;
				program.interrupt_requested = program.next_interrupt;
				program.next_interrupt = 0xd7;


				break;
			}
		}
		while(running)
		{
			running = game(&program);

			if(program.cycles >= vblank_t)
			{
				program.cycles = 0;

				program.proc_interrupts = 1;

				program.interrupt_requested = program.next_interrupt;
				program.next_interrupt = 0xcf;

				{
					/*
					   Actualizar VRAM

					   Los pixeles estan divididos por bits en las direcciones 2400 - 3fff.
					   Hay que dividir los bytes por bits, donde cada bit con valor 1 representa
					   un pixel encendido.

					   Restados 0x4000 - 0x2400 dan como resultado 7168, multiplicado por 8 = 57344
					   osea 256 * 224

					   Overlay dimensions (screen rotated 90 degrees anti-clockwise):
					   ,_______________________________.
					   |WHITE            ^             |
					   |                32             |
					   |                 v             |
					   |-------------------------------|
					   |RED              ^             |
					   |                32             |
					   |                 v             |
					   |-------------------------------|
					   |WHITE                          |
					   |         < 224 >               |
					   |                               |
					   |                 ^             |
					   |                120            |
					   |                 v             |
					   |                               |
					   |                               |
					   |                               |
					   |-------------------------------|
					   |GREEN                          |
					   | ^                  ^          |
					   |56        ^        56          |
					   | v       72         v          |
					   |____      v      ______________|
					   |  ^  |          | ^            |
					   |<16> |  < 102 > |16   < 122 >  |
					   |  v  |          | v            |
					   |WHITE|          |         WHITE|
					   `-------------------------------'
					   */
					{
						int pixel_i = vram_addr;

						int xt = 0;
						int yt = 0;
						//7168
						for(int i = 0; i < (256 * 224) / 8; i++)
						{
							int x = (i * 8) % 256;
							int y = i * 8 / 256;


							unsigned char sp_pixel = *((unsigned char *)(program.memory) + 0x2400 + i);

							for(int b = 0; b < 8; b++)
							{
								unsigned char sp_bit = (sp_pixel >> b) & 1;

								int xp = -(x + b) + 256 - 1;
								int j = y + xp * 224;
								/*
								   r = 0x01;
								   g = 0x02;
								   b = 0x04;

								   to avoid confusion:
								   xp is the y coordinate of the rotated screen,
								   and y is the x coordinate.
								*/

								unsigned char rgb = 0;

								if(sp_bit)
								{
									rgb = 0x07;

									if(global_color_on)
									{
										if(y >= 16 && y <= 102
												&& xp >= 240 && xp < 256)
										{
											rgb &= 0x02;
										}

										if(xp >= 184 && xp < 240)
										{
											rgb &= 0x02;
										}

										if(xp > 32 && xp < 64)
										{
											rgb &= 0x01;
										}
									}
								}

								ASSERT(j < (0x4000 * 8));
								game_pixels[j] = rgb;
							}
						}

						ASSERT(pixel_i <= 0x4000);
						int i0 = 0;

					}
				}
				break;
			}
		}

		/////////////////////DEBUG OUTPUT//////////////

		if(g_print_buffer_used)
		{
			global_debug_output();
		}
		else
		{
		}

		///////////////////////////////////////////////

		////////////////AUDIO//////////////////////////
		{
			invaders_s *invaders = program.user;
			if(audioc && invaders->audio_avadible)
			{
				unsigned int padding = 0;
				audioc->lpVtbl->GetCurrentPadding(audioc, &padding);

				unsigned int endpoint_buffer_size = 0;
				audioc->lpVtbl->GetBufferSize(audioc, &endpoint_buffer_size);

				unsigned int bytes_to_write = (endpoint_buffer_size - padding);

				unsigned char *endpoint_buffer = 0;
				HRESULT re = arenderc->lpVtbl->GetBuffer(arenderc, bytes_to_write, &endpoint_buffer);
				ASSERT(re == S_OK);


				short *endpoint_buffer16 = (short *)endpoint_buffer;
				short *game_audio_buffer = (short *)invaders->audio_buffer;

				//Not possible
				ASSERT(bytes_to_write < GAME_AUDIO_BUFFER_SIZE);
				//only clear the bytes that I'm going to send.
				memory_clear(bytes_to_write * sizeof(short), game_audio_buffer);


				unsigned int written_bytes = 0;
				//get game sound samples
				{
					invaders_s *invaders = program.user;

					short *game_audio_samples = (short *)(invaders->audio_slots_buffer);

					for(int a = 0; a < INVADERS_AUDIO_COUNT; a++)
					{
						written_bytes = bytes_to_write;
						audio_slot *as = invaders->audio_slots + a;
						as->delay -= as->delay != 0;

						if(as->run)
						{


							/*
							   If loop, always fill the whole buffer
							   */

							unsigned int samples_sent = 0;
							//I have to read why this was necessary.
							float sp = 0.25f;
							float volume = 0.5f;

							while(1)
							{
								unsigned int avadible_samples_to_send = (bytes_to_write / audio_channels) - samples_sent;
								unsigned int as_remaining_samples = (unsigned int)( (as->sample_count - (int)as->sample_index) / sp ) ;

								unsigned int samples_to_send = avadible_samples_to_send;

								if(samples_to_send > as_remaining_samples)
								{
									samples_to_send = as_remaining_samples;
								}
#if 1
								//Send the samples
								for(unsigned int s = 0; s < samples_to_send; s++)
								{
									unsigned char aval = *( (char *)(game_audio_samples) + as->offset_to_audio_data + (unsigned int )(as->sample_index + (float)s * sp));


									short avalf = (aval << 8);
									avalf -= 32768;

									float fv0 = *game_audio_buffer;

									fv0 += ((float)avalf * volume);

									int mm = 32000;
									fv0 = fv0 < -mm ? -mm : fv0 > mm ? mm : fv0;
									//aval = 10000;

									*game_audio_buffer++ = (short)(fv0 + .5f);

								}

#else

								//send square
								static float sing = 0;
								float pi = 3.14159265358f;
								for(unsigned int b = 0; b < samples_to_send; b++)
								{
									//		sing += pi * 2 * 256 / audio_samples_per_sec;

									sing++;
									short aval = 10000;
									int val = ((int)sing / (audio_samples_per_sec / 256 / 4)) % 2;
									if(val)
									{
										aval = -10000;
									}

									*game_audio_buffer++ = aval;
									*game_audio_buffer++ = aval;
								}
#endif


								//advance through the sound effect
								samples_sent += samples_to_send;
								as->sample_index += (samples_to_send * sp);



								if(as->loop && as->sample_index >= as->sample_count)
								{
									as->sample_index = 0;

									//Don't send 0 samples.
									if(samples_sent >= avadible_samples_to_send)
									{
										break;
									}

									continue;
								}
								else if(as->sample_index >= as->sample_count)
								{
									if(!as->loop)
									{
										as->run = 0;
									}
									as->sample_index = 0;
								}

								break;
							}
							ASSERT(samples_sent < GAME_AUDIO_BUFFER_SIZE);
//							gprintf("Samples sent: {%u}", samples_sent);

						}
						game_audio_buffer = (short *)invaders->audio_buffer;
					}

				}

				if(endpoint_buffer)
				{


					endpoint_buffer16 = (short *)endpoint_buffer;
					game_audio_buffer = (short *)invaders->audio_buffer;

					////////////////////////////////
					for(unsigned int b = 0; b < bytes_to_write; b++)
					{
						short v0= 	*game_audio_buffer++;

						*endpoint_buffer16++ = v0;

					}

					re = arenderc->lpVtbl->ReleaseBuffer(arenderc, written_bytes, 0);
				}

				ASSERT(re == S_OK);
			}
		}

		///////////////////////////////////////////////
	
		/////////////////////////////////////

		/*
		   Adjust the scale dimensions
		   */
		if(render_api != render_api_software)
		{

			renderer->rec_x0 = 0;
			renderer->rec_y0 = 0;
			renderer->rec_x1 = (float)global_win32_client_w;
			renderer->rec_y1 = (float)global_win32_client_h;

			//game scaled with specified scale
			float game_scaled_w = (float)INV_SCREENW * global_window_scale;
			float game_scaled_h = (float)INV_SCREENH * global_window_scale;
			//multiplier to scale the viewport with the window
			//scale with window if using stretching
			float window_w = (float)global_win32_client_w;
			float window_h = (float)global_win32_client_h;

			renderer->rec_x1 = (float)(int)game_scaled_w;
			renderer->rec_y1 = (float)(int)game_scaled_h;

			float scale_w  = game_scaled_w / window_w;
			float scale_h  = game_scaled_h / window_h;
			if(scale_w < 1.0f)
			{
				float scaled_x = (window_w - game_scaled_w) / 2;
				renderer->rec_x0 = (float)(scaled_x);
				renderer->rec_x1 = (float)(game_scaled_w + scaled_x);
			}
			if(scale_h < 1.0f)
			{
				float scaled_y = (window_h - game_scaled_h) / 2;
				renderer->rec_y0 = (float)(int)(scaled_y);
				renderer->rec_y1 = (float)(int)(game_scaled_h + scaled_y);
			}
		}


		f_renderer_draw(renderer, INV_SCREENW, INV_SCREENH, game_pixels);
		/////////////////////////////////////
#else

		//RUN TESTS

		program.proc_interrupts = 0;
		while(running)
			running = run_test(&program);
		break;
#endif

		//sleep
		float elapsed_ms = win32_get_ms_elapsed(time_lst, time_end);
		if(elapsed_ms < target_ms)
		{

			if(granular_time == TIMERR_NOERROR)
			{
				float ms_difference = (1000 * (target_ms - elapsed_ms));
				//only accepts sleep time as seconds
				int sleep_time  = (int)ms_difference;

				if(sleep_time > 0)
				{
					Sleep(sleep_time);
				}
			}

			//spin lock remaining frames
			while(elapsed_ms < target_ms)
			{
				elapsed_ms = win32_get_ms_elapsed(time_lst, win32_get_performance_counter());
			}
		}

		f_renderer_swap_buffers(renderer);

		ms = win32_get_performance_counter() - time_lst;
		time_lst = time_end;
		time_end = win32_get_performance_counter();
		delta_time = (float)ms / global_performance_frequency;

		last_interrupt_t = win32_get_performance_counter();
	}

	if(!g_console_handle)
	{
		if(g_error == error_file_not_found)
		{
			MessageBox(
					hwnd,
					"The program expects an \"invaders.rom\" file. Instructions are on the readme.",
					"Error",
					MB_OK
					);
		}
		else if(g_error == error_invalid_file_size)
		{
			MessageBox(
					hwnd,
					"The \"invaders.rom\" file is invalid.",
					"Error",
					MB_OK
					);
		}
	}


	FreeConsole();
	return(0);
}
