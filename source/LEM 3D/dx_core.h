#ifndef DXCORE_H
#define DXCORE_H

//Matrices
D3DXMATRIX matIdentity;
D3DXMATRIX matLightView;
D3DXMATRIX matLightProjection;
D3DXMATRIX matDustView;
D3DXMATRIX matDustProjection;

//D3D and Windows
unsigned SCREEN_WIDTH = 0;
unsigned SCREEN_HEIGHT = 0;
unsigned REFRESH_RATE = 0;
HWND hwnd = NULL;
LPDIRECT3D9 d3d = NULL;				
LPDIRECT3DDEVICE9 d3ddev = NULL;	
D3DPRESENT_PARAMETERS d3dpp;

//Effects
LPD3DXEFFECT globalLightingEffect = NULL;

//Shadow settings
unsigned SHADOW_MAP_SIZE = 2048;
float shadowMapBias = 0.005f;

//Text
LPD3DXFONT debugFont = NULL;

//Extra textures
IDirect3DTexture9 *moonDetailSmallTex = NULL;
IDirect3DTexture9 *dustBlastTex = NULL;

//Sound
irrklang::ISoundEngine* soundEngine = NULL;
irrklang::ISound* engine_sound = NULL;
irrklang::ISound* rcs_sound = NULL;

template <class T> void SAFE_RELEASE(T **ppT)
{
    if (*ppT)
	{
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#endif
