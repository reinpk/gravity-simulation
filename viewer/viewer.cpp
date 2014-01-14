/* GravitySimulation Viewer  --- viewer.cpp
 *
 *Peter K. Reinhardt - 2008
 *This code may not be distributed without the author's explicit written permission.
 */

#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>

// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)

//const int DISPLAY_MOVIE_OLD = 0;
const int DISPLAY_MOVIE = 0;
const int DISPLAY_EXIT_STATE = 1;

struct CUSTOMVERTEX
{
    float x, y, z;    // from the D3DFVF_XYZRHW flag
    DWORD color;    // from the D3DFVF_DIFFUSE flag
};


// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
LPDIRECT3DVERTEXBUFFER9 t_buffer;
LPD3DXFONT m_font; //for the percentage displayed tag

//simulation variables
double initialBounds;
int numParticles;
int aliveParticles;
int numFrames;
int exists[10000];
CUSTOMVERTEX positions[10000];
CUSTOMVERTEX wireFrame[12][400];
CUSTOMVERTEX COM[2];

std::ofstream fout;

// function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void initWireFrame();
void render_frame(double xyrot, double xzrot);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	std::ifstream fin ("dataIn.txt");
	//fout.open("output.txt");
	COM[0].x = 0;
	COM[0].y = 0;
	COM[0].z = 0;
	COM[0].color = D3DCOLOR_XRGB(255, 120, 120);
	COM[1].x = 0;
	COM[1].y = 0;
	COM[1].z = 0;
	COM[1].color = D3DCOLOR_XRGB(255, 120, 120);

	DWORD pointColor = D3DCOLOR_XRGB(160, 255, 135);
	//DWORD pointColor = D3DCOLOR_XRGB(255, 255, 255);
	int display_type;
	fin >> display_type;
	if (display_type == DISPLAY_MOVIE) {
		fin >> numParticles >> initialBounds >> numFrames;
		fin >> COM[0].x >> COM[0].y >> COM[0].z;
		COM[1].x = COM[0].x;
		COM[1].y = COM[0].y;
		COM[1].z = COM[0].z;
	} /*else if (display_type == DISPLAY_MOVIE_OLD) {
		fin >> numParticles >> initialBounds >> numFrames;
	} */else if (display_type == DISPLAY_EXIT_STATE) {
		double temp;
		fin >> numParticles >> initialBounds >> temp >> temp >> temp >> temp >> temp;
	} else {
		//THIS CODE IS TEMPORARY FOR THE FIRST SCIENTIFIC RESULT
		numParticles = display_type;
		fin >> initialBounds >> numFrames;
		display_type = DISPLAY_MOVIE;
	}
	aliveParticles = 0;
	
	// the handle for the window, filled by a function
    HWND hWnd;
    // this struct holds information for the window class
    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "WindowClass1";

    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(NULL,
                          "WindowClass1",    // name of the window class
                          "GravityViewer 1.0",   // title of the window
                          WS_OVERLAPPEDWINDOW,    // window style
                          50,    // x-position of the window
                          50,    // y-position of the window
                          1000,    // width of the window
                          800,    // height of the window
                          NULL,    // we have no parent window, NULL
                          NULL,    // we aren't using menus, NULL
                          hInstance,    // application handle
                          NULL);    // used with multiple windows, NULL

    ShowWindow(hWnd, nCmdShow);

	//initialize direct3D
	initD3D(hWnd);

	//initialize the wireframe box around the field
	initWireFrame();

	// create the vertex and store the pointer into t_buffer, which is created globally
	d3ddev->CreateVertexBuffer(numParticles*sizeof(CUSTOMVERTEX),
								0,
								CUSTOMFVF,
								D3DPOOL_MANAGED,
								&t_buffer,
								NULL);

    // this struct holds Windows event messages
    MSG msg;
	double xyrot = 0.0;
	double xzrot = 0.0;
	double change = 0.05;
	if (display_type == DISPLAY_MOVIE) {
		for (int i = 0; i < numFrames; i++)
		{
			int dead = 0;
			double temp;
			for (int j = 0; j < numParticles; j++) {
				fin >> exists[j - dead];
				if (exists[j - dead] == 1) {
					fin >> positions[j - dead].x >> positions[j - dead].y >> positions[j - dead].z >> temp;
					positions[j - dead].color = pointColor;
				} else {
					//fout << "Dead: " << j << "  " << exists[j - dead] << std::endl;
					dead++;
					fin >> temp >> temp >> temp >> temp;
				}
			}
			aliveParticles = numParticles - dead;

			DWORD starting_point = GetTickCount();
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			render_frame(xyrot, xzrot);
			if (KEY_DOWN(VK_UP) && xzrot < 1.57)
				xzrot += change;
			if (KEY_DOWN(VK_DOWN) && xzrot > -1.57)
				xzrot -= change;
			if (KEY_DOWN(VK_LEFT))
				xyrot -= change;
			if (KEY_DOWN(VK_RIGHT))
				xyrot += change;
			if (KEY_DOWN(VK_SHIFT)) {
				while (KEY_DOWN(VK_SHIFT)) {
					DWORD starting_point = GetTickCount();
					if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						if (msg.message == WM_QUIT)
							break;

						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					render_frame(xyrot, xzrot);
					if (KEY_DOWN(VK_UP) && xzrot < 1.57)
						xzrot += change;
					if (KEY_DOWN(VK_DOWN) && xzrot > -1.57)
						xzrot -= change;
					if (KEY_DOWN(VK_LEFT))
						xyrot -= change;
					if (KEY_DOWN(VK_RIGHT))
						xyrot += change;

					while ((GetTickCount() - starting_point) < 50);
				}
			}


			// check the 'escape' key
			if(KEY_DOWN(VK_ESCAPE))
				PostMessage(hWnd, WM_DESTROY, 0, 0);

			while ((GetTickCount() - starting_point) < 100);
		}
		fin >> COM[1].x >> COM[1].y >> COM[1].z;
	} else if (display_type == DISPLAY_EXIT_STATE) {
			int dead = 0;
			double temp;
			for (int j = 0; j < numParticles; j++) {
				fin >> exists[j - dead];
				if (exists[j - dead] == 1) {
					fin >> temp >> temp >> positions[j - dead].x >> positions[j - dead].y >> positions[j - dead].z >> temp >> temp >> temp;
					positions[j - dead].color = pointColor;
				} else {
					dead++;
					fin >> temp >> temp >> temp >> temp >> temp >> temp >> temp >> temp;
				}
			}
			aliveParticles = numParticles - dead;
	}
	DWORD starting_point;
	while (TRUE) {
		starting_point = GetTickCount();
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		render_frame(xyrot, xzrot);
		if (KEY_DOWN(VK_UP) && xzrot < 1.57)
			xzrot += change;
		if (KEY_DOWN(VK_DOWN) && xzrot > -1.57)
			xzrot -= change;
		if (KEY_DOWN(VK_LEFT))
			xyrot -= change;
		if (KEY_DOWN(VK_RIGHT))
			xyrot += change;

		// check the 'escape' key
		if(KEY_DOWN(VK_ESCAPE))
			PostMessage(hWnd, WM_QUIT, 0, 0);

		while ((GetTickCount() - starting_point) < 50);

		starting_point = GetTickCount();
	}
	
	DestroyWindow(hWnd);
	cleanD3D();
    return 0;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }
    return DefWindowProc (hWnd, message, wParam, lParam);
}

// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // create a device class using this information and information from the d3dpp stuct
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);
	d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);    // turn off the 3D lighting
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer

    return;
}

//initializes the wire frame around the field.
void initWireFrame()
{
	DWORD color = D3DCOLOR_XRGB(120, 130, 255);
	for (int i = 0; i < 400; i++) {
		wireFrame[0][i].x = initialBounds/2.0;
		wireFrame[0][i].y = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[0][i].z = initialBounds/2.0;
		wireFrame[0][i].color = color;

		wireFrame[1][i].x = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[1][i].y = initialBounds/2.0;
		wireFrame[1][i].z = initialBounds/2.0;
		wireFrame[1][i].color = color;

		wireFrame[2][i].x = -initialBounds/2.0;
		wireFrame[2][i].y = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[2][i].z = initialBounds/2.0;
		wireFrame[2][i].color = color;

		wireFrame[3][i].x = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[3][i].y = -initialBounds/2.0;
		wireFrame[3][i].z = initialBounds/2.0;
		wireFrame[3][i].color = color;

		wireFrame[4][i].x = initialBounds/2.0;
		wireFrame[4][i].y = initialBounds/2.0;
		wireFrame[4][i].z = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[4][i].color = color;

		wireFrame[5][i].x = -initialBounds/2.0;
		wireFrame[5][i].y = initialBounds/2.0;
		wireFrame[5][i].z = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[5][i].color = color;

		wireFrame[6][i].x = -initialBounds/2.0;
		wireFrame[6][i].y = -initialBounds/2.0;
		wireFrame[6][i].z = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[6][i].color = color;

		wireFrame[7][i].x = initialBounds/2.0;
		wireFrame[7][i].y = -initialBounds/2.0;
		wireFrame[7][i].z = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[7][i].color = color;

		wireFrame[8][i].x = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[8][i].y = initialBounds/2.0;
		wireFrame[8][i].z = -initialBounds/2.0;
		wireFrame[8][i].color = color;

		wireFrame[9][i].x = -initialBounds/2.0;
		wireFrame[9][i].y = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[9][i].z = -initialBounds/2.0;
		wireFrame[9][i].color = color;

		wireFrame[10][i].x = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[10][i].y = -initialBounds/2.0;
		wireFrame[10][i].z = -initialBounds/2.0;
		wireFrame[10][i].color = color;

		wireFrame[11][i].x = initialBounds/2.0;
		wireFrame[11][i].y = i*(initialBounds/400.0) - initialBounds/2.0;
		wireFrame[11][i].z = -initialBounds/2.0;
		wireFrame[11][i].color = color;
	}
}

// this is the function used to render a single frame
void render_frame(double xyrot, double xzrot)
{
    // clear the window to black
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    d3ddev->BeginScene();    // begins the 3D scene
	
    // select which vertex format we are using
    d3ddev->SetFVF(CUSTOMFVF);
	//VIEW TRANSFORMATION
    D3DXMATRIX matView;    // the view transform matrix
	double x = cos(xyrot);
	double y = sin(xyrot);
	double z = sin(xzrot);
	double length = 2.5*initialBounds/*/sqrt(x*x + y*y + z*z)*/;

    D3DXMatrixLookAtLH(&matView,
                       &D3DXVECTOR3 (length*x, length*y, length*z),    // the camera position
                       &D3DXVECTOR3 (0.0f, 0.0f, 0.0f),    // the look-at position
                       &D3DXVECTOR3 (0.0f, 0.0f, 1.0f));    // the up direction

    d3ddev->SetTransform(D3DTS_VIEW, &matView);

	//PROJECTION TRANSFORMATION
    D3DXMATRIX matProjection;
    D3DXMatrixPerspectiveFovLH(&matProjection,
                               D3DXToRadian(45),    // the horizontal field of view
                               1.25f, //aspect ratio
                               1.0f,    //the near view-plane
                               10*initialBounds);    //the far view-plane
    d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);

	//display the particles

	void* pVoid;    // the void pointer

	t_buffer->Lock(0, 0, (void**)&pVoid, 0);    // lock the vertex buffer
	memcpy(pVoid, positions, aliveParticles*sizeof(CUSTOMVERTEX));    // copy the vertices to the locked buffer
	t_buffer->Unlock();    // unlock the vertex buffer

	d3ddev->SetStreamSource(0, t_buffer, 0, sizeof(CUSTOMVERTEX));
	d3ddev->DrawPrimitive(D3DPT_POINTLIST, 0, aliveParticles);

	for (int i = 0; i < 12; i++) {
		t_buffer->Lock(0, 0, (void**)&pVoid, 0);    // lock the vertex buffer
		memcpy(pVoid, wireFrame[i], 400*sizeof(CUSTOMVERTEX));    // copy the vertices to the locked buffer
		t_buffer->Unlock();    // unlock the vertex buffer
		d3ddev->SetStreamSource(0, t_buffer, 0, sizeof(CUSTOMVERTEX));
		d3ddev->DrawPrimitive(D3DPT_POINTLIST, 0, 400);
	}

	//draw the center of mass
	t_buffer->Lock(0, 0, (void**)&pVoid, 0);    // lock the vertex buffer
	memcpy(pVoid, COM, 2*sizeof(CUSTOMVERTEX));    // copy the vertices to the locked buffer
	t_buffer->Unlock();    // unlock the vertex buffer
	d3ddev->SetStreamSource(0, t_buffer, 0, sizeof(CUSTOMVERTEX));
	d3ddev->DrawPrimitive(D3DPT_POINTLIST, 0, 2);

	d3ddev->EndScene();
    d3ddev->Present(NULL, NULL, NULL, NULL);
    return;
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	t_buffer->Release();
    d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D

    return;
}