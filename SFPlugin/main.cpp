#include <Windows.h>
#include "main.h"
#include "xorstr.h"
#include <algorithm>
#include <array>
#include <list>

SAMPFUNCS* SF = new SAMPFUNCS();

bool state = false;
std::list <int> shootObjects;
std::list <int> buffershootObjects;
DWORD lastCreateTirObject = 0;
DWORD lastShoot = 0;
int lastWeapon = 0;

bool canShoot = true;
bool LBM = false;
bool prodlevayu = false;

int tirModels[] = { 1588, 1589, 1590, 1591, 1592 };
int weaponIdsDialog[] = { 22, 23, 24, 25, 28, 29, 30, 31 };

bool __stdcall onRecievePacket(stRakNetHookParams* param);
bool __stdcall onSendPacket(stRakNetHookParams* param);
bool __stdcall onRecvRpc(stRakNetHookParams* param);
bool __stdcall onSendRpc(stRakNetHookParams* param);

bool isGoodWeapon(int id);
bool isTir(int model);
int GetWeaponID();
void leftButtonSync();
bool sendBulletData(int i);
void start();
float randF(float low, float high);
void EmulKey(int id, bool sKeys);
int gunWait(int weaponId);


void __stdcall mainloop()
{
	static bool initialized = false;
	if (!initialized)
	{
		if (GAME && GAME->GetSystemState() == eSystemState::GS_PLAYING_GAME && SF->getSAMP()->IsInitialized())
		{
			initialized = true;
			SF->getSAMP()->getChat()->AddChatMessage(-1, xorstr("[Author: https://www.youtube.com/@D.V.AkeGGa]: Бот на тир был успешно загружен"));

			SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_OUTCOMING_PACKET, onSendPacket);
			SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_OUTCOMING_RPC, onSendRpc);
			SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_INCOMING_PACKET, onRecievePacket);
			SF->getRakNet()->registerRakNetCallback(RAKHOOK_TYPE_INCOMING_RPC, onRecvRpc);

			SF->getSAMP()->registerChatCommand("start_tir", [](std::string arg) {
				start();
				});
		}
	}

	if (initialized) {
		if (state) {
			if (isGoodWeapon(GetWeaponID())) {
				lastWeapon = GetWeaponID();
			}
			if (buffershootObjects.size() != 0) {
				if (GetTickCount() - 1000 > lastCreateTirObject) {
					shootObjects.merge(buffershootObjects);
					buffershootObjects.clear();
				}
			}

			if (shootObjects.size() > 0 && canShoot) {
				int objId = shootObjects.front();
				if (SF->getSAMP()->getInfo()->pPools->pObject->iIsListed[objId] == -1)
					shootObjects.pop_front();
				else {
					stObject* myObj = SF->getSAMP()->getInfo()->pPools->pObject->object[objId];
					float mY = PEDSELF->GetPosition()->fY;

					if (myObj->fPos[1] < (mY + 0.85) && myObj->fPos[1] > (mY - 0.65)) {
						if (isGoodWeapon(GetWeaponID())) {
							leftButtonSync();
							if (sendBulletData(shootObjects.front())) {
								canShoot = false;
								lastShoot = GetTickCount();
								CWeapon* myWeapon = PEDSELF->GetWeapon(PEDSELF->GetCurrentWeaponSlot());
								myWeapon->SetAmmoTotal(myWeapon->GetAmmoTotal() - 1);
								SF->getSAMP()->getChat()->AddChatMessage(-1, xorstr("[Author: https://www.youtube.com/@D.V.AkeGGa]: Стреляю в: %d"), shootObjects.front());
								shootObjects.pop_front();
							}
						}
					}
					else
					{
						shootObjects.pop_front();
					}
				}
			}

			if (GetTickCount() - gunWait(GetWeaponID()) > lastShoot) {
				canShoot = true;
			}
		}
	}
}

void start()
{
	state = !state;
	SF->getSAMP()->getChat()->AddChatMessage(-1, xorstr("[Author: https://www.youtube.com/@D.V.AkeGGa]: Бот на тир был успешно %s"), state ? "включен" : "выключен");
	shootObjects.clear();
	canShoot = true;
	LBM = false;
}

void leftButtonSync()
{
	LBM = true;
	SF->getSAMP()->getPlayers()->pLocalPlayer->ForceSendOnfootSync();
	LBM = false;
}

bool isGoodWeapon(int id)
{
	return (id >= 22 && id <= 25) || (id >= 28 && id <= 31);
}

bool isTir(int model)
{
	return std::find(std::begin(tirModels), std::end(tirModels), model) != std::end(tirModels);
}


int GetWeaponID()
{
	return PEDSELF->GetWeapon(PEDSELF->GetCurrentWeaponSlot())->GetType();
}

bool __stdcall onSendPacket(stRakNetHookParams* param)
{
	if (param->packetId == PacketEnumeration::ID_PLAYER_SYNC) {
		if (state && isGoodWeapon(GetWeaponID())) {
			lastWeapon = GetWeaponID();
			stOnFootData data;
			memset(&data, 0, sizeof(stOnFootData));
			byte packet;

			param->bitStream->ResetReadPointer();
			param->bitStream->Read(packet);
			param->bitStream->Read((PCHAR)&data, sizeof(stOnFootData));
			param->bitStream->ResetReadPointer();
			// data.controllerState.rightShoulder1 = aim
			// data.controllerState.buttonCircle = secondaryFire_shoot
			data.stSampKeys.keys_aim = 1;

			if (LBM) {
				data.stSampKeys.keys_secondaryFire__shoot = 1;
			}
			BitStream Onfoot;
			Onfoot.ResetWritePointer();
			Onfoot.Write(packet);
			Onfoot.Write((PCHAR)&data, sizeof(data));
			SF->getRakNet()->SendPacket(&Onfoot);
			return false;
		}
	}
	return true;
}

bool __stdcall onRecievePacket(stRakNetHookParams* param) {
	return true;
}

bool __stdcall onSendRpc(stRakNetHookParams* param) {
	return true;
}

bool __stdcall onRecvRpc(stRakNetHookParams* param) {
	if (state) {
		if (param->packetId == ScriptRPCEnumeration::RPC_ScrShowDialog && prodlevayu) {
			INT16 dialogId;
			param->bitStream->ResetReadPointer();
			param->bitStream->Read(dialogId);

			if (dialogId == 8173) {
				if (isGoodWeapon(lastWeapon)) {
					for (int i = 0; i < 8; i++) {
						if (weaponIdsDialog[i] == lastWeapon) {
							SF->getSAMP()->sendDialogResponse(8173, 1, i, "");
							prodlevayu = false;
							return false;
						}
					}
				}
			}
		}

		if (param->packetId == ScriptRPCEnumeration::RPC_ScrClientMessage) {
			UINT32 dColor;
			UINT32 dMessageLength;
			char Message[576];

			param->bitStream->ResetReadPointer();
			param->bitStream->Read(dColor);
			param->bitStream->Read(dMessageLength);
			param->bitStream->Read(Message, dMessageLength);
			std::string strdialogText(Message);
			if (strdialogText.find("кабинка была освобождена") != std::string::npos && strdialogText.find("что у вас зако") != std::string::npos) {
				prodlevayu = true;
				EmulKey(128, false);
			}
		}

		if (param->packetId == ScriptRPCEnumeration::RPC_ScrCreateObject) {
			uint16_t objectId;
			uint16_t modelId;


			param->bitStream->ResetReadPointer();
			param->bitStream->Read(objectId);
			param->bitStream->Read(modelId);
			if (isTir(modelId)) {
				buffershootObjects.push_back(objectId);
				lastCreateTirObject = GetTickCount();
			}
		}

		if (param->packetId == ScriptRPCEnumeration::RPC_ScrDestroyObject) {
			uint16_t objectId;

			param->bitStream->ResetReadPointer();
			param->bitStream->Read(objectId);

			shootObjects.remove(objectId);
		}

		if (param->packetId == ScriptRPCEnumeration::RPC_ScrTogglePlayerControllable || param->packetId == RPC_ScrSetPlayerArmedWeapon) {
			return !state;
		}
	}

	return true;
}


void EmulKey(int id, bool sKeys)
{
	stOnFootData sync;
	memset(&sync, 0, sizeof(stOnFootData));
	sync = SF->getSAMP()->getPlayers()->pLocalPlayer->onFootData;
	if (sKeys) {
		sync.sKeys = id;
	}
	else
	{
		sync.byteCurrentWeapon = id;
	}
	BitStream bsActorSync;
	bsActorSync.Write((BYTE)ID_PLAYER_SYNC);
	bsActorSync.Write((PCHAR)&sync, sizeof(stOnFootData));
	SF->getRakNet()->SendPacket(&bsActorSync);
	memset(&bsActorSync, 0, sizeof(BitStream));
	sync.sKeys = 0;
	bsActorSync.Write((BYTE)ID_PLAYER_SYNC);
	bsActorSync.Write((PCHAR)&sync, sizeof(stOnFootData));
	SF->getRakNet()->SendPacket(&bsActorSync);
}


bool sendBulletData(int i)
{
	stObject* pObject = SF->getSAMP()->getInfo()->pPools->pObject->object[i];
	if (!pObject) return false;
	stBulletData sync;
	ZeroMemory(&sync, sizeof(stBulletData));

	sync.sTargetID = i;

	sync.fOrigin[0] = PEDSELF->GetPosition()->fX + randF(-0.3, 0.3);
	sync.fOrigin[1] = PEDSELF->GetPosition()->fY + randF(-0.3, 0.3);
	sync.fOrigin[2] = PEDSELF->GetPosition()->fZ + randF(-0.3, 0.3);

	sync.fTarget[0] = pObject->fPos[0] + randF(-0.3, 0.3);
	sync.fTarget[1] = pObject->fPos[1] + randF(-0.3, 0.3);
	sync.fTarget[2] = pObject->fPos[2] + randF(-0.3, 0.3);

	sync.fCenter[0] = randF(-0.3, 0.3);
	sync.fCenter[1] = randF(-0.3, 0.3);
	sync.fCenter[2] = randF(-0.3, 0.3);

	sync.byteWeaponID = GetWeaponID();

	sync.byteType = 3;

	BitStream BulletSync;
	BulletSync.Write((BYTE)PacketEnumeration::ID_BULLET_SYNC);
	BulletSync.Write((PCHAR)&sync, sizeof(stBulletData));
	SF->getRakNet()->SendPacket(&BulletSync);
	return true;
}


int gunWait(int weaponId)
{
	switch (weaponId)
	{
	case 30:
		return 700;
		break;

	case 31:
		return 700;
		break;

	case 28:
		return 700;
		break;

	case 29:
		return 700;
		break;

	case 22:
		return 700;
		break;

	case 23:
		return 300;
		break;

	case 24:
		return 700;
		break;

	case 25:
		return 1050;
		break;

	default:
		return 500;
		break;
	}
}


float randF(float low, float high)
{
	return low + rand() * (high - low) / RAND_MAX;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
		SF->initPlugin(mainloop, hModule);
	return TRUE;
}
