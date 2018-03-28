#ifndef GAMECONTROL_H
#define GAMECONTROL_H

//Camera
D3DXVECTOR3 camToCraft;
float camCircularPos = D3DX_PI/2;
float camHeightPos = 0;
float camDistance = 14;
float mouseSensitivity = 0.005f;
POINT oldMousePos;

//Lighting
D3DXVECTOR3 lightDir(0, -1, -2);
float lightAmbient = 0.1f;
float lightDiffuse = 1;

//Misc.
float timeDelta = 0;
unsigned overlayAlpha = 255;
D3DVIEWPORT9 fullViewport;
float dustFactor = 0;
TimeFadeValue dustFactorLanded(1, 0, 1);
bool paused = false;

#endif
