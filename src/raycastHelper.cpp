#include "raycastHelper.h"

namespace Raycast {
	RE::NiPoint3 CheckClearance(RE::TESObjectREFR* caster) {
		auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		RE::bhkPickData pick_data;
		RE::NiPoint3 ray_start, ray_end;

		ray_start = caster->data.location;
		ray_start.z += 100.0f;
		ray_end = ray_start;
		ray_end.z += 50000.0f;

		pick_data.rayInput.from = ray_start * havokWorldScale;
		pick_data.rayInput.to = ray_end * havokWorldScale;

		uint32_t collisionFilterInfo = 0;
		RE::PlayerCharacter::GetSingleton()->GetCollisionFilterInfo(collisionFilterInfo);
		pick_data.rayInput.filterInfo = (static_cast<uint32_t>(collisionFilterInfo >> 16) << 16) |
			static_cast<uint32_t>(RE::COL_LAYER::kCharController);

		caster->GetParentCell()->GetbhkWorld()->PickObject(pick_data);
		RE::NiPoint3 hitpos;
		if (pick_data.rayOutput.HasHit()) {
			hitpos = ray_start + (ray_end - ray_start) * pick_data.rayOutput.hitFraction;
		}
		else {
			hitpos = ray_end;
		}
		return hitpos;
	}
}