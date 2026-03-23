#pragma once
#include "class/enums.hpp"
#include "SDK/ReadyOrNot_classes.hpp"

namespace big
{
    struct BonePair { int parent; int child; };

    // ========== SUSPECT ==========
    static constexpr std::array<BonePair, 14> suspect_skel_1 = { {
        {50,51},{9,50},{8,9},{7,8},
        {10,11},{11,12},{12,15},
        {62,63},{63,64},{64,67},
        {103,104},{104,107},
        {115,116},{116,119}
    } };

    static constexpr std::array<BonePair, 14> suspect_skel_2 = { {
        {44,45},{5,44},{3,5},{2,3},
        {6,7},{7,8},{8,11},
        {49,50},{50,51},{51,54},
        {111,112},{112,115},
        {99,100},{100,103}
    } };

    // ========== CIVILIAN ==========
    static constexpr std::array<BonePair, 14> civ_skel_1 = { {
        {50,51},{9,50},{7,9},{1,7},
        {10,11},{11,12},{12,15},
        {62,63},{63,64},{64,67},
        {103,104},{104,107},
        {115,116},{116,119}
    } };

    static constexpr std::array<BonePair, 14> civ_skel_2 = { {
        {44,45},{5,44},{4,5},{3,4},
        {6,7},{7,8},{8,11},
        {49,50},{50,51},{51,54},
        {111,112},{112,115},
        {99,100},{100,103}
    } };

	class esp
	{
	public:
		static void draw_esp();
	};
}