#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <math.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE) 


// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
LPD3DXMESH meshCube;

// function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(double xyrot, double xzrot);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
                          "First Light 1.0",   // title of the window
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

    // this struct holds Windows event messages
    MSG msg;
	double xyrot = 0.0;
	double xzrot = 0.0;
	double change = 0.05;
	while(TRUE)
	{
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

		// check the 'escape' key
		if(KEY_DOWN(VK_ESCAPE))
			PostMessage(hWnd, WM_DESTROY, 0, 0);

		while ((GetTickCount() - starting_point) < 25);
	}

	cleanD3D();
    return msg.wParam;
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

// this is the function used to render a single frame
void render_frame(double xyrot, double xzrot)
{
    // clear the window to a deep blue
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
	double length = 10.0/sqrt(x*x + y*y + z*z);

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
                               100.0f);    //the far view-plane
    d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);

	D3DXCreateSphere(d3ddev, 2.0, 4, 2, &meshCube, NULL);
	meshCube->DrawSubset(0);

    d3ddev->EndScene();
    d3ddev->Present(NULL, NULL, NULL, NULL);

    return;
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	meshCube->Release();    // close and release the teapot mesh
    d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D

    return;
}