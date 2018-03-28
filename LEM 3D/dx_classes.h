#ifndef DXCLASSES_H
#define DXCLASSES_H

//Camera
class Camera
{
private:
	float yFov, aspectRatio, nearClip, farClip;
	D3DXVECTOR3 upGlobal;
public:
	D3DXVECTOR3 frustumPlanePoints[6], frustumPlaneNormals[6];
	D3DXMATRIX matView, matProjection;
	D3DXVECTOR3 pos, lookAt;

	Camera(){};

	void UpdateView(D3DXVECTOR3 p, D3DXVECTOR3 l, D3DXVECTOR3 u)
	{
		pos = p;
		lookAt = l;
		upGlobal = u;
		D3DXMatrixLookAtLH(&matView, &pos, &lookAt, &upGlobal);
	}

	void UpdateProjection(float fy, float ar, float nc, float fc)
	{
		yFov = fy;
		aspectRatio = ar;
		nearClip = nc;
		farClip = fc;
		D3DXMatrixPerspectiveFovLH(&matProjection, yFov, aspectRatio, nearClip, farClip);
	}

	void UpdateFrustum()
	{
		D3DXVECTOR3 v_temp;

		D3DXVECTOR3 look;
		D3DXVec3Normalize(&look, &(lookAt - pos));
		D3DXVECTOR3 right;
		D3DXVec3Cross(&v_temp, &look, &upGlobal);
		D3DXVec3Normalize(&right, &v_temp);
		D3DXVECTOR3 up;
		D3DXVec3Cross(&v_temp, &right, &look);
		D3DXVec3Normalize(&up, &v_temp);

		float halfHeightNear = nearClip*tan(yFov/2);
		float halfWidthNear = halfHeightNear*aspectRatio;

		//Near plane
		frustumPlanePoints[0] = pos + look*nearClip;
		frustumPlaneNormals[0] = look;
		
		//Far plane
		frustumPlanePoints[1] = pos + look*farClip;
		frustumPlaneNormals[1] = -look;

		//Right plane
		frustumPlanePoints[2] = pos + look*nearClip + right*halfWidthNear;
		D3DXVec3Cross(&v_temp, &up, &(frustumPlanePoints[2] - pos));
		D3DXVec3Normalize(&v_temp, &v_temp);
		frustumPlaneNormals[2] = v_temp;

		//Left plane
		frustumPlanePoints[3] = pos + look*nearClip - right*halfWidthNear;
		D3DXVec3Cross(&v_temp, &(frustumPlanePoints[3] - pos), &up);
		D3DXVec3Normalize(&v_temp, &v_temp);
		frustumPlaneNormals[3] = v_temp;

		//Top plane
		frustumPlanePoints[4] = pos + look*nearClip + up*halfHeightNear;
		D3DXVec3Cross(&v_temp, &(frustumPlanePoints[4] - pos), &right);
		D3DXVec3Normalize(&v_temp, &v_temp);
		frustumPlaneNormals[4] = v_temp;

		//Bottom plane
		frustumPlanePoints[5] = pos + look*nearClip - up*halfHeightNear;
		D3DXVec3Cross(&v_temp, &right, &(frustumPlanePoints[5] - pos));
		D3DXVec3Normalize(&v_temp, &v_temp);
		frustumPlaneNormals[5] = v_temp;
	}
};
Camera mainCam;

//Mesh
class MeshObjectTNS
{
private:
	DWORD numMaterials;
	LPD3DXBUFFER materialBuffer;
	LPD3DXMATERIAL meshMaterials;
	LPDIRECT3DTEXTURE9 *meshTextures;
	LPDIRECT3DTEXTURE9 *meshNormals;
	LPDIRECT3DTEXTURE9 *meshSpeculars;
	float boundingRadius;
public:
	LPD3DXMESH objectMesh;
	string filename;

	MeshObjectTNS(string name)
	{filename = name;};
	
	bool Load()
	{
		if (FAILED(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_SYSTEMMEM, d3ddev, NULL, &materialBuffer, NULL, &numMaterials, &objectMesh))){return false;}
		
		BYTE* v = 0;
		if (FAILED(objectMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&v))){return false;}
		if (FAILED(D3DXComputeBoundingSphere((D3DXVECTOR3*)v, objectMesh->GetNumVertices(), D3DXGetFVFVertexSize(objectMesh->GetFVF()), &D3DXVECTOR3(0, 0, 0), &boundingRadius))){return false;}
		if (FAILED(objectMesh->UnlockVertexBuffer())){return false;}

		meshMaterials = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();
		meshTextures = new LPDIRECT3DTEXTURE9[numMaterials];
		meshNormals = new LPDIRECT3DTEXTURE9[numMaterials];
		meshSpeculars = new LPDIRECT3DTEXTURE9[numMaterials];

		for (DWORD i = 0; i < numMaterials; i++)
		{
			meshTextures[i] = NULL;
			meshNormals[i] = NULL;
			meshSpeculars[i] = NULL;

			if (meshMaterials[i].pTextureFilename)
			{
				string texture_path_n = string(meshMaterials[i].pTextureFilename);
				string texture_path_s = string(meshMaterials[i].pTextureFilename);
				texture_path_n.insert(texture_path_n.find("."), "_n");
				texture_path_s.insert(texture_path_s.find("."), "_s");

				if (FAILED(D3DXCreateTextureFromFile(d3ddev, meshMaterials[i].pTextureFilename, &meshTextures[i]))){return false;}				
				if (FAILED(D3DXCreateTextureFromFile(d3ddev, texture_path_n.c_str(), &meshNormals[i])))
				{
					if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/default/default_n.jpg", &meshNormals[i]))){return false;}
				}
				if (FAILED(D3DXCreateTextureFromFile(d3ddev, texture_path_s.c_str(), &meshSpeculars[i])))
				{
					if (FAILED(D3DXCreateTextureFromFile(d3ddev, "textures/default/default_s.jpg", &meshSpeculars[i]))){return false;}
				}	
			}
		}

		return true;
	}
	
	bool ComputeTangentSpace()
	{
		D3DVERTEXELEMENT9 elements[] =
		{
			{0, sizeof(float)*0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, sizeof(float)*3, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
			{0, sizeof(float)*6, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, sizeof(float)*9, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
			{0, sizeof(float)*12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
			D3DDECL_END()
		};

		LPD3DXMESH tempMesh = NULL;
		if (FAILED(objectMesh->CloneMesh(D3DXMESH_MANAGED, elements, d3ddev, &tempMesh)))
		{SAFE_RELEASE(&tempMesh); return false;}

		SAFE_RELEASE(&objectMesh);
		objectMesh = tempMesh;

		if (FAILED(D3DXComputeTangentFrameEx(objectMesh, D3DDECLUSAGE_TEXCOORD, 0, D3DDECLUSAGE_BINORMAL, 0, D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_NORMAL, 0, 0, 0, 0.01f, 0.25f, 0.01f, &objectMesh, 0)))
		{return false;}

		SAFE_RELEASE(&tempMesh);

		return true;
	}

	void RenderGlobal(D3DXVECTOR3 pos, D3DXVECTOR3 rot, bool cull)
	{
		if (cull)
		{
			for (unsigned i = 0; i < 6; i++)
			{
				if (D3DXVec3Dot(&(pos - mainCam.frustumPlanePoints[i]), &mainCam.frustumPlaneNormals[i]) < -boundingRadius)
				{return;}
			}
		}

		D3DXMATRIX translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, pos.x, pos.y, pos.z);
		D3DXMATRIX xRotate, yRotate, zRotate;
		D3DXMatrixRotationX(&xRotate, rot.x); D3DXMatrixRotationY(&yRotate, rot.y); D3DXMatrixRotationZ(&zRotate, rot.z);
		D3DXMATRIX worldMatrix = xRotate*yRotate*zRotate*translationMatrix;

		globalLightingEffect->SetMatrix("m_World", &worldMatrix);
		globalLightingEffect->SetMatrix("m_WorldViewProjection", &(worldMatrix*mainCam.matView*mainCam.matProjection));
		globalLightingEffect->SetMatrix("m_LightWorldViewProjection", &(worldMatrix*matLightView*matLightProjection));
		globalLightingEffect->SetMatrix("m_DustWorldViewProjection", &(worldMatrix*matDustView*matDustProjection));
		globalLightingEffect->CommitChanges();

		for (DWORD i = 0; i < numMaterials; i++)
		{
			globalLightingEffect->SetTexture("t_Texture", meshTextures[i]);
			globalLightingEffect->SetTexture("t_TextureN", meshNormals[i]);
			globalLightingEffect->SetTexture("t_TextureS", meshSpeculars[i]);
			globalLightingEffect->SetValue("f_Reflectivity", &meshMaterials[i].MatD3D.Specular.r, sizeof(float));
			globalLightingEffect->SetValue("f_SpecularPower", &meshMaterials[i].MatD3D.Power, sizeof(float));

			globalLightingEffect->CommitChanges();
			objectMesh->DrawSubset(i);
		}
	}

	void RenderCustomMatrix(D3DXVECTOR3 pos, D3DXMATRIX orientation, bool cull)
	{
		if (cull)
		{
			for (unsigned i = 0; i < 6; i++)
			{
				if (D3DXVec3Dot(&(pos - mainCam.frustumPlanePoints[i]), &mainCam.frustumPlaneNormals[i]) < -boundingRadius)
				{return;}
			}
		}

		D3DXMATRIX translationMatrix;
		D3DXMatrixTranslation(&translationMatrix, pos.x, pos.y, pos.z);
		D3DXMATRIX worldMatrix = orientation*translationMatrix;

		globalLightingEffect->SetMatrix("m_World", &worldMatrix);
		globalLightingEffect->SetMatrix("m_WorldViewProjection", &(worldMatrix*mainCam.matView*mainCam.matProjection));
		globalLightingEffect->SetMatrix("m_LightWorldViewProjection", &(worldMatrix*matLightView*matLightProjection));
		globalLightingEffect->SetMatrix("m_DustWorldViewProjection", &(worldMatrix*matDustView*matDustProjection));
		globalLightingEffect->CommitChanges();

		for (DWORD i = 0; i < numMaterials; i++)
		{
			globalLightingEffect->SetTexture("t_Texture", meshTextures[i]);
			globalLightingEffect->SetTexture("t_TextureN", meshNormals[i]);
			globalLightingEffect->SetTexture("t_TextureS", meshSpeculars[i]);
			globalLightingEffect->SetValue("f_Reflectivity", &meshMaterials[i].MatD3D.Specular.r, sizeof(float));
			globalLightingEffect->SetValue("f_SpecularPower", &meshMaterials[i].MatD3D.Power, sizeof(float));

			globalLightingEffect->CommitChanges();
			objectMesh->DrawSubset(i);
		}
	}

	void Clean()
	{
		SAFE_RELEASE(&objectMesh);
		SAFE_RELEASE(&materialBuffer);
		for (DWORD i = 0; i < numMaterials; i++)
		{
			SAFE_RELEASE(&meshTextures[i]);
			SAFE_RELEASE(&meshNormals[i]);
			SAFE_RELEASE(&meshSpeculars[i]);
		}
		delete[] meshTextures;
		delete[] meshNormals;
		delete[] meshSpeculars;
	}
};

//Sprite
class SpriteObject
{
private:
	LPD3DXSPRITE objectSprite;
	LPDIRECT3DTEXTURE9 spriteTexture;
public:
	string texturepath;
	
	SpriteObject(string path)
	{texturepath = path;};

	bool CreateResources()
	{
		if (FAILED(D3DXCreateTextureFromFileEx(d3ddev, texturepath.c_str(), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, 0, NULL, NULL, &spriteTexture))){return false;}
		if (FAILED(D3DXCreateSprite(d3ddev, &objectSprite))){return false;}
		
		return true;
	}

	bool Render(RECT* rect, D3DXVECTOR3 centre, D3DXVECTOR3 pos, D3DXVECTOR2 scaling, unsigned alpha)
	{
		if (FAILED(objectSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DO_NOT_ADDREF_TEXTURE | D3DXSPRITE_SORT_TEXTURE))){return false;}
		
		D3DXMATRIX matTransform;
		D3DXMatrixTransformation2D(&matTransform, &D3DXVECTOR2(pos.x, pos.y), 0, &scaling, NULL, 0, NULL);
		objectSprite->SetTransform(&matTransform);
		
		if (FAILED(objectSprite->Draw(spriteTexture, rect, &centre, &pos, D3DCOLOR_ARGB(alpha, 255, 255, 255)))){return false;}
		
		if (FAILED(objectSprite->End())){return false;}
		
		return true;
	}

	bool LostDevice()
	{
		if (FAILED(objectSprite->OnLostDevice())){return false;}
		
		return true;
	}

	bool ResetDevice()
	{
		if (FAILED(objectSprite->OnResetDevice())){return false;}
		
		return true;
	}

	void Clean()
	{
		SAFE_RELEASE(&objectSprite);
		SAFE_RELEASE(&spriteTexture);
	}
};
class GUIButton
{
private:
	LPD3DXSPRITE buttonSprite;
	LPDIRECT3DTEXTURE9 buttonTexture;
public:
	D3DCOLOR currentColour;
	D3DCOLOR mouseOverColour;
	string texturepath;

	GUIButton(string path, D3DCOLOR mouse_over_colour)
	{texturepath = path; mouseOverColour = mouse_over_colour; currentColour = D3DCOLOR_XRGB(255, 255, 255);};

	bool CreateResources()
	{
		if (FAILED(D3DXCreateTextureFromFileEx(d3ddev, texturepath.c_str(), D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_DEFAULT, 0, NULL, NULL, &buttonTexture))){return false;}
		if (FAILED(D3DXCreateSprite(d3ddev, &buttonSprite))){return false;}
		
		return true;
	}

	bool Render(D3DXVECTOR3 pos, D3DXVECTOR2 scaling)
	{
		if (FAILED(buttonSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DO_NOT_ADDREF_TEXTURE | D3DXSPRITE_SORT_TEXTURE))){return false;}
		
		D3DXMATRIX matTransform;
		D3DXMatrixTransformation2D(&matTransform, &D3DXVECTOR2(pos.x, pos.y), 0, &scaling, NULL, 0, NULL);
		buttonSprite->SetTransform(&matTransform);
		
		if (FAILED(buttonSprite->Draw(buttonTexture, NULL, NULL, &pos, currentColour))){return false;}

		if (FAILED(buttonSprite->End())){return false;}
		
		return true;
	}

	bool CheckMouseOver(D3DXVECTOR2 pos, D3DXVECTOR2 dimensions)
	{
		POINT mousePos;
		GetCursorPos(&mousePos);
		
		if (mousePos.x >= pos.x && mousePos.x <= pos.x + dimensions.x && mousePos.y >= pos.y && mousePos.y <= pos.y + dimensions.y){return true;}

		return false;
	}

	bool LostDevice()
	{
		if (FAILED(buttonSprite->OnLostDevice())){return false;}
		
		return true;
	}

	bool ResetDevice()
	{
		if (FAILED(buttonSprite->OnResetDevice())){return false;}

		return true;
	}

	void Clean()
	{
		SAFE_RELEASE(&buttonSprite);
		SAFE_RELEASE(&buttonTexture);
	}
};

//Texture
class DrawableTex2D
{
private:
	LPDIRECT3DSURFACE9 drawSurface;
public:
	LPDIRECT3DTEXTURE9 drawTexture;
	
	DrawableTex2D(){};

	bool CreateResources(unsigned width, unsigned height, D3DFORMAT format)
	{
		if (FAILED(d3ddev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &drawTexture, NULL))){return false;}
		if (FAILED(drawTexture->GetSurfaceLevel(0, &drawSurface))){return false;}
		
		return true;
	}

	bool SetAsTarget()
	{
		if (FAILED(d3ddev->SetRenderTarget(0, drawSurface))){return false;}
		
		return true;
	}

	void DeleteResources()
	{
		SAFE_RELEASE(&drawTexture);
		SAFE_RELEASE(&drawSurface);
	}
};
class DrawableTex2DDepth
{
private:
	LPDIRECT3DSURFACE9 drawSurface;
	LPDIRECT3DSURFACE9 depthSurface;
public:
	LPDIRECT3DTEXTURE9 drawTexture;	
	
	DrawableTex2DDepth(){};	

	bool CreateResources(unsigned width, unsigned height, D3DFORMAT format, D3DFORMAT depth_format)
	{
		if (FAILED(d3ddev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &drawTexture, NULL))){return false;}
		if (FAILED(drawTexture->GetSurfaceLevel(0, &drawSurface))){return false;}
		if (FAILED(d3ddev->CreateDepthStencilSurface(width, height, depth_format, D3DMULTISAMPLE_NONE, 0, TRUE, &depthSurface, NULL))){return false;}
		
		return true;
	}

	bool SetAsTarget()
	{
		if (FAILED(d3ddev->SetRenderTarget(0, drawSurface))){return false;}
		if (FAILED(d3ddev->SetDepthStencilSurface(depthSurface))){return false;}

		return true;
	}

	void DeleteResources()
	{
		SAFE_RELEASE(&drawTexture);
		SAFE_RELEASE(&drawSurface);
		SAFE_RELEASE(&depthSurface);
	}
};

#endif
