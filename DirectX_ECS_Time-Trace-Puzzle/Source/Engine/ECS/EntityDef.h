#ifndef ___ENTITY_DEF_H___
#define ___ENTITY_DEF_H___

// ===== インクルード =====
#include "Engine/pch.h"

// Entity型とNullEntity定数をここで定義する
using Entity = uint32_t;
constexpr Entity NullEntity = 0xFFFFFFFF;

#endif // !___ENTITY_DEF_H___
