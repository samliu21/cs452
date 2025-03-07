#include "track_seg.h"

void init_sega(track_node* track)
{
    for (int i = 0; i < TRACK_MAX; ++i) {
        track[i].enters_seg[0] = track[i].enters_seg[1] = -1;
    }

    // BR1 -> EX7, EN7 -> MR1
    track[80].enters_seg[1] = 0;
    track[136].enters_seg[0] = 1;

    // BR1 -> EX8, EN8 -> MR1
    track[80].enters_seg[0] = 1;
    track[138].enters_seg[0] = 1;

    // BR2 -> BR1, MR1 -> MR2
    track[82].enters_seg[0] = 2;
    track[81].enters_seg[0] = 2;

    // BR2 -> EX10, EN10 -> MR2
    track[82].enters_seg[1] = 3;
    track[142].enters_seg[0] = 3;

    // BR3 -> EX9, EN9 -> MR3
    track[84].enters_seg[0] = 4;
    track[140].enters_seg[0] = 4;

    // BR3 -> BR2, MR2 -> MR3
    track[84].enters_seg[1] = 5;
    track[83].enters_seg[0] = 5;

    // BR4 -> EX6, EN6 -> MR4
    track[86].enters_seg[0] = 6;
    track[134].enters_seg[0] = 6;

    // BR4 -> EX4, EN4 -> MR4
    track[86].enters_seg[1] = 7;
    track[130].enters_seg[0] = 7;

    // BR5 -> MR7, BR7 -> MR5
    track[88].enters_seg[1] = 8;
    track[92].enters_seg[1] = 8;

    // BR5 -> EX3, EN3 -> MR5
    track[88].enters_seg[0] = 9;
    track[128].enters_seg[0] = 9;

    // BR6 -> MR18, BR18 -> MR6
    track[90].enters_seg[1] = 10;
    track[114].enters_seg[1] = 10;

    // BR6 -> MR7, BR7 -> MR6
    track[90].enters_seg[0] = 11;
    track[92].enters_seg[0] = 11;

    // BR8 -> BR17, MR17 -> MR8
    track[94].enters_seg[1] = 12;
    track[113].enters_seg[0] = 12;

    // BR8 -> BR7, MR7 -> MR8
    track[94].enters_seg[0] = 13;
    track[93].enters_seg[0] = 13;

    // BR9 -> BR11, MR11 -> MR9
    track[96].enters_seg[0] = 14;
    track[101].enters_seg[0] = 14;

    // BR9 -> BR10, MR10 -> MR9
    track[96].enters_seg[1] = 15;
    track[99].enters_seg[0] = 15;

    // BR10 -> MR155, BR155 -> MR10
    track[98].enters_seg[1] = 16;
    track[120].enters_seg[1] = 16;

    // BR10 -> MR13, BR13 -> MR10
    track[98].enters_seg[0] = 17;
    track[104].enters_seg[0] = 17;

    // BR11 -> MR14, BR14 -> MR11
    track[100].enters_seg[1] = 18;
    track[106].enters_seg[0] = 18;

    // BR11 -> BR12, MR12 -> MR11
    track[100].enters_seg[0] = 19;
    track[103].enters_seg[0] = 19;

    // BR12 -> BR4, MR4 -> MR12
    track[102].enters_seg[1] = 20;
    track[87].enters_seg[0] = 20;

    // BR12 -> EX5, EN5 -> MR12
    track[102].enters_seg[0] = 21;
    track[132].enters_seg[0] = 21;

    // BR13 -> MR156, BR156 -> MR13
    track[104].enters_seg[1] = 22;
    track[122].enters_seg[1] = 22;

    // BR14 -> BR13, MR13 -> MR14
    track[106].enters_seg[1] = 23;
    track[105].enters_seg[0] = 23;

    // BR15 -> BR6, MR6 -> MR15
    track[108].enters_seg[0] = 24;
    track[91].enters_seg[0] = 24;

    // BR15 -> BR16, MR16 -> MR15
    track[108].enters_seg[1] = 25;
    track[111].enters_seg[0] = 25;

    // BR16 -> MR153, BR153 -> MR16
    track[110].enters_seg[1] = 26;
    track[116].enters_seg[0] = 26;

    // BR16 -> MR17, BR17 -> MR16
    track[110].enters_seg[0] = 27;
    track[112].enters_seg[0] = 27;

    // BR17 -> MR154, BR154 -> MR17
    track[112].enters_seg[1] = 28;
    track[118].enters_seg[1] = 28;

    // BR18 -> BR3, MR3 -> MR18
    track[114].enters_seg[0] = 29;
    track[85].enters_seg[0] = 29;

    // BR153 -> EX1, EN1 -> MR153
    track[116].enters_seg[0] = 30;
    track[124].enters_seg[0] = 30;

    // BR154 -> BR153, MR153 -> MR154, TODO: fix
    track[118].enters_seg[1] = 31;
    track[117].enters_seg[0] = 31;

    // BR155 -> EX2, EN2 -> MR155
    track[120].enters_seg[0] = 32;
    track[126].enters_seg[0] = 32;

    // BR156 -> BR155, MR155 -> MR156, TODO: fix
    track[122].enters_seg[1] = 33;
    track[121].enters_seg[0] = 33;

    // MR5 -> BR18, MR18 -> BR5
    track[89].enters_seg[0] = 34;
    track[115].enters_seg[0] = 34;

    // MR8 -> BR9, MR9 -> BR8
    track[95].enters_seg[0] = 35;
    track[97].enters_seg[0] = 35;

    // MR14 -> BR15, MR15 -> BR14
    track[107].enters_seg[0] = 36;
    track[109].enters_seg[0] = 36;

    // MR154 -> BR156, MR156 -> BR154, TODO: fix
    track[119].enters_seg[0] = 37;
    track[123].enters_seg[0] = 37;
}