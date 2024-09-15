#ifndef PHYSICS_H
#define PHYSICS_H

//Misc.
float dampedThrusterProfile(float input, float control, float width, float target)
{
	return input > target + width ? control : input < target - width ? -control : control*sin(D3DX_PI/2*(input - target)/width);
}

//Classes/structs
class Spacecraft
{
public:
	D3DXVECTOR3 pos, prevPos, vel, acc, angvel, angacc;
	D3DXVECTOR3 left, up, back;
	D3DXMATRIX orientation;
	MeshObjectTNS *mesh;
	vector <D3DXVECTOR3> contactPoints;
	float engineThrustAcc, rcsTorqueAcc;
	float thrust, fuel;
	bool landed;

	Spacecraft(float engine_acc, float rcs_acc, MeshObjectTNS *msh)
	{engineThrustAcc = engine_acc; rcsTorqueAcc = rcs_acc; mesh = msh; contactPoints.clear();};

	bool CheckAboveMesh(MeshObjectTNS *msh)
	{
		for (unsigned i = 0; i < contactPoints.size(); i++)
		{
			BOOL aboveMesh;
			D3DXVECTOR3 rayOrigin = contactPoints[i];
			D3DXVec3TransformCoord(&rayOrigin, &rayOrigin, &orientation);
			rayOrigin += pos;
			D3DXIntersect(msh->objectMesh, &rayOrigin, &D3DXVECTOR3(0, -1, 0), &aboveMesh, NULL, NULL, NULL, NULL, NULL, NULL);
			if (!aboveMesh){return false;}
		}

		return true;
	}

	void UpdatePhysics(float dt)
	{
		//Angular mechanics
		angacc = D3DXVECTOR3(0, 0, 0);
		if (GetAsyncKeyState(VK_NUMPAD3)){angacc.x = rcsTorqueAcc;}
		if (GetAsyncKeyState(VK_NUMPAD1)){angacc.x = -rcsTorqueAcc;}
		if (GetAsyncKeyState(VK_NUMPAD8)){angacc.y = -rcsTorqueAcc;}
		if (GetAsyncKeyState(VK_NUMPAD2)){angacc.y = rcsTorqueAcc;}
		if (GetAsyncKeyState(VK_NUMPAD4)){angacc.z = -rcsTorqueAcc;}
		if (GetAsyncKeyState(VK_NUMPAD6)){angacc.z = rcsTorqueAcc;}
		
		if (GetAsyncKeyState(VK_NUMPAD5))
		{
			angacc.x = dampedThrusterProfile(angvel.x, -rcsTorqueAcc, 0.01f, 0);
			angacc.y = dampedThrusterProfile(angvel.y, -rcsTorqueAcc, 0.01f, 0);
			angacc.z = dampedThrusterProfile(angvel.z, -rcsTorqueAcc, 0.01f, 0);
		}

		D3DXVECTOR3 orientationDelta = angvel*dt + angacc*dt*dt/2;
		angvel += angacc*dt;

		//Reorthogonalise orientation vectors
		D3DXVec3Normalize(&up, &up);
		D3DXVec3Cross(&left, &up, &back);
		D3DXVec3Normalize(&left, &left);
		D3DXVec3Cross(&back, &left, &up);
		D3DXVec3Normalize(&back, &back);

		//Update orientation
		D3DXMATRIX matYaw;
		D3DXMatrixRotationAxis(&matYaw, &up, orientationDelta.x);
		D3DXVec3TransformCoord(&back, &back, &matYaw);
		D3DXVec3TransformCoord(&left, &left, &matYaw);
		D3DXMATRIX matPitch;
		D3DXMatrixRotationAxis(&matPitch, &left, orientationDelta.y);
		D3DXVec3TransformCoord(&back, &back, &matPitch);
		D3DXVec3TransformCoord(&up, &up, &matPitch);
		D3DXMATRIX matRoll;
		D3DXMatrixRotationAxis(&matRoll, &back, orientationDelta.z);
		D3DXVec3TransformCoord(&left, &left, &matRoll);
		D3DXVec3TransformCoord(&up, &up, &matRoll);

		orientation *= matYaw*matPitch*matRoll;

		//Retrieve orientation vectors from the matrix
		left = D3DXVECTOR3(orientation._11, orientation._12, orientation._13);
		up = D3DXVECTOR3(orientation._21, orientation._22, orientation._23);
		back = D3DXVECTOR3(orientation._31, orientation._32, orientation._33);

		//Linear mechanics/engine
		if (GetAsyncKeyState(VK_UP)){thrust += dt/0.08f;}
		if (GetAsyncKeyState(VK_DOWN)){thrust -= dt/0.08f;}
		thrust = thrust > 100 ? 100 : thrust < 0 ? 0 : thrust;
		fuel -= 0.01f*thrust*dt;
		if (fuel <= 0){fuel = 0; thrust = 0;}

		prevPos = pos;
		D3DXVECTOR3 acc = D3DXVECTOR3(0, -float(4.901e12)/((pos.y + 1738000)*(pos.y + 1738000)), 0) + thrust/100*engineThrustAcc*up;
		pos += vel*dt + acc*dt*dt/2;
		vel += acc*dt;
	}

	void Reset(D3DXVECTOR3 init_pos, D3DXVECTOR3 init_vel, float init_thrust)
	{
		angvel = D3DXVECTOR3(0, 0, 0);
		pos = init_pos;
		prevPos = init_pos;
		vel = init_vel;	
		D3DXMatrixIdentity(&orientation);
		left = D3DXVECTOR3(orientation._11, orientation._12, orientation._13);
		up = D3DXVECTOR3(orientation._21, orientation._22, orientation._23);
		back = D3DXVECTOR3(orientation._31, orientation._32, orientation._33);	
		thrust = init_thrust;
		fuel = 100;
		landed = false;
	}
};
class TimeFadeValue
{
private:
	float duration, elapsed;
	float startValue, endValue;
public:

	TimeFadeValue(float start_val, float end_val, float dur)
	{startValue = start_val; endValue = end_val; duration = dur; elapsed = 0;};

	float GetValue(float dt)
	{
		elapsed += dt;
		elapsed = elapsed > duration ? duration : elapsed;
		return startValue + elapsed/duration*(endValue - startValue);
	}

	void Reset()
	{
		elapsed = 0;
	}
};

#endif
