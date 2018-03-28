//#define D3D_DEBUG_INFO
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <d3d9.h>
#include <d3dx9.h>
#include <irrKlang.h>

using namespace std;

#include "dx_core.h"
#include "dx_classes.h"
#include "physics.h"
#include "gamecontrol.h"

MeshObjectTNS meshApolloLEM("models/ApolloLEM.x");
MeshObjectTNS meshLS1("models/LS1.x");

SpriteObject spriteSun("textures/sun.png");
SpriteObject spriteExhaust("textures/exhaust.png");
SpriteObject spriteCrosshair("textures/gauges/crosshair.png");
SpriteObject spriteAxes("textures/gauges/axes.png");
SpriteObject spriteCross("textures/gauges/cross.png");
SpriteObject spriteArrowDown("textures/gauges/arrow_down.png");
SpriteObject spriteArrowUp("textures/gauges/arrow_up.png");

Spacecraft ApolloLEM(2.8f, 0.1f, &meshApolloLEM);
Spacecraft *activeCraft = NULL;

DrawableTex2DDepth spacecraftCloseShadowMap;

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "irrKlang.lib")

//Misc.
void setPauseState(bool p)
{
	paused = p;
	engine_sound->setIsPaused(p);
	rcs_sound->setIsPaused(p);
}

//Initialisation
void initProgram()
{
	//Camera projection matrix
	mainCam.UpdateProjection(D3DXToRadian(65), float(SCREEN_WIDTH)/float(SCREEN_HEIGHT), 0.1f, 2000);

	//Light projection matrix
	D3DXMatrixOrthoLH(&matLightProjection, 12, 12, 0.1f, 16);

	//Dust projection matrix
	D3DXMatrixOrthoLH(&matDustProjection, 25, 25, 0.1f, 1);

	//Spacecraft
	ApolloLEM.contactPoints.push_back(D3DXVECTOR3(0.03328f, -2.7827f, -3.65543f));
	ApolloLEM.contactPoints.push_back(D3DXVECTOR3(0.03328f, -2.7827f, 4.62773f));
	ApolloLEM.contactPoints.push_back(D3DXVECTOR3(4.17299f, -2.7827f, 0.48802f));
	ApolloLEM.contactPoints.push_back(D3DXVECTOR3(-4.11017f, -2.7827f, 0.48802f));
	activeCraft = &ApolloLEM;
	activeCraft->Reset(D3DXVECTOR3(200, 200, 600), D3DXVECTOR3(0, -5, -10), 60);

	//Sound
	engine_sound = soundEngine->play2D("sound/engine.wav", true, false, true);
	rcs_sound = soundEngine->play2D("sound/rcs.wav", true, false, true);

	//Misc.
	d3ddev->GetViewport(&fullViewport);
	D3DXMatrixIdentity(&matIdentity);
}
bool initResources()
{
	//Mesh
	if (!meshApolloLEM.Load()){MessageBox(NULL, "Unable to load ApolloLEM.x", "Error", MB_OK); return false;}
		if (!meshApolloLEM.ComputeTangentSpace()){MessageBox(NULL, "Can't compute tangent space for 'ApolloLEM.x'", "Error", MB_OK); return false;}
	if (!meshLS1.Load()){MessageBox(NULL, "Unable to load LS1.x", "Error", MB_OK); return false;}
	
	//Extra textures
	if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/moon_detail_s.jpg", &moonDetailSmallTex))){MessageBox(NULL, "Unable to load 'moon_detail_s.jpg'", "Error", MB_OK); return false;}
	if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/dust.png", &dustBlastTex))){MessageBox(NULL, "Unable to load 'dust.png'", "Error", MB_OK); return false;}

	//Shader
	if (FAILED(D3DXCreateEffectFromFile(d3ddev, "shaders/Shaders.fx", NULL, NULL, D3DXSHADER_DEBUG, NULL, &globalLightingEffect, NULL)))
	{MessageBox(NULL, "Unable to read Shaders.fx", "Error", MB_OK); return false;}

	//Shadow
	if (!spacecraftCloseShadowMap.CreateResources(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, D3DFMT_R32F, d3dpp.AutoDepthStencilFormat)){return false;}

	//Sprite
	if (!spriteSun.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteSun'", "Error", MB_OK); return false;}
	if (!spriteExhaust.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteExhaust'", "Error", MB_OK); return false;}
	if (!spriteCrosshair.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteCrosshair'", "Error", MB_OK); return false;}
	if (!spriteAxes.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteAxes'", "Error", MB_OK); return false;}
	if (!spriteCross.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteCross'", "Error", MB_OK); return false;}
	if (!spriteArrowDown.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteArrowDown'", "Error", MB_OK); return false;}
	if (!spriteArrowUp.CreateResources()){MessageBox(NULL, "Can't create resources for 'spriteArrowUp'", "Error", MB_OK); return false;}

	//Misc.
	if (FAILED(D3DXCreateFont(d3ddev, 0, 0, 0, 0, TRUE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &debugFont))){return false;}

	//Sound
	soundEngine = irrklang::createIrrKlangDevice();
	if (!soundEngine){MessageBox(NULL, "Sound engine failed to initialise", "Error", MB_OK); return false;}

	return true;
}
bool initD3D(HWND hWnd)
{
	//Create D3D9
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL){return false;}

	//Check current display mode
	D3DDISPLAYMODE d3ddm; 
	if (FAILED(d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))){return false;} 
	SCREEN_WIDTH = d3ddm.Width;
	SCREEN_HEIGHT = d3ddm.Height;
	REFRESH_RATE = d3ddm.RefreshRate;

	//DirectX parameters
    ZeroMemory(&d3dpp, sizeof(d3dpp));	
	d3dpp.BackBufferFormat = d3ddm.Format;				
	d3dpp.BackBufferWidth = SCREEN_WIDTH;							
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;						
	d3dpp.BackBufferCount = 1;								
    d3dpp.Windowed = FALSE;								
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;		
	d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;			
	d3dpp.MultiSampleQuality = 0;						
	d3dpp.hDeviceWindow = hWnd;								
    d3dpp.EnableAutoDepthStencil = TRUE;					
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;			
	d3dpp.Flags = 0;									
	d3dpp.FullScreen_RefreshRateInHz = REFRESH_RATE;					
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; 

	//Check shader/vertex processing caps
	D3DCAPS9 d3dCaps;
	if(FAILED(d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps))){return false;}
	DWORD VertexProcessingMethod = d3dCaps.VertexProcessingCaps != 0 ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	if (d3dCaps.VertexShaderVersion < D3DVS_VERSION(2, 0) || d3dCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
	{MessageBox(NULL, "Default video adapter does not support shader version 2.0", "Error", MB_OK); return false;}

	//Device creation
    if(FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, VertexProcessingMethod, &d3dpp, &d3ddev))){return false;}

	//Resource initiation
	if (!initResources()){return false;}

	return true;
}

//Device handling
bool onLostDevice()
{
	if (FAILED(debugFont->OnLostDevice())){return false;}

	spacecraftCloseShadowMap.DeleteResources();

	if (!spriteSun.LostDevice()){return false;}
	if (!spriteExhaust.LostDevice()){return false;}
	if (!spriteCrosshair.LostDevice()){return false;}
	if (!spriteAxes.LostDevice()){return false;}
	if (!spriteCross.LostDevice()){return false;}
	if (!spriteArrowDown.LostDevice()){return false;}
	if (!spriteArrowUp.LostDevice()){return false;}

	if (FAILED(globalLightingEffect->OnLostDevice())){return false;}

	return true;
}
bool onResetDevice()
{
	if (FAILED(debugFont->OnResetDevice())){return false;}

	if (!spacecraftCloseShadowMap.CreateResources(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, D3DFMT_R32F, d3dpp.AutoDepthStencilFormat)){return false;}

	if (!spriteSun.ResetDevice()){return false;}
	if (!spriteExhaust.ResetDevice()){return false;}
	if (!spriteCrosshair.ResetDevice()){return false;}
	if (!spriteAxes.ResetDevice()){return false;}
	if (!spriteCross.ResetDevice()){return false;}
	if (!spriteArrowDown.ResetDevice()){return false;}
	if (!spriteArrowUp.ResetDevice()){return false;}

	if (FAILED(globalLightingEffect->OnResetDevice())){return false;}

	return true;
}
bool isDeviceLost()
{
	HRESULT hr = d3ddev->TestCooperativeLevel();

	if (hr == D3DERR_DEVICELOST)
	{
		if (!paused){setPauseState(true);}
		Sleep(100);
		return true;
	}

	else if (hr == D3DERR_DEVICENOTRESET)
	{
		if (!onLostDevice()){MessageBox(NULL, "Can't prepare lost device", "Error", MB_OK); return true;}
		if (FAILED(d3ddev->Reset(&d3dpp))){MessageBox(NULL, "Can't reset the present parameters for the device", "Error", MB_OK); return true;}
		if (!onResetDevice()){MessageBox(NULL, "Can't reset the device", "Error", MB_OK); return true;}
	}

	return false;
}

//Draw calls
bool draw_Readout(string caption, float value, string unit, unsigned precision, D3DXVECTOR2 pos, D3DCOLOR colour, LPD3DXFONT font)
{
	stringstream readoutStream;
	readoutStream.setf(ios::fixed);
	RECT textRect = {long(pos.x), long(pos.y), 0, 0};
	readoutStream << caption << setprecision(precision) << value << unit;
	if(FAILED(font->DrawText(NULL, readoutStream.str().c_str(), -1, &textRect, DT_NOCLIP, colour))){return false;}
	
	return true;
}
bool draw_Caption(string caption, D3DXVECTOR2 pos, D3DCOLOR colour, LPD3DXFONT font)
{
	RECT textRect = {long(pos.x), long(pos.y), 0, 0};
	if(FAILED(font->DrawText(NULL, caption.c_str(), -1, &textRect, DT_NOCLIP, colour))){return false;}
	
	return true;
}
bool draw_Shadow_Map()
{
	globalLightingEffect->SetTechnique("ShadowMap");
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		activeCraft->mesh->RenderCustomMatrix(activeCraft->pos, activeCraft->orientation, false);
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Spacecraft()
{
	globalLightingEffect->SetValue("v3_LightDir", &lightDir, sizeof(D3DXVECTOR3));
	globalLightingEffect->SetValue("v3_CamPos", &mainCam.pos, sizeof(D3DXVECTOR3));

	globalLightingEffect->SetValue("f_AmbientStrength", &lightAmbient, sizeof(float));
	globalLightingEffect->SetValue("f_DiffuseStrength", &lightDiffuse, sizeof(float));
	globalLightingEffect->SetValue("f_ShadowBias", &shadowMapBias, sizeof(float));

	globalLightingEffect->SetBool("b_ReceiveShadows", true);

	globalLightingEffect->SetTexture("t_ShadowMap", spacecraftCloseShadowMap.drawTexture);

	globalLightingEffect->SetTechnique("NormalMap");
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		activeCraft->mesh->RenderCustomMatrix(activeCraft->pos, activeCraft->orientation, true);
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Terrain()
{
	globalLightingEffect->SetValue("v3_LightDir", &lightDir, sizeof(D3DXVECTOR3));

	globalLightingEffect->SetValue("f_AmbientStrength", &lightAmbient, sizeof(float));
	globalLightingEffect->SetValue("f_DiffuseStrength", &lightDiffuse, sizeof(float));
	globalLightingEffect->SetValue("f_DustFactor", &dustFactor, sizeof(float));

	globalLightingEffect->SetTexture("t_ShadowMap", spacecraftCloseShadowMap.drawTexture);
	globalLightingEffect->SetTexture("t_MoonDetailSmall", moonDetailSmallTex);
	globalLightingEffect->SetTexture("t_DustBlast", dustBlastTex);

	globalLightingEffect->SetTechnique("Terrain");
	UINT numPasses = 0;
	if (FAILED(globalLightingEffect->Begin(&numPasses, 0))){return false;}
	for (UINT i = 0; i < numPasses; i++)
	{
	if (FAILED(globalLightingEffect->BeginPass(i))){return false;}
		meshLS1.RenderGlobal(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), true);
	if (FAILED(globalLightingEffect->EndPass())){return false;}
	}
	if (FAILED(globalLightingEffect->End())){return false;}

	return true;
}
bool draw_Sprites()
{
	//Sun
	if (D3DXVec3Dot(&-lightDir, &(mainCam.lookAt - mainCam.pos)) > 0)
	{
		D3DXVECTOR3 sunScreenPos;
		D3DXVec3Project(&sunScreenPos, &(mainCam.pos - lightDir), &fullViewport, &mainCam.matProjection, &mainCam.matView, &matIdentity);
		sunScreenPos.z = 1;
		if (!spriteSun.Render(NULL, D3DXVECTOR3(256, 256, 0), sunScreenPos, D3DXVECTOR2(1, 1), 255)){return false;}
	}

	//Exhaust
	D3DXVECTOR3 exhaustScreenPos;
	D3DXVec3Project(&exhaustScreenPos, &(activeCraft->pos - 2.5f*activeCraft->up + 0.5f*activeCraft->back), &fullViewport, &mainCam.matProjection, &mainCam.matView, &matIdentity);
	if (!spriteExhaust.Render(NULL, D3DXVECTOR3(256, 256, 0), exhaustScreenPos, 14/camDistance*activeCraft->thrust/100*D3DXVECTOR2(1, 1), 255)){return false;}

	return true;
}
bool draw_Overlays()
{
	//Velocity crosshair
	if (!activeCraft->landed && D3DXVec3Dot(&activeCraft->vel, &(mainCam.lookAt - mainCam.pos)) > 0)
	{
		D3DXVECTOR3 crosshairScreenPos;
		D3DXVec3Project(&crosshairScreenPos, &(mainCam.pos + activeCraft->vel), &fullViewport, &mainCam.matProjection, &mainCam.matView, &matIdentity);
		crosshairScreenPos.z = 0;
		if (!spriteCrosshair.Render(NULL, D3DXVECTOR3(25, 25, 0), crosshairScreenPos, D3DXVECTOR2(1, 1), overlayAlpha)){return false;}
	}

	//Roll/pitch
	RECT barRect = {0, 86, 200, 114};
	if (!draw_Caption("ROLL", D3DXVECTOR2(88, 75), fabs(activeCraft->left.y) < 0.2f ? D3DCOLOR_XRGB(0, 255, 0) : D3DCOLOR_XRGB(255, 0, 0), debugFont)){return false;}
	if (!draw_Caption("PITCH", D3DXVECTOR2(85, 132), fabs(activeCraft->back.y) < 0.2f ? D3DCOLOR_XRGB(0, 255, 0) : D3DCOLOR_XRGB(255, 0, 0), debugFont)){return false;}
	if (!spriteAxes.Render(&barRect, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(10, 100, 0), D3DXVECTOR2(1, 1), overlayAlpha)){return false;}
	if (!spriteArrowDown.Render(NULL, D3DXVECTOR3(5, 10, 0), D3DXVECTOR3(110 + 100*activeCraft->left.y, 114, 0), D3DXVECTOR2(1, 1), overlayAlpha)){return false;}
	if (!spriteArrowUp.Render(NULL, D3DXVECTOR3(5, 0, 0), D3DXVECTOR3(110 + 100*-activeCraft->back.y, 114, 0), D3DXVECTOR2(1, 1), overlayAlpha)){return false;}

	//Lateral velocity
	D3DXVECTOR2 leftLatProj, backLatProj;
	D3DXVec2Normalize(&leftLatProj, &D3DXVECTOR2(activeCraft->left.x, activeCraft->left.z));
	D3DXVec2Normalize(&backLatProj, &D3DXVECTOR2(activeCraft->back.x, activeCraft->back.z));
	float leftLatVel = 10*D3DXVec2Dot(&D3DXVECTOR2(activeCraft->vel.x, activeCraft->vel.z), &leftLatProj);
	leftLatVel = leftLatVel > 100 ? 100 : leftLatVel < -100 ? -100 : leftLatVel;
	float backLatVel = 10*D3DXVec2Dot(&D3DXVECTOR2(activeCraft->vel.x, activeCraft->vel.z), &backLatProj);
	backLatVel = backLatVel > 100 ? 100 : backLatVel < -100 ? -100 : backLatVel;
	if (!draw_Caption("LATERAL VELOCITY", D3DXVECTOR2(float(SCREEN_WIDTH) - 185, 225), D3DXVec2Length(&D3DXVECTOR2(activeCraft->vel.x, activeCraft->vel.z)) < 5 ? D3DCOLOR_XRGB(0, 255, 0) : D3DCOLOR_XRGB(255, 0, 0), debugFont)){return false;}
	if (!spriteAxes.Render(NULL, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(float(SCREEN_WIDTH) - 210, 10, 0), D3DXVECTOR2(1, 1), overlayAlpha)){return false;}
	if (!spriteCross.Render(NULL, D3DXVECTOR3(5, 5, 0), D3DXVECTOR3(float(SCREEN_WIDTH) - 110 - leftLatVel, 110 + backLatVel, 0), D3DXVECTOR2(1, 1), overlayAlpha)){return false;}

	//Overlay text
	if (!draw_Readout("Fuel: ", activeCraft->fuel, "%", 1, D3DXVECTOR2(10, 10), activeCraft->fuel > 20 ? D3DCOLOR_XRGB(0, 255, 0) : D3DCOLOR_XRGB(255, 0, 0), debugFont)){return false;}
	if (!draw_Readout("Thrust: ", activeCraft->thrust, "%", 0, D3DXVECTOR2(10, 30), D3DCOLOR_XRGB(0, 255, 0), debugFont)){return false;}
	if (!draw_Readout("Vert. speed: ", activeCraft->vel.y, "m/s", 1, D3DXVECTOR2(10, 180), activeCraft->vel.y > -1 ? D3DCOLOR_XRGB(0, 255, 0) : D3DCOLOR_XRGB(255, 0, 0), debugFont)){return false;}
	if (paused){if (!draw_Caption("PAUSED", D3DXVECTOR2(10, float(SCREEN_HEIGHT) - 30), D3DCOLOR_XRGB(0, 0, 255), debugFont)){return false;}}

	return true;
}

//Rendering
void preparePhysics()
{
	activeCraft->UpdatePhysics(timeDelta);

	if (!activeCraft->CheckAboveMesh(&meshLS1))
	{
		D3DXVECTOR3 newUp(0, 1, 0);
		activeCraft->up = newUp;
		D3DXVECTOR3 newLeft(activeCraft->orientation._11, 0, activeCraft->orientation._13);
		D3DXVec3Normalize(&newLeft, &newLeft);
		activeCraft->left = newLeft;
		D3DXVECTOR3 newBack(activeCraft->orientation._31, 0, activeCraft->orientation._33);
		D3DXVec3Normalize(&newBack, &newBack);
		activeCraft->back = newBack;

		activeCraft->orientation._11 = newLeft.x; activeCraft->orientation._12 = newLeft.y; activeCraft->orientation._13 = newLeft.z;
		activeCraft->orientation._21 = newUp.x; activeCraft->orientation._22 = newUp.y; activeCraft->orientation._23 = newUp.z;
		activeCraft->orientation._31 = newBack.x; activeCraft->orientation._32 = newBack.y; activeCraft->orientation._33 = newBack.z;

		activeCraft->angvel = D3DXVECTOR3(0, 0, 0);
		activeCraft->thrust = 0;

		if (D3DXVec3Length(&activeCraft->vel) < 0.1f)
		{
			activeCraft->landed = true;
			activeCraft->vel = D3DXVECTOR3(0, 0, 0);
			activeCraft->angacc = D3DXVECTOR3(0, 0, 0);

			soundEngine->play2D("sound/touchdown.wav");
		}

		else
		{
			activeCraft->pos = activeCraft->prevPos;
			activeCraft->vel.x *= 0.2f;
			activeCraft->vel.y *= -0.2f;
			activeCraft->vel.z *= 0.2f;
		}
	}
}
void prepareSounds()
{
	rcs_sound->setVolume(min(D3DXVec3Length(&activeCraft->angacc)/activeCraft->rcsTorqueAcc, 1));
	engine_sound->setVolume(activeCraft->thrust/100);
}
void prepareMisc()
{
	//Light view matrix
	D3DXVECTOR3 normalisedLightDir;
	D3DXVec3Normalize(&normalisedLightDir, &lightDir);
	D3DXVECTOR3 virtualLightPos = activeCraft->pos - 6*normalisedLightDir;
	D3DXMatrixLookAtLH(&matLightView, &virtualLightPos, &(virtualLightPos + normalisedLightDir), &D3DXVECTOR3(0, 1, 0));

	//Dust view matrix/intensity
	BOOL altRayHit = FALSE;
	float altitude = 0;
	D3DXIntersect(meshLS1.objectMesh, &activeCraft->pos, &D3DXVECTOR3(0, -1, 0), &altRayHit, NULL, NULL, NULL, &altitude, NULL, NULL);
	dustFactor = altitude > 30 ? 0 : altitude < 15 ? 1 : (30 - altitude)/(30 - 15);
	dustFactor *= activeCraft->thrust > 30 ? 1 : 1 - (30 - activeCraft->thrust)/30;
	//dustFactor *= dustFactorLanded.GetValue(activeCraft->landed ? timeDelta : 0);
	D3DXMatrixLookAtLH(&matDustView, &(activeCraft->pos + D3DXVECTOR3(0.01f, 1, 0)), &activeCraft->pos, &D3DXVECTOR3(0, 1, 0));

	//Spot camera
	camToCraft = -camDistance*D3DXVECTOR3(cos(camCircularPos)*cos(camHeightPos), sin(camHeightPos), sin(camCircularPos)*cos(camHeightPos));
	mainCam.UpdateView(activeCraft->pos - camToCraft, activeCraft->pos, D3DXVECTOR3(0, 1, 0));
	mainCam.UpdateFrustum();
}
void renderFrame()
{
//PRERENDERING
	if (!paused && !activeCraft->landed){preparePhysics();}
	prepareSounds();
	prepareMisc();

	LPDIRECT3DSURFACE9 backBuffer = NULL;
	LPDIRECT3DSURFACE9 oldDepthStencil = NULL;
	d3ddev->GetRenderTarget(0, &backBuffer);
	d3ddev->GetDepthStencilSurface(&oldDepthStencil);

//PASS ONE: render spacecraft close-up shadow map
	spacecraftCloseShadowMap.SetAsTarget();

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(255, 255, 255), 1, 0);
	d3ddev->BeginScene();

		draw_Shadow_Map();

	d3ddev->EndScene();

//PASS TWO: render main scene to back buffer
	d3ddev->SetRenderTarget(0, backBuffer);
	SAFE_RELEASE(&backBuffer);
	d3ddev->SetDepthStencilSurface(oldDepthStencil);
	SAFE_RELEASE(&oldDepthStencil);

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1, 0);
	d3ddev->BeginScene();

		draw_Spacecraft();
		draw_Terrain();
		draw_Sprites();
		draw_Overlays();

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);
}

//Cleanup
void cleanD3D()
{
	SAFE_RELEASE(&globalLightingEffect);	
	SAFE_RELEASE(&d3ddev);					
	SAFE_RELEASE(&d3d);

	SAFE_RELEASE(&debugFont);

	SAFE_RELEASE(&moonDetailSmallTex);
	SAFE_RELEASE(&dustBlastTex);
	spacecraftCloseShadowMap.DeleteResources();

	spriteSun.Clean();
	spriteExhaust.Clean();
	spriteCrosshair.Clean();
	spriteAxes.Clean();
	spriteCross.Clean();
	spriteArrowDown.Clean();
	spriteArrowUp.Clean();

	meshApolloLEM.Clean();
	meshLS1.Clean();

	if (engine_sound){engine_sound->drop();}
	if (rcs_sound){rcs_sound->drop();}
	if (soundEngine){soundEngine->drop();}
}

//Win32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
    {
		case WM_KEYDOWN:
			switch(LOWORD(wParam))
			{
				case VK_ESCAPE:
					PostQuitMessage(WM_QUIT);
					break;

				case 0x50:
					setPauseState(!paused);
					break;
			}
			break;

		case WM_MOUSEMOVE:
			{
				POINT currMousePos;
				GetCursorPos(&currMousePos);
				D3DXVECTOR2 MouseDelta = D3DXVECTOR2(float(currMousePos.x - oldMousePos.x), float(currMousePos.y - oldMousePos.y));
				if (GetAsyncKeyState(VK_LBUTTON))
				{
					camCircularPos -= mouseSensitivity*MouseDelta.x;
					camHeightPos += mouseSensitivity*MouseDelta.y;
					if (camCircularPos > 2*D3DX_PI){camCircularPos = 0;}
					else if (camCircularPos < 0){camCircularPos = 2*D3DX_PI;}
					if (camHeightPos > D3DX_PI/2 - 0.01f){camHeightPos = D3DX_PI/2 - 0.01f;}
					else if (camHeightPos < -D3DX_PI/2 + 0.01f){camHeightPos = -D3DX_PI/2 + 0.01f;}
				}
				else if (GetAsyncKeyState(VK_RBUTTON))
				{
					camDistance += 4*mouseSensitivity*MouseDelta.y;
				}
				oldMousePos = currMousePos;
			}
			break;

        case WM_DESTROY:
            PostQuitMessage(WM_QUIT);
			break;
	}
	
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //Step 1: Registering the Window Class
	WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
    wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
    wc.lpszClassName = "WindowClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)){MessageBox(NULL, "The window could not be registered!", "Error", MB_ICONEXCLAMATION | MB_OK); return 0;}

    //Step 2: Creating the Window
    hwnd = CreateWindow("WindowClass", "LEM 3D", WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if(hwnd == NULL){MessageBox(NULL, "Window Creation Failed!", "Error", MB_OK); return 0;}

	if (!initD3D(hwnd)){MessageBox(NULL, "Direct3D failed to initialise", "Error", MB_OK); return 0;}
	initProgram();
	
	ShowWindow(hwnd, nCmdShow);

	//Initiate timing variables
	LARGE_INTEGER countFrequency;
	QueryPerformanceFrequency(&countFrequency);
	float secsPerCount = 1/(float)countFrequency.QuadPart;

	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);

    //Step 3: The Message Loop
	MSG Msg;
	while (TRUE)
    {
		while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }

        if (Msg.message == WM_QUIT)
            break;

		LARGE_INTEGER currTime;
		QueryPerformanceCounter(&currTime);
		timeDelta = (currTime.QuadPart - lastTime.QuadPart)*secsPerCount;
		if (!isDeviceLost()){renderFrame();}
		lastTime = currTime;
    }

	    cleanD3D();
	    return Msg.wParam;
}
