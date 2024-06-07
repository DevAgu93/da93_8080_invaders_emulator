typedef struct platform_renderer{
	void *data;

	void (*f_draw_start)(struct platform_renderer);
	void (*f_draw_end)(struct platform_renderer);
	void (*f_swap_buffers)(struct platform_renderer);

	float rec_x0;
	float rec_y0;
	float rec_x1;
	float rec_y1;

}platform_renderer;

typedef struct{
	platform_renderer header;

	unsigned char *pixels;
	void *device;
}software_renderer;

enum{
	render_api_software,
	render_api_d3d11,
}render_api;

static void 
renderer_draw_zero(platform_renderer *renderer, int game_w, int game_h, unsigned char *game_pixels)
{
}

static void 
renderer_swap_buffers_zero(platform_renderer *renderer)
{
}

static inline float
f32_round(float val)
{
	//extract sign and activate the first bit
	int val_s    = (*((int *)&val) >> 31) | 1;
	//round to integer
	int val_i    = (int)val;
	//extract decimals, add 0.5 multiplied by the sign to them, and check if the first bit is active
	float decimals = (float) ((int)(val - val_i + (0.5f * val_s)) & 1);
	float result   = val_i + decimals * val_s;
	return(result);
}

static void 
software_renderer_draw(platform_renderer *renderer, int game_w, int game_h, unsigned char *game_pixels)
{
	software_renderer *swr = ( software_renderer *)renderer;
	unsigned char *pixels_at = swr->pixels;

	unsigned int w = game_w;
	unsigned int h = game_h;

	for(unsigned int y = 0; y < h; y++)
	{
		for(unsigned int x = 0; x < w; x ++)
		{
			int i = x + y * w;

			unsigned char pix = game_pixels[i];
			unsigned int r = (pix & 0x01) ? 255 : 0;
			unsigned int g = (pix & 0x02) ? 255 : 0;
			unsigned int b = (pix & 0x04) ? 255 : 0;

			//pixels_at are in ABGR order
			((unsigned int *)pixels_at)[x] = 0xff000000 | (b << 16) | (g << 8) | (r); 

		}

		pixels_at += 4*w;
	}
}
