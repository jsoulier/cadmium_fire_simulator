#include <SDL3/SDL.h>
#include <imgui.h>

#include <cassert>
#include <cstdint>

#include "fire_fuel_model.hpp"

bool FireFuelModelTypeIsBurnable(FireFuelModelType fuelModel)
{
    return fuelModel > 0 && (fuelModel < kFireFuelModelNB1 || fuelModel > kFireFuelModelNB9);
}

ImColor FireFuelModelTypeGetColor(FireFuelModelType fuelModel)
{
    switch (fuelModel)
    {
    case kFireFuelModelFM1: return ImColor(255, 251, 143);
    case kFireFuelModelFM2: return ImColor(240, 227, 104);
    case kFireFuelModelFM3: return ImColor(219, 202, 76);
    case kFireFuelModelFM4: return ImColor(197, 176, 120);
    case kFireFuelModelFM5: return ImColor(178, 154, 96);
    case kFireFuelModelFM6: return ImColor(158, 133, 74);
    case kFireFuelModelFM7: return ImColor(137, 112, 56);
    case kFireFuelModelFM8: return ImColor(120, 176, 100);
    case kFireFuelModelFM9: return ImColor(88, 148, 76);
    case kFireFuelModelFM10: return ImColor(58, 120, 56);
    case kFireFuelModelFM11: return ImColor(190, 140, 96);
    case kFireFuelModelFM12: return ImColor(166, 116, 74);
    case kFireFuelModelFM13: return ImColor(140, 92, 54);

    case kFireFuelModelNB1:
    case kFireFuelModelNB2:
    case kFireFuelModelNB3:
    case kFireFuelModelNB4:
    case kFireFuelModelNB5:
    case kFireFuelModelNB8:
    case kFireFuelModelNB9:
        return ImColor(130, 130, 130);

    case kFireFuelModelGR1:
    case kFireFuelModelGR2:
    case kFireFuelModelGR3:
    case kFireFuelModelGR4:
    case kFireFuelModelGR5:
    case kFireFuelModelGR6:
    case kFireFuelModelGR7:
    case kFireFuelModelGR8:
    case kFireFuelModelGR9:
    case kFireFuelModelVHb:
    case kFireFuelModelVHa:
        return ImColor(255, 237, 130);

    case kFireFuelModelGS1:
    case kFireFuelModelGS2:
    case kFireFuelModelGS3:
    case kFireFuelModelGS4:
        return ImColor(202, 214, 104);

    case kFireFuelModelSH1:
    case kFireFuelModelSH2:
    case kFireFuelModelSH3:
    case kFireFuelModelSH4:
    case kFireFuelModelSH5:
    case kFireFuelModelSH6:
    case kFireFuelModelSH7:
    case kFireFuelModelSH8:
    case kFireFuelModelSH9:
    case kFireFuelModelSCAL17:
    case kFireFuelModelSCAL15:
    case kFireFuelModelSCAL16:
    case kFireFuelModelSCAL14:
    case kFireFuelModelSCAL18:
    case kFireFuelModelVMH:
    case kFireFuelModelVMMb:
    case kFireFuelModelVMAb:
    case kFireFuelModelVMMa:
    case kFireFuelModelVMaa:
        return ImColor(163, 143, 92);

    case kFireFuelModelTU1:
    case kFireFuelModelTU2:
    case kFireFuelModelTU3:
    case kFireFuelModelTU4:
    case kFireFuelModelTU5:
    case kFireFuelModelMEUCd:
    case kFireFuelModelMH:
    case kFireFuelModelMF:
    case kFireFuelModelMCAD:
    case kFireFuelModelMESC:
    case kFireFuelModelMPIN:
    case kFireFuelModelMEUC:
        return ImColor(118, 168, 92);

    case kFireFuelModelTL1:
    case kFireFuelModelTL2:
    case kFireFuelModelTL3:
    case kFireFuelModelTL4:
    case kFireFuelModelTL5:
    case kFireFuelModelTL6:
    case kFireFuelModelTL7:
    case kFireFuelModelTL8:
    case kFireFuelModelTL9:
    case kFireFuelModelFRAC:
    case kFireFuelModelFFOL:
    case kFireFuelModelFPIN:
    case kFireFuelModelFEUC:
        return ImColor(56, 110, 63);

    case kFireFuelModelSB1:
    case kFireFuelModelSB2:
    case kFireFuelModelSB3:
    case kFireFuelModelSB4:
        return ImColor(158, 109, 71);
    }

    SDL_assert(false);
    return ImColor(0, 0, 0);
}
