#pragma once

#include "Engine/EngineTypes.h"         // ENetMode
#include "GameFramework/Actor.h"        // ENetRole via actor headers

inline const TCHAR* MO56RoleToString(ENetRole Role)
{
        switch (Role)
        {
        case ROLE_None:            return TEXT("ROLE_None");
        case ROLE_SimulatedProxy:  return TEXT("ROLE_SimulatedProxy");
        case ROLE_AutonomousProxy: return TEXT("ROLE_AutonomousProxy");
        case ROLE_Authority:       return TEXT("ROLE_Authority");
        default:                   return TEXT("ROLE_Unknown");
        }
}

inline const TCHAR* MO56NetModeToString(ENetMode Mode)
{
        switch (Mode)
        {
        case NM_Standalone:       return TEXT("NM_Standalone");
        case NM_DedicatedServer:  return TEXT("NM_DedicatedServer");
        case NM_ListenServer:     return TEXT("NM_ListenServer");
        case NM_Client:           return TEXT("NM_Client");
        default:                  return TEXT("NM_Unknown");
        }
}
