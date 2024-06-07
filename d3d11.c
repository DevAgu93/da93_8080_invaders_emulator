typedef struct{

	float pos[3];
	float uvs[2];

}render_vertex;


#include <d3d11_1.h>


//#include <d3dcompiler.h>
#define D3DCOMPILE_DEBUG                                (1 << 0)
#define D3DCOMPILE_SKIP_VALIDATION                      (1 << 1)
#define D3DCOMPILE_SKIP_OPTIMIZATION                    (1 << 2)
#define D3DCOMPILE_PACK_MATRIX_ROW_MAJOR                (1 << 3)
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR             (1 << 4)
#define D3DCOMPILE_PARTIAL_PRECISION                    (1 << 5)
#define D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT             (1 << 6)
#define D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT             (1 << 7)
#define D3DCOMPILE_NO_PRESHADER                         (1 << 8)
#define D3DCOMPILE_AVOID_FLOW_CONTROL                   (1 << 9)
#define D3DCOMPILE_PREFER_FLOW_CONTROL                  (1 << 10)
#define D3DCOMPILE_ENABLE_STRICTNESS                    (1 << 11)
#define D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY       (1 << 12)
#define D3DCOMPILE_IEEE_STRICTNESS                      (1 << 13)
#define D3DCOMPILE_OPTIMIZATION_LEVEL0                  (1 << 14)
#define D3DCOMPILE_OPTIMIZATION_LEVEL1                  0
#define D3DCOMPILE_OPTIMIZATION_LEVEL2                  ((1 << 14) | (1 << 15))
#define D3DCOMPILE_OPTIMIZATION_LEVEL3                  (1 << 15)
#define D3DCOMPILE_RESERVED16                           (1 << 16)
#define D3DCOMPILE_RESERVED17                           (1 << 17)
#define D3DCOMPILE_WARNINGS_ARE_ERRORS                  (1 << 18)
#define D3DCOMPILE_RESOURCES_MAY_ALIAS                  (1 << 19)
#define D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES   (1 << 20)
#define D3DCOMPILE_ALL_RESOURCES_BOUND                  (1 << 21)
#define D3DCOMPILE_DEBUG_NAME_FOR_SOURCE                (1 << 22)
#define D3DCOMPILE_DEBUG_NAME_FOR_BINARY                (1 << 23)


HRESULT WINAPI
D3DCompile(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
           _In_ SIZE_T SrcDataSize,
           _In_opt_ LPCSTR pSourceName,
           _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines,
           _In_opt_ ID3DInclude* pInclude,
           _In_opt_ LPCSTR pEntrypoint,
           _In_ LPCSTR pTarget,
           _In_ UINT Flags1,
           _In_ UINT Flags2,
           _Out_ ID3DBlob** ppCode,
           _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorMsgs);

typedef struct{
    platform_renderer header;

    ID3D11Device        *device;
    ID3D11DeviceContext *device_context;
	//to present
    IDXGISwapChain1     *swap_chain; 

	//back buffer handle
    void *frame_buffer;
	unsigned short last_frame_buffer_index;
    unsigned short current_frame_buffer_index;

	//copy the game pixels here
	ID3D11Texture2D *canvas_texture;
	//send the pixels to the shader
	ID3D11ShaderResourceView *canvas_texture_view;
	//for MAP and UNMAP
    ID3D11Buffer *vertex_buffer;


}d3d11_device;

//I use this to read shader errors on the debugger
char*
sherr(ID3DBlob *shadercode)
{
    return (char *)shadercode->lpVtbl->GetBufferPointer(shadercode);
}

//return result
#define RR(func) if((func) != S_OK) return(0);
//create a texture that needs to be constantly updated.
static int
d3d11_create_canvas_texture(
		int width,
		int height,
		ID3D11Device *device,
		ID3D11DeviceContext *devicecontext,
		ID3D11Texture2D **out_texture,
		ID3D11ShaderResourceView **canvas_texture_view
		)
{

   D3D11_SHADER_RESOURCE_VIEW_DESC shader_desc = {0};

   ID3D11Texture2D *created_texture;

   D3D11_TEXTURE2D_DESC texture_desc = {0};


   texture_desc.Width          = width;
   texture_desc.Height         = height;
   texture_desc.MipLevels      = 1;
   texture_desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
   //Usage is dynamic, so it can be frequently updated.
   texture_desc.Usage          = D3D11_USAGE_DYNAMIC;
   texture_desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
   texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   texture_desc.ArraySize      = 1;

   texture_desc.SampleDesc.Count = 1;
   texture_desc.SampleDesc.Quality = 0;

   shader_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   shader_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; //SRV = Shader Resource View
   shader_desc.Texture2D.MipLevels = -1; 
   shader_desc.Texture2D.MostDetailedMip = 0;

   RR( device->lpVtbl->CreateTexture2D(device, &texture_desc, 0, &created_texture) );

   RR( device->lpVtbl->CreateShaderResourceView(device,
		   (ID3D11Resource *)created_texture,
		   &shader_desc,
		   canvas_texture_view) );

   *out_texture = created_texture;

   return(1);
}


static d3d11_device * 
d3d11_init(
        HWND windowhand)
		

{

	unsigned int back_buffer_w = INV_SCREENW;
	unsigned int back_buffer_h = INV_SCREENH;

    d3d11_device *direct_device  = win32_alloc(sizeof(d3d11_device));

    IDXGISwapChain1     *swap_chain1;
    ID3D11Device        *device;
    ID3D11DeviceContext *device_context;

    D3D_FEATURE_LEVEL flevel = {D3D_FEATURE_LEVEL_11_0};
    HRESULT result = S_OK;
	result  = D3D11CreateDevice( 
                    NULL, 
                    //D3D_DRIVER_TYPE_REFERENCE, //Software
                    D3D_DRIVER_TYPE_HARDWARE,
                    NULL, //for software 
                    D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG, 
                    0, 
                    0, 
                    D3D11_SDK_VERSION, 
                    &device, 
                    &flevel, //Not applied
                    &device_context);

	if(result != S_OK)
	{
		return(0);
	}

	//using the base device and device context, create Device1 and DeviceContext1
	ID3D11Device1* device1;

    RR(device->lpVtbl->QueryInterface(device , &IID_ID3D11Device1, &device1));

    ID3D11DeviceContext1* device_context1;

	RR( device_context->lpVtbl->QueryInterface(device_context, &IID_ID3D11DeviceContext1, &device_context1) );

	///////////////////////////////////////////////////////////////////////////////////////////////

    IDXGIDevice1 *dxgi_device1;

    RR( device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice1, &dxgi_device1) );

    IDXGIAdapter* dxgi_adapter;

	RR( dxgi_device1->lpVtbl->GetAdapter(dxgi_device1, &dxgi_adapter) );

    IDXGIFactory2 *dxgi_factory;

	RR( dxgi_adapter->lpVtbl->GetParent(dxgi_adapter, &IID_IDXGIFactory2, &dxgi_factory) );

    ///////////////////////////////////////////////////////////////////////////////////////////////



    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1 = {0}; //Gives instructions to the input pipeline and swaps buffers
    swap_chain_desc1.AlphaMode        = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.SampleDesc.Count   = 1; //sampling
    swap_chain_desc1.SampleDesc.Quality = 0;
    swap_chain_desc1.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc1.Stereo             = FALSE;
    swap_chain_desc1.SampleDesc.Count   = 1;
    swap_chain_desc1.SampleDesc.Quality = 0;
    swap_chain_desc1.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;

	//for double buffering
    swap_chain_desc1.BufferCount = 2;
    swap_chain_desc1.Scaling     = DXGI_SCALING_NONE;
    swap_chain_desc1.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc1.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags       = 0;

	//get monitor resolution
    HDC hdc = GetDC(windowhand);
	int monitor_w = GetDeviceCaps(hdc, HORZRES);
	int monitor_h = GetDeviceCaps(hdc, VERTRES);

    swap_chain_desc1.Width  = monitor_w; //swap buffer width and height
    swap_chain_desc1.Height = monitor_h;

	RR( dxgi_factory->lpVtbl->CreateSwapChainForHwnd(
			dxgi_factory,
			(IUnknown*)device1,
			windowhand,
			&swap_chain_desc1,
			0,
			0,
			&swap_chain1
			) );

	//////////////////////////////////////////////////////////////

   direct_device->swap_chain        = swap_chain1; 
   direct_device->device           = device;
   direct_device->device_context   = device_context;

   /////////////////////////////////////////////////////////
   
   ID3D11RenderTargetView   *back_buffer;
   ID3D11Texture2D          *back_buffer_texture;

   swap_chain1->lpVtbl->GetBuffer(swap_chain1,
		   0, &IID_ID3D11Texture2D, &back_buffer_texture); //Default render target

   device->lpVtbl->CreateRenderTargetView(device,
		   (ID3D11Resource *)back_buffer_texture, 0, &back_buffer);

   //Se utiliza para mapearlo como recurso
   ID3D11ShaderResourceView *back_buffer_resource_view;
   D3D11_SHADER_RESOURCE_VIEW_DESC back_buffer_resource_desc = {0};

   back_buffer_resource_desc.Format = 0;
   back_buffer_resource_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
   back_buffer_resource_desc.Texture2D.MostDetailedMip = 0;
   back_buffer_resource_desc.Texture2D.MipLevels = 1;

   RR( device->lpVtbl->CreateShaderResourceView(
		   device,
		   (ID3D11Resource *)back_buffer_texture,
		   &back_buffer_resource_desc,
		   &back_buffer_resource_view) );

   back_buffer_texture->lpVtbl->Release(back_buffer_texture);
   //////////////////////////////////////////////////////////////


   /////////////////////////////////////////////////////////////////////////
   // DEPTHSTENCIL

   D3D11_DEPTH_STENCIL_DESC  depthdesc = {0};
   ID3D11DepthStencilState *depth_stencil_desc;
   depthdesc.DepthEnable    = 0;
   depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
   depthdesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;

   depthdesc.StencilEnable                = 0;
   depthdesc.StencilReadMask              = 0xff; //0xff is default.
   depthdesc.StencilWriteMask             = 0xff; //0xff is default.
   depthdesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
   depthdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
   depthdesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
   depthdesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

   depthdesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
   depthdesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;
   depthdesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
   depthdesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;

   RR( device->lpVtbl->CreateDepthStencilState(device, &depthdesc, &depth_stencil_desc) );
   device_context->lpVtbl->OMSetDepthStencilState(device_context, depth_stencil_desc ,1);

   ///////////////////////////////////////////////////////////////////
   //viewport
    
   D3D11_VIEWPORT vport = {0}; 
   vport.TopLeftX = 0;
   vport.TopLeftY = 0;
   vport.Width  = (float)back_buffer_w;
   vport.Height = (float)back_buffer_h;
   vport.MinDepth = 0;
   vport.MaxDepth = 1;

   device_context->lpVtbl->RSSetViewports(device_context, 1, &vport);
   ////////////////////////////////////////////////////////////


   ////////////////////////////////////////////////////////////
   //create vertex buffer

   ID3D11Buffer *vertex_buffer;
   D3D11_BUFFER_DESC vertex_buffer_desc      = {0};
   D3D11_SUBRESOURCE_DATA vertex_buffer_data = {0};

   //Only 6 vertices needed
   void *vertex_buffer_temp[sizeof(render_vertex) * 6] = {0};

   vertex_buffer_data.pSysMem        = vertex_buffer_temp; 
   vertex_buffer_desc.ByteWidth      = sizeof(vertex_buffer_temp);
   vertex_buffer_desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
   vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   vertex_buffer_desc.Usage          = D3D11_USAGE_DYNAMIC;

   RR( device->lpVtbl->CreateBuffer(device, &vertex_buffer_desc, &vertex_buffer_data, &vertex_buffer) );

   //Map vertices to buffer.
   UINT vertices_size = sizeof(render_vertex);
   static UINT render_offsets = 0;
   device_context->lpVtbl->IASetVertexBuffers(
		   device_context,
		   0,
		   1,
		   &vertex_buffer,
		   &vertices_size,
		   &render_offsets);

   //////////////////////////////////////////////////////////////////////////
   //filter and uv test

   ID3D11SamplerState *texture_sampler_state;
   D3D11_SAMPLER_DESC texture_sampler_desc = {0};

   texture_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
   texture_sampler_desc.MaxAnisotropy = 1;

   D3D11_TEXTURE_ADDRESS_MODE uv_adress_mode = D3D11_TEXTURE_ADDRESS_WRAP;
   texture_sampler_desc.AddressU = uv_adress_mode;
   texture_sampler_desc.AddressV = uv_adress_mode;
   texture_sampler_desc.AddressW = uv_adress_mode;
   texture_sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
   texture_sampler_desc.MipLODBias = 0;
   texture_sampler_desc.MinLOD = 0;
   texture_sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

   RR( device->lpVtbl->CreateSamplerState(device, &texture_sampler_desc, &texture_sampler_state) );

   device_context->lpVtbl->PSSetSamplers(
		   device_context,
		   0,
		   1,
		   &texture_sampler_state);

   ///////////////////////////////////////////////////////////////////////////////////


   ///////////////////////////////////////////////////////////////////////////////
   //Rasterizing 
   ID3D11RasterizerState *rasterizer_state;
   D3D11_RASTERIZER_DESC rasterizer_desc = {0};
   rasterizer_desc.FillMode = D3D11_FILL_SOLID; // or D3D11_FILL_WIREFRAME
   rasterizer_desc.CullMode = D3D11_CULL_BACK;

   //order of vertices
   rasterizer_desc.FrontCounterClockwise = 0;
   rasterizer_desc.FrontCounterClockwise = 0;
   rasterizer_desc.DepthBias             = 0;
   rasterizer_desc.DepthClipEnable       = 1;
   rasterizer_desc.ScissorEnable         = 0;
   rasterizer_desc.AntialiasedLineEnable = 1;
   rasterizer_desc.MultisampleEnable     = 0;
   RR( device->lpVtbl->CreateRasterizerState(device, &rasterizer_desc, &rasterizer_state) );
   device_context->lpVtbl->RSSetState(device_context, rasterizer_state);
   
   ///////////////////////////////////////////////////////////////////////////////


   ///////////////////////////////////////////////////////////////////////////////

   direct_device->vertex_buffer      = vertex_buffer;

   d3d11_create_canvas_texture(
		   back_buffer_w,
		   back_buffer_h,
		   device,
		   device_context,
		   &direct_device->canvas_texture,
		   &direct_device->canvas_texture_view);
   

   //set default display buffer
   direct_device->frame_buffer = back_buffer;
   /////////////////////////////////////////////////////

   device_context->lpVtbl->OMSetRenderTargets(device_context ,1, &back_buffer, 0);

   device_context->lpVtbl->IASetPrimitiveTopology(device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   //////////////////////////////////////////////////////
   //Initialize shaders
   {


	   //This macro converts code into a string.
#define SHADER_CODE(code) #code

	   char shader_code[] = SHADER_CODE(

			   //Struct build and send as vertex output
			   struct vt_out
			   {
			       float4 position : SV_POSITION;
			       float2 uvs : TEXCOORD;
			   };

			   vt_out vs_main(float3 position : POSITION,
				   float2 uvs : TEXCOORD
				   )

			   {
			       vt_out output;
			       //This sorts by column major by default.
			       output.position.xyz = position.xyz; 
			       output.position.w = 1;


			       output.uvs = uvs;

			       return output;
			   }

			   /////////PIXEL SHADER/////////

			   Texture2D canvas_texture;
			   SamplerState Sampler;

			   float4 ps_main(vt_out vout) : SV_TARGET
	           {

	               //SELECTED PIXEL SAMPLER IN MACRO
	               float2 uvi = vout.uvs;
	               float4 c = canvas_texture.Sample(Sampler, uvi);

	               return c;
	           }

			   /////////////////////////////
			   );



			   //////////////////////////////////////////////////////////////


			   ID3DBlob *vshader_blob;

			   ID3DBlob *pshader_blob;

			   ID3DBlob *error_msg;

			   //Compile vertex and pixel shaders
			   RR( D3DCompile(shader_code,
					   sizeof(shader_code),
					   0,
					   0, //defines
					   0,
					   "vs_main",
					   "vs_4_0",
					   D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
					   0,
					   &vshader_blob,
					   &error_msg) );

			   RR( D3DCompile(shader_code,
					   sizeof(shader_code),
					   0,
					   0,
					   0,
					   "ps_main",
					   "ps_4_0",
					   D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
					   0,
					   &pshader_blob,
					   &error_msg) );


			   ID3D11VertexShader *vs_default;


			   RR( direct_device->device->lpVtbl->CreateVertexShader(direct_device->device, 
					   vshader_blob->lpVtbl->GetBufferPointer(vshader_blob),
					   vshader_blob->lpVtbl->GetBufferSize(vshader_blob),
					   0,
					   &vs_default) );

			   //////////////////////////////////////////////////////////////

			   ID3D11PixelShader *ps_default;

			   RR( device->lpVtbl->CreatePixelShader(device,
					   pshader_blob->lpVtbl->GetBufferPointer(pshader_blob),
					   pshader_blob->lpVtbl->GetBufferSize(pshader_blob),
					   0, 
					   &ps_default) );



			   device_context->lpVtbl->VSSetShader(device_context, vs_default, 0, 0);
			   device_context->lpVtbl->PSSetShader(device_context, ps_default, 0, 0);

			   //set the input for using on the vertex shader
			   D3D11_INPUT_ELEMENT_DESC vt_out[] = 
			   {
				   {"POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				   {"TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			   };
			   ID3D11InputLayout *input_layout;

			   unsigned int input_count = sizeof(vt_out) / sizeof(D3D11_INPUT_ELEMENT_DESC);
			   result = direct_device->device->lpVtbl->CreateInputLayout(direct_device->device,
					   vt_out,
					   input_count, //parameters count
					   vshader_blob->lpVtbl->GetBufferPointer(vshader_blob),
					   vshader_blob->lpVtbl->GetBufferSize(vshader_blob),
					   &input_layout);

			   ASSERT(result == S_OK);
			   direct_device->device_context->lpVtbl->IASetInputLayout(direct_device->device_context, input_layout);
   }
   //////////////////////////////////////////////////////////////

   return(direct_device); 
}

static void
d3d11_draw(platform_renderer *renderer, int game_w, int game_h, unsigned char *game_pixels)
{
	d3d11_device *direct_device = (d3d11_device *) renderer;

	render_vertex *vertex_buffer = 0;
	//get the vertex buffer from the buffer one.

	D3D11_MAPPED_SUBRESOURCE current_swap_vertex_buffer; 


	direct_device->device_context->lpVtbl->Map(direct_device->device_context,
			(ID3D11Resource *)direct_device->vertex_buffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&current_swap_vertex_buffer);

	vertex_buffer = current_swap_vertex_buffer.pData;

	direct_device->device_context->lpVtbl->Unmap(direct_device->device_context,
			(ID3D11Resource *)direct_device->vertex_buffer,
			0);

	////////////////SET VIEWPORT/////////////////////////


    D3D11_VIEWPORT vport = {0}; 
    
    vport.TopLeftX = renderer->rec_x0;
    vport.TopLeftY = renderer->rec_y0;
    vport.Width    = renderer->rec_x1 - renderer->rec_x0; //How big the world looks
    vport.Height   = renderer->rec_y1 - renderer->rec_y0;
    vport.MinDepth = 0;
    vport.MaxDepth = 1;

	direct_device->device_context->lpVtbl->RSSetViewports(direct_device->device_context, 1 ,&vport);

	/////////////////////////////////////////////////////

	/////////////////////////////////////////////////////

	ID3D11RenderTargetView *renderTarget = 
		direct_device->frame_buffer;
   direct_device->device_context->lpVtbl->OMSetRenderTargets(direct_device->device_context,
										  1, 
										  &renderTarget,
										  0);


	/////////////////////////////////////////////////////

   /*
	    1
      y |
        |
	   -1===== 1
	       X

	   For UVS:
	   (0, 0)
	    ======
      V |    |
        |    |
	    ======
	      U   (1, 1)
   */

   /*
	  v1==v2
	  |  /
	  | /
	  v0
   */

	render_vertex *v0 = vertex_buffer;
	render_vertex *v1 = vertex_buffer + 1;
	render_vertex *v2 = vertex_buffer + 2;

	render_vertex *v3 = vertex_buffer + 3;
	render_vertex *v4 = vertex_buffer + 4;
	render_vertex *v5 = vertex_buffer + 5;

	v0->pos[0] = -1;
	v0->pos[1] = -1;
	v0->pos[2] = 1;
	v0->uvs[0] = 0;
	v0->uvs[1] = 1;

	v1->pos[0] = -1;
	v1->pos[1] = 1;
	v1->pos[2] = 1;
	v1->uvs[0] = 0;
	v1->uvs[1] = 0;

	v2->pos[0] = 1;
	v2->pos[1] = 1;
	v2->pos[2] = 1;
	v2->uvs[0] = 1;
	v2->uvs[1] = 0;

   /*
	      v4
	     /|
	    / |
	  v3--v5
   */

	v3->pos[0] = -1;
	v3->pos[1] = -1;
	v3->pos[2] = 1;
	v3->uvs[0] = 0;
	v3->uvs[1] = 1;

	v4->pos[0] = 1;
	v4->pos[1] = 1;
	v4->pos[2] = 1;
	v4->uvs[0] = 1;
	v4->uvs[1] = 0;

	v5->pos[0] = 1;
	v5->pos[1] = -1;
	v5->pos[2] = 1;
	v5->uvs[0] = 1;
	v5->uvs[1] = 1;

	float clear_color[4] = {
		0,
		0,
		0,
		255,
	};


	/////////////////////////////////////////

   //paint dynamic canvas texture
	D3D11_MAPPED_SUBRESOURCE canvas_texture_subresource; 

	//Map and copy game pixels here
	direct_device->device_context->lpVtbl->Map(direct_device->device_context,
			(ID3D11Resource *)direct_device->canvas_texture,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&canvas_texture_subresource);

	unsigned char *pixels = canvas_texture_subresource.pData;


	unsigned int w = game_w;
	unsigned int h = game_h;

	unsigned int dest_pitch = w;

	unsigned char *pixels_at = pixels;

	for(unsigned int y = 0; y < h; y++)
	{
		for(unsigned int x = 0; x < w; x ++)
		{
			int i = x + y * w;

#if 0
			unsigned int col = x > 10 ? 0xff0000ff : 0xffffffff;
			((unsigned int *)pixels_at)[x] = col;
#else
			unsigned char pix = game_pixels[i];
			unsigned int r = (pix & 0x01) ? 255 : 0;
			unsigned int g = (pix & 0x02) ? 255 : 0;
			unsigned int b = (pix & 0x04) ? 255 : 0;

			//pixels_at are in ABGR order
			((unsigned int *)pixels_at)[x] = 0xff000000 | (b << 16) | (g << 8) | (r); 

#endif
		}

		pixels_at += canvas_texture_subresource.RowPitch;
	}


	//unmap so shader can use it.
	direct_device->device_context->lpVtbl->Unmap(
			direct_device->device_context,
			(ID3D11Resource *)direct_device->canvas_texture,
			0);

	//ponelo como view para el shader, con el fin de que se renderizarlo luego.
	direct_device->device_context->lpVtbl->PSSetShaderResources(direct_device->device_context,
			0, 1, &direct_device->canvas_texture_view);
	/////////////////////////////////////////



	direct_device->device_context->lpVtbl->ClearRenderTargetView(
			direct_device->device_context,
			direct_device->frame_buffer,
			clear_color);


   direct_device->device_context->lpVtbl->PSSetShaderResources(direct_device->device_context,
		   0, 1, &direct_device->canvas_texture_view);

   direct_device->device_context->lpVtbl->Draw(direct_device->device_context,
		   6, 0);


}

static void
d3d11_swap_buffers(platform_renderer *renderer)
{
	d3d11_device *direct_device = (d3d11_device *)renderer;
    direct_device->swap_chain->lpVtbl->Present(direct_device->swap_chain, 0, 0);
}
