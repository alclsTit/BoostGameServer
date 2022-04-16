﻿#include "../stdafx.h"

int CStoneBulletObject::g_npc_bulletID = -1;

CBulletObject::CBulletObject(const unsigned short & master_id, const unsigned short & my_id, const Position & pos, const Rotation & rot, float bulltime, Vel3f & vel, const unsigned char & type, const Position3D & endpt, const float& degree)
{

	pp = new PhysicsPoint();

	m_OffLookvector = XMFLOAT3(0, 0, 1);
	m_OffRightvector = XMFLOAT3(1, 0, 0);

	m_pos4f = pos;
	m_orgPos4f = pos;
	m_rot4f = rot;

	m_orgpluspos = { 0.f, 0.f, 0.f, 0.f };
	m_tempangle = 0.0;

	m_masterID = master_id;

	m_mtx.lock();
	m_id = my_id;
	m_mtx.unlock();

	m_lifetime = bulltime;
	m_vel3f = vel;

	m_type = type;
	m_alive = true;

	m_savept = endpt;

	m_degree = degree;

	m_createfirst = true;

	m_after_collision = AFTER_COLLISION_EFFECT::EMPTY;		//기본은 충돌 후 파티클 생성x

	UpdateLookvector();			//degree를 이용해 변경된 회전값을 클라이언트로부터 건네받았음. 이를 이용해 lookvector, rightvector, upvector 업데이트
	UpdateUpvector();

	xmf4_pos = { m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w };
	pp->SetPosition(&xmf4_pos);
	pp->SetVelocity(vel.x, vel.y, vel.z);

	pp->SetDamping(1);
	pp->SetBounce(false);

	m_ability.curHP = 1;
	m_ability.orignHP = 1;
	m_ability.exp = 0;
	m_ability.level = 0;

	if (BULLET_TYPE::protocol_LightBullet == static_cast<int>(type))
	{
		m_ability.speed = 150;
		pp->SetHalfBox(1, 1, 1);
		pp->SetMass(0.35f);
		m_ability.attack = 10;
	}
	else if (BULLET_TYPE::protocol_HeavyBullet == static_cast<int>(type)) 
	{
		m_ability.speed = 180;
		pp->SetHalfBox(2, 2, 2);
		pp->SetMass(1.0f);
		m_ability.attack = 30;
	}
	else if (BULLET_TYPE::protocol_DiceBullet == static_cast<int>(type)) 
	{
		m_ability.speed = 50;
		pp->SetHalfBox(2, 2, 2);
		pp->SetMass(0.8f);
		m_ability.attack = 20;
	}
	
	m_bulldata.damage = m_ability.attack;
	UpdateDataForPacket();

	//--------------------------------------------------------------------------------------------

	/*
	m_bulldata.alive = m_alive;				
	m_bulldata.endpoint = m_savept;				//고정
	m_bulldata.master_id = m_masterID;			//고정
	m_bulldata.my_id = m_id;						//고정
	m_bulldata.pos4f = m_pos4f;
	m_bulldata.rot4f = m_rot4f;			
	m_bulldata.type = m_type;					//고정
	m_bulldata.vel3f = m_vel3f;					//고정
	m_bulldata.degree = m_degree;				//한번 초기화되면 안 변함 // 고정
	*/

	//--------------------------------------------------------------------------------------------
}

STC_BulletObject_Info CBulletObject::GetChangedBulletState() const
{
	STC_BulletObject_Info stc_bullet;
	stc_bullet.alive = m_alive;
	stc_bullet.degree = m_degree;
	stc_bullet.endpoint = m_savept;
	stc_bullet.master_id = m_masterID;
	stc_bullet.my_id = m_id;
	stc_bullet.pos4f = m_pos4f;
	stc_bullet.rot4f = m_rot4f;
	stc_bullet.type = m_type;
	stc_bullet.after_coll = m_after_collision;
	stc_bullet.damage = m_bulldata.damage;

	return STC_BulletObject_Info(stc_bullet);
}

void CBulletObject::UpdateDataForPacket()
{
	m_bulldata.alive = m_alive;
	m_bulldata.endpoint = m_savept;
	m_bulldata.master_id = m_masterID;
	m_bulldata.my_id = m_id;
	m_bulldata.pos4f = m_pos4f;
	m_bulldata.rot4f = m_rot4f;
	m_bulldata.type = m_type;
	m_bulldata.vel3f = m_vel3f;
	m_bulldata.degree = m_degree;
	m_bulldata.after_coll = m_after_collision;
}

void CBulletObject::AfterGravitySystem()
{
	if (m_pos4f.y <= 0 && m_alive == true)
	{
		m_alive = false;
		m_bulldata.alive = false;
	}
		
}

void CBulletObject::Tick(double deltime)
{
	m_lifetime += deltime;
	
	if (m_lifetime >= MAX_LIGHT_BULLET_TIME && BULLET_TYPE::protocol_LightBullet == static_cast<int>(m_type)) {
		m_alive = false; 
		m_bulldata.alive = false;
		return;
	}
	else if (m_lifetime >= MAX_HEAVY_BULLET_TIME && BULLET_TYPE::protocol_HeavyBullet == static_cast<int>(m_type)) {
		m_alive = false;
		m_bulldata.alive = false;
		return;
	}
	else if (m_lifetime >= MAX_DICE_BULLET_TIME && BULLET_TYPE::protocol_DiceBullet == static_cast<int>(m_type)) {
		m_alive = false;
		m_bulldata.alive = false;
		return;
	}


	xmf4_rot = { m_rot4f.x, m_rot4f.y, m_rot4f.z, m_rot4f.w };
	xmf4_pos = { m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w };

	pp->SetPosition(&xmf4_pos);
	
	pp->integrate(deltime);

	xmf4_rot = QuaternionMultiply(xmf4_rot, QuaternionRotation(m_Lookvector, MMPE_PI * deltime));
	m_rot4f = { xmf4_rot.x, xmf4_rot.y, xmf4_rot.z, xmf4_rot.w };
	
	UpdatePPosCenterPos();	//integrate를 거쳐 변경된 pp->Centerpos를 이용한 불렛 클래스가 관리하는 불렛 위치인 m_pos4f 업데이트

	UpdateLookvector();		//Lookvector와 기존의 회전값인 xmf4_rot를 이용한 QuterninMultiply에 의해 변경된 xmf4_rot, 즉 불렛회전값을 이용해 lookvector, rightvector
	UpdateUpvector();		//upvector 업데이트

	m_bulldata.pos4f = { m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w };
	m_bulldata.rot4f = { m_rot4f.x, m_rot4f.y, m_rot4f.z, m_rot4f.w };
	m_bulldata.alive = m_alive;

	//cout << "RotX: " << m_rot4f.x << "," << "RotY: " << m_rot4f.y << "," << "RotZ: " << m_rot4f.z << "," << "RotW: " << m_rot4f.w << "\n";

}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//플레이어 공격 - 라이트 불렛, 헤비 불렛, 다이스스트라이크
//npc몬스터 공격 - 스톤 불렛
//이들을 CBulletObject에서 관리

//플레이어가 발사하는 불렛타입 스킬들과 플레이어가 충돌할 경우
void CBulletObject::Collision(vector<CPlayerObject*>* clients, double deltime)
{
	for (auto iter = clients->begin(); iter != clients->end(); ++iter)
	{
		//----------------------------------------------------충돌 스킵------------//

		//1. 해당 불렛을 쏜 주인이 내 자신인 경우
		if ((*iter)->GetID() == m_masterID) continue;

		//2. 해당 불렛을 쏜 주인이 죽었을 경우 
		if (!g_clients[this->m_masterID]->GetAlive()) continue;

		//3. 상대가 죽어있을 경우
		if (!(*iter)->GetAlive()) continue;

		//----------------------------------------------------충돌 스킵------------//

		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//player 와 불렛이 부딪혔을 경우 데미지 파티클을 띄워야함
				m_after_collision = AFTER_COLLISION_EFFECT::DAMAGE_AND_BOOM;
				m_bulldata.after_coll = m_after_collision;

				(*iter)->GetDamaged(m_ability.attack);

				//해당 불렛을 맞은 플레이어가 쉴드 상태일경우 // 데미지 1 파티클이 뜨도록 클라에보내야됨
				if ((*iter)->GetShieldState())
					m_bulldata.damage = 1;
				else
					m_bulldata.damage = m_ability.attack;

				//고정된 물체가 아니면
				// 불렛을 맞은 상대가 죽었을 경우, 그 상대가 플레이어일 경우 점수를 올린다
				if ((*iter)->GetMyCurrHP() <= 0)
				{
					(*iter)->SetMyCharacterDeathCount();
					(*iter)->CalculatePlayerScoreForRanking();

					if (g_clients[this->GetBulletMasterID()]->GetAlive())
					{
						if ((*iter)->GetID() == CPlayerObject::m_topRankerID)
							g_clients[this->GetBulletMasterID()]->SetPlayerScore(TOP_RANKER_SCORE);
						else
							g_clients[this->GetBulletMasterID()]->SetPlayerScore(NORMAL_PLAYER_SCORE);

						g_clients[this->GetBulletMasterID()]->SetMyCharacterKillCount();
						g_clients[this->GetBulletMasterID()]->CalculatePlayerScoreForRanking();
					}
				}

				XMFLOAT3 cn;
				cn = XMFloat4to3(Float4Add(pp->GetPosition(), (*(*iter)->GetPhysicsPoint()).GetPosition(), false));
				cn = Float3Normalize(cn);

				(*iter)->GetPhysicsPoint()->SetBounce(true);

				pp->ResolveVelocity(*(*iter)->GetPhysicsPoint(), cn, deltime);
				(*iter)->GetPhysicsPoint()->SetBounce(false);

				m_alive = false;
				m_bulldata.alive = false;
			}
		}
	}
	
}

//플레이어가 발사하는 불렛타입 스킬들과 npc 충돌할 경우
void CBulletObject::Collision(vector<CNpcObject*>* npcs, double deltime)
{
	for (auto iter = npcs->begin(); iter != npcs->end(); ++iter)
	{
		//불렛(플레이어 불렛, npc 불렛)과 그들의 주인(npc)와 충돌x
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			//----------------------------------------------------충돌 스킵------------//

			//1. 불렛과 충돌했는데 임프가 죽은경우 스킵
			if ((*iter)->GetAlive() == false) continue;

			//2. 불렛과 충돌했는데, 불렛을 쏜 상대가 죽은경우
			if (!g_clients[m_masterID]->GetAlive()) continue;

			//3. 해당 불렛이 임프가 쏜 불렛인 경우 스킵
			if (GetObjectType() == protocol_NpcStoneBullet) continue;

			//----------------------------------------------------충돌 스킵------------//

			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//NPC 와 불렛이 부딪혔을 경우 데미지 파티클을 띄워야함
				m_after_collision = AFTER_COLLISION_EFFECT::DAMAGE_AND_BOOM;
				m_bulldata.after_coll = m_after_collision;

				(*iter)->GetDamaged(m_ability.attack);

				//npc 몬스터인 경우 -> 총알 방향을 보도록한다
				if ((*iter)->GetMyBasicPacketData().monster_type == IMP)
					(*iter)->fsm->aidata.LastPosition = this->GetCenterPos4f();
				
				m_alive = false;
				m_bulldata.alive = false;

				//플레이어가 쏜 불렛에 맞고 임프(IMP)몬스터의 체력이 0이하가 되어 죽었음
				//해당 불렛의 주인ID를 가진 플레이어가 살아있다면 점수를 올린다
				//임프(IMP) 스코어는 100
				if ((*iter)->GetMyCurrHP() <= 0)
				{
					if (g_clients[this->GetBulletMasterID()]->GetAlive())
					{
						g_clients[this->GetBulletMasterID()]->SetPlayerScore(MONSTER_IMP_SCORE);
						g_clients[this->GetBulletMasterID()]->SetMyCharacterKillCount(3);
						g_clients[this->GetBulletMasterID()]->CalculatePlayerScoreForRanking();
					}
				}

				//고정된 물체가 아니면
				XMFLOAT3 cn;
				cn = XMFloat4to3(Float4Add(pp->GetPosition(), (*(*iter)->GetPhysicsPoint()).GetPosition(), false));
				cn = Float3Normalize(cn);
				(*iter)->GetPhysicsPoint()->SetBounce(true);

				pp->ResolveVelocity(*(*iter)->GetPhysicsPoint(), cn, deltime);
				(*iter)->GetPhysicsPoint()->SetBounce(false);

			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CBulletObject::Collision(unordered_set<CStaticObject*>* sobjs, double deltime)
{

	for (auto iter = sobjs->begin(); iter != sobjs->end(); ++iter)
	{
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//StaticObject 와 불렛이 부딪혔을 경우 데미지 파티클을 띄우지 말아야함
				m_after_collision = AFTER_COLLISION_EFFECT::BOOM;
				m_bulldata.after_coll = m_after_collision;

				//고정된 물체면
				XMFLOAT3 cn;
				cn = pp->pAxis;

				pp->ResolveVelocity(*(*iter)->GetPhysicsPoint(), cn, deltime);
				(*iter)->GetPhysicsPoint()->SetBounce(false);
				m_alive = false;
				m_bulldata.alive = false;
			}
		}
	}
	
}

void CBulletObject::SetBulletRotatevalue(const XMFLOAT4 & xmf4)
{
	m_rot4f.x = xmf4.x;
	m_rot4f.y = xmf4.y;
	m_rot4f.z = xmf4.z;
	m_rot4f.w = xmf4.w;

	//m_bulldata.Rotate_status = m_rot4f;
}

CBulletObject::~CBulletObject()
{
	//if (pp != nullptr)
	//	delete pp;
}


STC_HammerSkillInfo CHammerBulletObject::GetChangedHammerBulletState() const
{
	return STC_HammerSkillInfo(m_stc_hammerInfo);
}

CHammerBulletObject::CHammerBulletObject(const unsigned short & master_id, const unsigned short & myid, const Position & pos, const Rotation & rot, int numOfHammer, const XMFLOAT4 & opp, bool isHead)
{
	m_masterID = master_id;
	m_id = myid;

	m_headBullet = isHead;

	m_type = BULLET_TYPE::protocol_HammerBullet;
	m_alive = true;
	m_createfirst = true;

	m_after_collision = AFTER_COLLISION_EFFECT::EMPTY;
	
	m_orgPos4f = pos;
	m_orgpluspos = opp;
	m_tempangle = 0.0;

	m_OffLookvector = XMFLOAT3(0, 0, 1);
	m_OffRightvector = XMFLOAT3(1, 0, 0);

	//처음 해머불렛 패킷에 담긴 rot값이 해당 해머불렛의 ori값이다 (0 , 0 , 0 , 1)
	XMFLOAT4 ori = { rot.x, rot.y, rot.z, rot.w };
	xmf4_rot = { m_rot4f.x, m_rot4f.y, m_rot4f.z, m_rot4f.w };
	xmf4_rot = QuaternionMultiply(xmf4_rot, ori);
	m_rot4f = { xmf4_rot.x, xmf4_rot.y, xmf4_rot.z, xmf4_rot.w };

	UpdateLookvector();
	UpdateUpvector();

	m_numOfHammer = numOfHammer;
	m_ability.curHP = 1;
	m_ability.orignHP = 1;
	m_ability.attack = 50;
	m_godmode = false;
	m_ability.speed = 0;
	m_lifetime = 12.0;
	
	m_pos4f = pos;
	xmf4_pos = { m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w };

	pp = new PhysicsPoint();
	pp->SetPosition(&xmf4_pos); //이 값은 항상 갱신되야한다.
	pp->SetHalfBox(2, 2, 2);	//충돌 박스의 x,y,z 크기
	pp->SetDamping(1);			//마찰력 대신 사용되는 댐핑계수. 매 틱마다 0.5배씩 속도감속
	pp->SetBounce(false);		//튕기지 않는다.
	pp->SetVelocity(0, 0, 0);	//트랜스폼으로 움직일거임
	pp->SetMass(5.35f);

	UpdateDataForPacket();
}

void CHammerBulletObject::UpdateDataForPacket()
{
	m_stc_hammerInfo.my_id = m_id;
	m_stc_hammerInfo.master_id = m_masterID;

	m_stc_hammerInfo.pos4f = m_pos4f;
	m_stc_hammerInfo.rot4f = m_rot4f;
	m_stc_hammerInfo.opp_pos4f = { m_orgpluspos.x, m_orgpluspos.y, m_orgpluspos.z, m_orgpluspos.w };


	m_stc_hammerInfo.alive = m_alive;
	m_stc_hammerInfo.damage = m_ability.attack;
	m_stc_hammerInfo.create_first = m_createfirst;
	m_stc_hammerInfo.after_coll = m_after_collision;
	m_stc_hammerInfo.headBullet = m_headBullet;
}

void CHammerBulletObject::Tick(double deltime)
{
	xmf4_rot = { m_rot4f.x, m_rot4f.y, m_rot4f.z, m_rot4f.w };
	xmf4_rot = QuaternionMultiply(xmf4_rot, QuaternionRotation(m_Upvector, MMPE_PI * deltime));
	m_rot4f = { xmf4_rot.x, xmf4_rot.y, xmf4_rot.z, xmf4_rot.w };

	XMFLOAT4 CenterPos = m_orgpluspos;

	m_tempangle += MMPE_PI / 3 * deltime;
	XMFLOAT4 tempori = QuaternionRotation(XMFLOAT3(0, 1, 0), m_tempangle);

	XMVECTOR to = XMLoadFloat4(&tempori);
	XMVECTOR cp = XMLoadFloat4(&CenterPos);

	cp = XMVector4Transform(cp, XMMatrixRotationQuaternion(to));
	XMStoreFloat4(&CenterPos, cp);
	CenterPos = Float4Add(CenterPos, XMFLOAT4(m_orgPos4f.x, m_orgPos4f.y, m_orgPos4f.z, m_orgPos4f.w));
	
	m_pos4f = { CenterPos.x, CenterPos.y, CenterPos.z, CenterPos.w };
	UpdatePhysicsCenterPos();

	m_lifetime -= deltime;
	m_orgpluspos.z += deltime * 7.0;

	if (m_lifetime <= 0)
		m_alive = false;
	
	m_stc_hammerInfo.pos4f = m_pos4f;
	m_stc_hammerInfo.rot4f = m_rot4f;
	m_stc_hammerInfo.opp_pos4f = { m_orgpluspos.x, m_orgpluspos.y, m_orgpluspos.z, m_orgpluspos.w };

	m_stc_hammerInfo.alive = m_alive;
	m_stc_hammerInfo.create_first = m_createfirst;
}

void CHammerBulletObject::AfterGravitySystem()
{
	if (m_pos4f.y <= 0 && m_alive == true)
	{
		m_alive = false;
		m_stc_hammerInfo.alive = false;
	}

}

void CHammerBulletObject::Collision(vector<CPlayerObject*>* clients, double deltime)
{
	for (auto iter = clients->begin(); iter != clients->end(); ++iter)
	{
		//클라이언트 아이디 == 불렛의 주인아이디 -> 충돌x
		if ((*iter)->GetID() == m_masterID) continue;

		if (!(*iter)->GetAlive()) continue;

		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{

				//player 와 불렛이 부딪혔을 경우 데미지 파티클을 띄워야함
				m_after_collision = AFTER_COLLISION_EFFECT::DAMAGE_AND_BOOM;
				m_stc_hammerInfo.after_coll = m_after_collision;

				(*iter)->GetDamaged(m_ability.attack);

				//해당 불렛을 맞은 플레이어가 쉴드 상태일경우 // 데미지 1 파티클이 뜨도록 클라에보내야됨
				if ((*iter)->GetShieldState())
					m_stc_hammerInfo.damage = 1;
				else
					m_stc_hammerInfo.damage = m_ability.attack;

				//고정된 물체가 아니면
				// 불렛을 맞은 상대가 죽었을 경우, 그 상대가 플레이어일 경우 점수를 올린다
				if ((*iter)->GetMyCurrHP() <= 0)
				{
					(*iter)->SetMyCharacterDeathCount();
					(*iter)->CalculatePlayerScoreForRanking();

					if (g_clients[this->GetBulletMasterID()]->GetAlive())
					{
						if ((*iter)->GetID() == CPlayerObject::m_topRankerID) 
							g_clients[this->GetBulletMasterID()]->SetPlayerScore(TOP_RANKER_SCORE);
						else
							g_clients[this->GetBulletMasterID()]->SetPlayerScore(NORMAL_PLAYER_SCORE);

						g_clients[this->GetBulletMasterID()]->SetMyCharacterKillCount();
						g_clients[this->GetBulletMasterID()]->CalculatePlayerScoreForRanking();
					}
				}

				m_alive = false;
				m_bulldata.alive = false;

				//cout << "BulletID: " << (*iter)->GetID()<< "Alive : " << static_cast<int>((*iter)->GetAlive()) << "\n";

				//해머는 다이나믹오브젝트랑 충돌시 재생성된다.
				
				if (m_numOfHammer > 0)
				{
					auto nPos4f = Float4Add(XMFLOAT4(m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w), XMFLOAT4(15, 0, 15, 0));

					m_mtx.lock();
					auto local_id = m_id;
					m_mtx.unlock();

					g_bullets.push_back(move(new CHammerBulletObject(m_masterID, ++local_id, { nPos4f.x, nPos4f.y, nPos4f.z, nPos4f.w }, { 0,0,0,1 }, m_numOfHammer - 1, XMFLOAT4(0, 0, 35, 0), false)));

					//cout << "BulletID: " << g_bullets.back()->GetID() << "Alive: " << static_cast<int>(g_bullets.back()->GetAlive()) << "\n";
					m_numOfHammer = 0;
				}
				
				
			}
		}
	}
}

void CHammerBulletObject::Collision(vector<CNpcObject*>* npcs, double deltime)
{
	for (auto iter = npcs->begin(); iter != npcs->end(); ++iter)
	{
		//불렛(플레이어 불렛, npc 불렛)과 그들의 주인(npc)와 충돌x
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			if (!(*iter)->GetAlive()) continue;

			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//NPC 와 불렛이 부딪혔을 경우 데미지 파티클을 띄워야함
				m_after_collision = AFTER_COLLISION_EFFECT::DAMAGE_AND_BOOM;
				m_stc_hammerInfo.after_coll = m_after_collision;

				(*iter)->GetDamaged(m_ability.attack);

				//npc 몬스터인 경우 -> 총알 방향을 보도록한다
				if ((*iter)->GetMyBasicPacketData().monster_type == IMP)
					(*iter)->fsm->aidata.LastPosition = this->GetCenterPos4f();


				//플레이어가 쏜 불렛에 맞고 임프(IMP)몬스터의 체력이 0이하가 되어 죽었음
				//해당 불렛의 주인ID를 가진 플레이어가 살아있다면 점수를 올린다
				//임프(IMP) 스코어는 100
				if ((*iter)->GetMyCurrHP() <= 0)
				{
					if (g_clients[this->GetBulletMasterID()]->GetAlive())
					{
						g_clients[this->GetBulletMasterID()]->SetPlayerScore(MONSTER_IMP_SCORE);
					}
				}

				m_alive = false;
				m_bulldata.alive = false;

				//해머는 다이나믹오브젝트랑 충돌시 재생성된다.
				if (m_numOfHammer > 0)
				{
					auto nPos4f = Float4Add(XMFLOAT4(m_pos4f.x, m_pos4f.y, m_pos4f.z, m_pos4f.w), XMFLOAT4(15, 0, 15, 0));
					
					m_mtx.lock();
					auto local_id = m_id;
					m_mtx.unlock();

					g_bullets.push_back(move(new CHammerBulletObject(m_masterID, ++local_id, { nPos4f.x, nPos4f.y, nPos4f.z, nPos4f.w }, { 0,0,0,1 }, m_numOfHammer - 1, XMFLOAT4(0, 0, 35, 0), false)));

					m_numOfHammer = 0;
					/*
					if (dynamic_cast<CHammerBulletObject*>(bullet))
					{
						dynamic_cast<CHammerBulletObject*>(bullet)->m_headBullet = false;
						dynamic_cast<CHammerBulletObject*>(bullet)->m_stc_hammerInfo.headBullet = false;
					}
					else { cout << "In CHammerBulletObject::Collision Error: Hammer Bullet dynamic_cast Error\n"; }
					*/


				}
			}
		}
	}
}

void CHammerBulletObject::Collision(unordered_set<CStaticObject*>* sobjs, double deltime)
{
	for (auto iter = sobjs->begin(); iter != sobjs->end(); ++iter)
	{
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//StaticObject 와 불렛이 부딪혔을 경우 데미지 파티클을 띄우지 말아야함
				m_after_collision = AFTER_COLLISION_EFFECT::BOOM;
				m_stc_hammerInfo.after_coll = m_after_collision;

				m_alive = false;
				m_bulldata.alive = false;

				//cout << "BulletID(staticobjects): " << m_id << "Alive : " << static_cast<int>(m_alive) << "\n";
			}
		}
	}
}




CStoneBulletObject::CStoneBulletObject(CNpcObject *master, const XMFLOAT4 & in_pos4f, const XMFLOAT4 & in_rot4f,  XMFLOAT4& ori, const XMFLOAT4& opp)
{
	m_createfirst = true;

	m_npc_master = master;
	m_masterID = master->GetID();

	m_type = protocol_NpcStoneBullet;
	m_after_collision = AFTER_COLLISION_EFFECT::EMPTY;

	//서버에서 npc가 생성한 불렛을 생성 -> 서버에서 따로관리 -> 패킷최대 id범위인 0xffff(65535)를 넘으면 
	//id 값을 모두 사용한것 -> id를 0으로 만들어 처음부터 id값 재사용
	if (g_npc_bulletID >= 0xffff )
		g_npc_bulletID = -1;

	m_npc_bulletID = ++g_npc_bulletID;

	m_orgpluspos = opp;
	m_tempangle = 0.0;

	m_OffLookvector = XMFLOAT3(0, 0, 1);
	m_OffRightvector = XMFLOAT3(1, 0, 0);

	xmf4_rot = { in_rot4f.x, in_rot4f.y, in_rot4f.z, in_rot4f.w };
	xmf4_rot = QuaternionMultiply(xmf4_rot, ori);
	m_rot4f = { xmf4_rot.x, xmf4_rot.y, xmf4_rot.z, xmf4_rot.w };

	UpdateLookvector();

	m_ability.orignHP = 1;
	m_ability.curHP = 1;
	m_ability.attack = 40;
	m_ability.speed = 0;
	m_godmode = false;
	m_lifetime = 10;

	pp = new PhysicsPoint();

	xmf4_pos = { in_pos4f.x, in_pos4f.y, in_pos4f.z, in_pos4f.w };
	pp->SetPosition(&xmf4_pos);
	pp->SetHalfBox(1, 1, 1);
	pp->SetDamping(1);
	pp->SetBounce(false);
	pp->SetVelocity(0, 0, 0);
	pp->SetMass(0.35f);

	UpdatePPosCenterPos();

	//패킷으로 보낼 CStornBullet 데이터 초기화
	UpdateDataForPacket();
}

/*
NPC_BulletObject_Info CBulletObject::GetChangedNPCBulletState() const
{
	//사용하지 않음. 기본으로 초기화. 상속 대상인 STONEBULLET에서 사용하기 위해 상속부모인 BULLET에 그냥 만들어둠
	NPC_BulletObject_Info stc_imp_bullet;
	stc_imp_bullet.alive = false;
	stc_imp_bullet.master_id = 0;
	stc_imp_bullet.my_id = 0;
	stc_imp_bullet.pos4f = { 0.f,0.f,0.f,0.f };
	stc_imp_bullet.rot4f = { 0.f,0.f,0.f,1.0f };
	stc_imp_bullet.create_first = true;
	stc_imp_bullet.after_coll = AFTER_COLLISION_EFFECT::EMPTY;
	stc_imp_bullet.damage = m_ability.attack;

	return NPC_BulletObject_Info(stc_imp_bullet);
}
*/

NPC_BulletObject_Info CStoneBulletObject::GetChangedNPCBulletState() const
{
	NPC_BulletObject_Info stc_imp_bullet;
	stc_imp_bullet.alive = m_alive;
	stc_imp_bullet.master_id = m_masterID;
	stc_imp_bullet.my_id = m_npc_bulletID;
	stc_imp_bullet.pos4f = m_pos4f;
	stc_imp_bullet.rot4f = m_rot4f;
	stc_imp_bullet.create_first = m_createfirst;
	stc_imp_bullet.after_coll = m_after_collision;
	stc_imp_bullet.damage = m_npc_bulldata.damage;

	return NPC_BulletObject_Info(stc_imp_bullet);
}

void CStoneBulletObject::AfterGravitySystem()
{
	if (m_pos4f.y <= 0 && m_alive == true)
	{
		m_alive = false;
		m_npc_bulldata.alive = false;
	}
}

void CStoneBulletObject::UpdateDataForPacket()
{
	m_npc_bulldata.alive = m_alive;
	m_npc_bulldata.master_id = m_masterID;
	m_npc_bulldata.my_id = m_npc_bulletID;
	m_npc_bulldata.pos4f = m_pos4f;
	m_npc_bulldata.rot4f = m_rot4f;
	m_npc_bulldata.after_coll = m_after_collision;
}

void CStoneBulletObject::Tick(double deltime)
{
	//UpdateLookvector();
	//UpdateUpvector();

	xmf4_rot = { m_rot4f.x, m_rot4f.y, m_rot4f.z, m_rot4f.w };
	xmf4_rot = QuaternionMultiply(xmf4_rot, QuaternionRotation(m_Upvector, MMPE_PI * deltime));

	xmf4_pos = m_orgpluspos;

	m_tempangle += MMPE_PI / 4 * deltime;
	XMFLOAT4 tempori = QuaternionRotation(XMFLOAT3(0, 1, 0), m_tempangle);

	XMVECTOR to = XMLoadFloat4(&tempori);
	XMVECTOR cp = XMLoadFloat4(&xmf4_pos);

	cp = XMVector4Transform(cp, XMMatrixRotationQuaternion(to));
	XMStoreFloat4(&xmf4_pos, cp);
	xmf4_pos = Float4Add(xmf4_pos, m_npc_master->GetCenterPos4f());

	m_pos4f = { xmf4_pos.x, xmf4_pos.y, xmf4_pos.z, xmf4_pos.w };
	UpdateLookvector();

	m_lifetime -= deltime;

	if (m_lifetime <= 0)
		m_alive = false;
	
}

void CStoneBulletObject::Collision(vector<CPlayerObject*>* clients, double deltime)
{
	for (auto iter = clients->begin(); iter != clients->end(); ++iter)
	{
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{
			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//Player 와 임프의 스톤불렛이 부딪혔을 경우 데미지 파티클을 띄워야함
				m_after_collision = AFTER_COLLISION_EFFECT::DAMAGE;
				m_npc_bulldata.after_coll = m_after_collision;

				(*iter)->GetDamaged(m_ability.attack);

				if ((*iter)->GetShieldState())
					m_npc_bulldata.damage = 1;
				else
					m_npc_bulldata.damage = m_ability.attack;

				if ((*iter)->GetMyCurrHP() <= 0)
				{
					(*iter)->SetMyCharacterDeathCount();
					(*iter)->CalculatePlayerScoreForRanking();
				}


				m_alive = false;
			}
		}
	}
}

void CStoneBulletObject::Collision(vector<CNpcObject*>* npcs, double deltime)
{
	//하는 일 없음
	return;
}

void CStoneBulletObject::Collision(unordered_set<CStaticObject*>* sobjs, double deltime)
{
	for (auto iter = sobjs->begin(); iter != sobjs->end(); ++iter)
	{
		if ((*iter)->GetPhysicsPoint() != nullptr)
		{

			bool test = pp->CollisionTest(*(*iter)->GetPhysicsPoint(), m_Lookvector, m_Rightvector, m_Upvector,
				(*iter)->GetLookVector(), (*iter)->GetRightVector(), (*iter)->GetUpVector());

			if (test)
			{
				//StaticObject 와 임프의 스톤불렛이 부딪혔을 경우 데미지 파티클을 띄우지 말아야함
				m_after_collision = AFTER_COLLISION_EFFECT::EMPTY;
				m_npc_bulldata.after_coll = m_after_collision;

				m_alive = false;
			
			}
			
		}
	}
}

