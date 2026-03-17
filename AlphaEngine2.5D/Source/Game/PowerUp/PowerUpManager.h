#pragma once
#include "PowerUp.h"
#include <vector>

namespace Game {

    class PowerUpManager {
    public:
        static std::vector<PowerUpData> allPowerUps;

        static void CreatePowerUpObject(PowerUpData data);
        static void PowerUpDataInit();
        static void Init();
        static void FreeManager();
    };

} // namespace Game