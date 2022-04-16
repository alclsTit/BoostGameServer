#include "stdafx.h"

boost::asio::io_service				g_io_service;

CTimerWorker						g_timer_queue(g_io_service);
CPhysicEngineWorker					g_physics_worker;

vector<CPlayerObject*>				g_clients;
vector<CNpcObject*>					g_npcs;

list<CBulletObject*>				g_bullets;
CBulletObject						*g_bullobj;

unordered_set<CStaticObject*>		g_staticobjs;
unordered_set<CRigidCubeObject*>	g_rigidobjs;
unordered_set<CMoveCubeObject*>		g_moveobjs;

CStaticObject						*g_staticobj;

FSM									*g_fsm;
CNpcObject							*g_npcobj;

queue<char>							g_pdir_queue;

bool								g_start_physics;
bool								g_start_oneGame;

map<string, HANDLE>					g_thread_handle;

void SuspendThreadFunc(const string& thread_name)
{
	std::cout << "Suspended thread Handle value : " << g_thread_handle[thread_name] << "\n";
	SuspendThread(g_thread_handle[thread_name]);
}

void ResumeThreadFunc(const std::string& thread_name)
{
	std::cout << "Resumed thread Handle value : " << g_thread_handle[thread_name] << "\n";
	ResumeThread(g_thread_handle[thread_name]);
}

int main()
{
	try
	{
		_wsetlocale(LC_ALL, L"korean");

		g_start_physics = false;
		g_start_oneGame = false;

		CAccpetPlayer server;
	}
	catch (const exception& e)
	{
		cout << "Main Thread Error: " << e.what() << endl;
	}
}