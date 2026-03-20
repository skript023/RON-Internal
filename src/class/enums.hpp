#pragma once

enum class EBonesIndex : int
{
    Root = 0,
    pelvis = 1,
    pelvis_hlp_1 = 2,
    pelvis_hlp_2 = 3,
    pelvis_hlp_3 = 4,
    pelvis_hlp_4 = 5,
    torso_stabilizer = 6,
    spine_1 = 7,
    spine_2 = 8,
    spine_3 = 9,

    clavicle_LE = 10,
    upperarm_LE = 11,
    lowerarm_LE = 12,
    lowerarm_hlp_front_LE = 13,
    lowerarm_hlp_back_LE = 14,
    hand_LE = 15,

    thumb_1_LE = 16,
    thumb_2_LE = 17,
    thumb_3_LE = 18,
    thumb_4_end_LE = 19,

    index_1_LE = 20,
    index_2_LE = 21,
    index_3_LE = 22,
    index_4_LE = 23,
    index_5_end_LE = 24,

    middle_1_LE = 25,
    middle_2_LE = 26,
    middle_3_LE = 27,
    middle_4_LE = 28,
    middle_5_end_LE = 29,

    ring_1_LE = 30,
    ring_2_LE = 31,
    ring_3_LE = 32,
    ring_4_LE = 33,
    ring_5_end_LE = 34,

    pinky_1_LE = 35,
    pinky_2_LE = 36,
    pinky_3_LE = 37,
    pinky_4_LE = 38,
    pinky_5_end_LE = 39,

    extra_hand_item_01_LE = 40,
    lowerarm_twist_1_LE = 41,
    lowerarm_twist_2_LE = 42,
    upperarm_twist_1_LE = 43,
    upperarm_hlp_deltoid_LE = 44,
    upperarm_hlp_front_LE = 45,
    upperarm_hlp_rear_LE = 46,
    upperarm_twist_2_LE = 47,

    extra_item_01 = 48,
    extra_item_02 = 49,

    neck_1 = 50,
    Head = 51,
    head_equipment = 52,
    head_end = 53,

    fp_camera = 54,
    weapon_left = 55,
    weapon_right = 56,

    ik_hand_weapon_root = 57,
    ik_hand_weapon_static_LE = 58,
    ik_hand_weapon_dynamic_LE = 59,
    ik_hand_weapon_static_RI = 60,
    ik_hand_weapon_dynamic_RI = 61,

    clavicle_RI = 62,
    upperarm_RI = 63,
    lowerarm_RI = 64,
    lowerarm_hlp_front_RI = 65,
    lowerarm_hlp_back_RI = 66,
    hand_RI = 67,

    thumb_1_RI = 68,
    thumb_2_RI = 69,
    thumb_3_RI = 70,
    thumb_4_end_RI = 71,

    index_1_RI = 72,
    index_2_RI = 73,
    index_3_RI = 74,
    index_4_RI = 75,
    index_5_end_RI = 76,

    middle_1_RI = 77,
    middle_2_RI = 78,
    middle_3_RI = 79,
    middle_4_RI = 80,
    middle_5_end_RI = 81,

    ring_1_RI = 82,
    ring_2_RI = 83,
    ring_3_RI = 84,
    ring_4_RI = 85,
    ring_5_end_RI = 86,

    pinky_1_RI = 87,
    pinky_2_RI = 88,
    pinky_3_RI = 89,
    pinky_4_RI = 90,
    pinky_5_end_RI = 91,

    extra_hand_item_01_RI = 92,
    lowerarm_twist_1_RI = 93,
    lowerarm_twist_2_RI = 94,
    upperarm_twist_1_RI = 95,
    upperarm_hlp_deltoid_RI = 96,
    upperarm_hlp_front_RI = 97,
    upperarm_hlp_rear_RI = 98,
    upperarm_twist_2_RI = 99,

    ik_hand_spine_root = 100,
    ik_hand_spine_LE = 101,
    ik_hand_spine_RI = 102,

    thigh_LE = 103,
    calf_LE = 104,
    calf_hlp_front_LE = 105,
    calf_hlp_back_LE = 106,
    foot_LE = 107,
    heel_LE = 108,
    ball_LE = 109,
    ball_end_LE = 110,
    calf_twist_1_LE = 111,
    calf_twist_2_LE = 112,
    thigh_twist_1_LE = 113,
    thigh_twist_2_LE = 114,

    thigh_RI = 115,
    calf_RI = 116,
    calf_hlp_front_RI = 117,
    calf_hlp_back_RI = 118,
    foot_RI = 119,
    heel_RI = 120,
    ball_RI = 121,
    ball_end_RI = 122,
    calf_twist_1_RI = 123,
    calf_twist_2_RI = 124,
    thigh_twist_1_RI = 125,
    thigh_twist_2_RI = 126,

    ik_foot_root = 127,
    ik_foot_LE = 128,
    ik_foot_RI = 129,

    ik_hand_free_root = 130,
    ik_hand_free_LE = 131,
    ik_hand_free_RI = 132,

    Sync = 133
};

/*
Bone 0: Root
Bone 1: pelvis
Bone 2: spine_01
Bone 3: spine_02
Bone 4: spine_03
Bone 5: spine_04
Bone 6: clavicle_l
Bone 7: upperarm_l
Bone 8: upperarm_twist_01_l
Bone 9: upperarm_twist_02_l
Bone 10: upperarm_correctiveRoot_l
Bone 11: upperarm_out_l
Bone 12: upperarm_fwd_l
Bone 13: upperarm_bck_l
Bone 14: upperarm_in_l
Bone 15: lowerarm_l
Bone 16: lowerarm_twist_01_l
Bone 17: lowerarm_twist_02_l
Bone 18: lowerarm_out_l
Bone 19: lowerarm_fwd_l
Bone 20: lowerarm_in_l
Bone 21: lowerarm_bck_l
Bone 22: hand_l
Bone 23: wrist_outer_l
Bone 24: wrist_inner_l
Bone 25: middle_01_l
Bone 26: middle_02_l
Bone 27: middle_03_l
Bone 28: index_01_l
Bone 29: index_02_l
Bone 30: index_03_l
Bone 31: thumb_01_l
Bone 32: thumb_02_l
Bone 33: thumb_03_l
Bone 34: cup_l
Bone 35: pinky_01_l
Bone 36: pinky_02_l
Bone 37: pinky_03_l
Bone 38: ring_01_l
Bone 39: ring_02_l
Bone 40: ring_03_l
Bone 41: Item_l
Bone 42: clavicle_scap_l
Bone 43: clavicle_r
Bone 44: upperarm_r
Bone 45: upperarm_twist_01_r
Bone 46: upperarm_twist_02_r
Bone 47: upperarm_correctiveRoot_r
Bone 48: upperarm_bck_r
Bone 49: upperarm_fwd_r
Bone 50: upperarm_in_r
Bone 51: upperarm_out_r
Bone 52: lowerarm_r
Bone 53: lowerarm_twist_01_r
Bone 54: lowerarm_twist_02_r
Bone 55: lowerarm_in_r
Bone 56: lowerarm_bck_r
Bone 57: lowerarm_out_r
Bone 58: lowerarm_fwd_r
Bone 59: hand_r
Bone 60: wrist_outer_r
Bone 61: wrist_inner_r
Bone 62: middle_01_r
Bone 63: middle_02_r
Bone 64: middle_03_r
Bone 65: index_01_r
Bone 66: index_02_r
Bone 67: index_03_r
Bone 68: thumb_01_r
Bone 69: thumb_02_r
Bone 70: thumb_03_r
Bone 71: cup_r
Bone 72: pinky_01_r
Bone 73: pinky_02_r
Bone 74: pinky_03_r
Bone 75: ring_01_r
Bone 76: ring_02_r
Bone 77: ring_03_r
Bone 78: Item_r
Bone 79: clavicle_scap_r
Bone 80: neck_01
Bone 81: Head
Bone 82: Bone_cloth1_B_l
Bone 83: Bone_cloth11_B_l
Bone 84: Bone_cloth12_B_l
Bone 85: Bone_cloth13_B_l
Bone 86: Bone_cloth1End_B_l
Bone 87: Bone_cloth1_B_r
Bone 88: Bone_cloth11_B_r
Bone 89: Bone_cloth12_B_r
Bone 90: Bone_cloth13_B_r
Bone 91: Bone_cloth1End_B_r
Bone 92: Bone_hair_behind_root
Bone 93: Bone_hair1_C_l
Bone 94: Bone_hair11_C_l
Bone 95: Bone_hair12_C_l
Bone 96: Bone_hair13_C_l
Bone 97: Bone_hair14_C_l
Bone 98: Bone_hair15_C_l
Bone 99: Bone_hair16_C_l
Bone 100: Bone_hair2_C_l
Bone 101: Bone_hair21_C_l
Bone 102: Bone_hair22_C_l
Bone 103: Bone_hair23_C_l
Bone 104: Bone_hair24_C_l
Bone 105: Bone_hair25_C_l
Bone 106: Bone_hair26_C_l
Bone 107: Bone_hair27_C_l
Bone 108: Bone_hair28_C_l
Bone 109: Bone_hair251_C_l
Bone 110: Bone_hair252_C_l
Bone 111: Bone_hair253_C_l
Bone 112: Bone_hair254_C_l
Bone 113: Bone_hair3_C_l
Bone 114: Bone_hair31_C_l
Bone 115: Bone_hair32_C_l
Bone 116: Bone_hair33_C_l
Bone 117: Bone_hair34_C_l
Bone 118: Bone_hair35_C_l
Bone 119: Bone_hair36_C_l
Bone 120: Bone_hair37_C_l
Bone 121: Bone_hair38_C_l
Bone 122: Bone_hair361_C_l
Bone 123: Bone_hair362_C_l
Bone 124: Bone_hair363_C_l
Bone 125: Bone_hair364_C_l
Bone 126: Bone_hair365_C_l
Bone 127: Bone_hair3_C_r
Bone 128: Bone_hair31_C_r
Bone 129: Bone_hair32_C_r
Bone 130: Bone_hair33_C_r
Bone 131: Bone_hair34_C_r
Bone 132: Bone_hair35_C_r
Bone 133: Bone_hair36_C_r
Bone 134: Bone_hair37_C_r
Bone 135: Bone_hair38_C_r
Bone 136: Bone_hair361_C_r
Bone 137: Bone_hair362_C_r
Bone 138: Bone_hair363_C_r
Bone 139: Bone_hair364_C_r
Bone 140: Bone_hair365_C_r
Bone 141: Bone_hair2_C_r
Bone 142: Bone_hair21_C_r
Bone 143: Bone_hair22_C_r
Bone 144: Bone_hair23_C_r
Bone 145: Bone_hair24_C_r
Bone 146: Bone_hair25_C_r
Bone 147: Bone_hair26_C_r
Bone 148: Bone_hair27_C_r
Bone 149: Bone_hair28_C_r
Bone 150: Bone_hair251_C_r
Bone 151: Bone_hair252_C_r
Bone 152: Bone_hair253_C_r
Bone 153: Bone_hair254_C_r
Bone 154: Bone_hair1_C_r
Bone 155: Bone_hair11_C_r
Bone 156: Bone_hair12_C_r
Bone 157: Bone_hair13_C_r
Bone 158: Bone_hair14_C_r
Bone 159: Bone_hair15_C_r
Bone 160: Bone_hair16_C_r
Bone 161: Bone_hair_front_root
Bone 162: Bone_hair1_B_c
Bone 163: Bone_hair11_B_c
Bone 164: Bone_hair12_B_c
Bone 165: Bone_hair13_B_c
Bone 166: Bone_hair14_B_c
Bone 167: Bone_hair3_B_c
Bone 168: Bone_hair31_B_c
Bone 169: Bone_hair32_B_c
Bone 170: Bone_hair33_B_c
Bone 171: Bone_hair34_B_c
Bone 172: Bone_hair2_B_c
Bone 173: Bone_hair21_B_c
Bone 174: Bone_hair22_B_c
Bone 175: Bone_hair23_B_c
Bone 176: Bone_hair24_B_c
Bone 177: Bone_hair3_A_c
Bone 178: Bone_hair31_A_c
Bone 179: Bone_hair32_A_c
Bone 180: Bone_hair33_A_c
Bone 181: Bone_hair34_A_c
Bone 182: Bone_hair1_A_c
Bone 183: Bone_hair11_A_c
Bone 184: Bone_hair12_A_c
Bone 185: Bone_hair13_A_c
Bone 186: Bone_hair2_A_c
Bone 187: Bone_hair21_A_c
Bone 188: Bone_hair22_A_c
Bone 189: Bone_hair23_A_c
Bone 190: Bone_hair24_A_c
Bone 191: facial_Root
Bone 192: facial_Nose
Bone 193: facial_Ear_l
Bone 194: facial_EyeAll_l
Bone 195: facial_InEyelid_l
Bone 196: facial_InEyelid01_l
Bone 197: facial_UpperInnerEyelid_l
Bone 198: facial_UpperEyelid_l
Bone 199: facial_UpperOuterEyelid_l
Bone 200: facial_OutEyelid01_l
Bone 201: facial_OutEyelid_l
Bone 202: facial_LowerInnerEyelid_l
Bone 203: facial_LowerInnerEyelid01_l
Bone 204: facial_LowerEyelid_l
Bone 205: facial_LowerOuterEyelid01_l
Bone 206: facial_LowerOuterEyelid_l
Bone 207: facial_eyelash1_l
Bone 208: facial_eyelash2_l
Bone 209: facial_eyelash3_l
Bone 210: facial_eyelash4_l
Bone 211: facial_Eye01_l
Bone 212: facial_Eye02_l
Bone 213: facial_Eye03_l
Bone 214: facial_Ear_r
Bone 215: facial_EyeAll_r
Bone 216: facial_InEyelid_r
Bone 217: facial_InEyelid01_r
Bone 218: facial_UpperInnerEyelid_r
Bone 219: facial_UpperEyelid_r
Bone 220: facial_UpperOuterEyelid_r
Bone 221: facial_OutEyelid01_r
Bone 222: facial_OutEyelid_r
Bone 223: facial_LowerInnerEyelid_r
Bone 224: facial_LowerInnerEyelid01_r
Bone 225: facial_LowerEyelid_r
Bone 226: facial_LowerOuterEyelid01_r
Bone 227: facial_LowerOuterEyelid_r
Bone 228: facial_eyelash1_r
Bone 229: facial_eyelash2_r
Bone 230: facial_eyelash3_r
Bone 231: facial_eyelash4_r
Bone 232: facial_Eye01_r
Bone 233: facial_Eye02_r
Bone 234: facial_Eye03_r
Bone 235: facial_Puffer_r
Bone 236: facial_Puffer_l
Bone 237: facial_Mouth_UpLipPinch_r
Bone 238: facial_Mouth_LowLipPinch_r
Bone 239: facial_Mouth_LowLipPinch_l
Bone 240: facial_Mouth_UpLipPinch_l
Bone 241: facial_Tongue03
Bone 242: facial_Tongue02
Bone 243: facial_Tongue01
Bone 244: facial_LowerTooth
Bone 245: facial_UpperTooth
Bone 246: facial_Mouth_UpLipCenter_r
Bone 247: facial_Mouth_LowLipCenter_r
Bone 248: facial_Mouth_Corner_r
Bone 249: facial_Mouth_UpLipCenter_l
Bone 250: facial_Mouth_LowLipCenter_l
Bone 251: facial_Mouth_Corner_l
Bone 252: facial_Mouth_UpLip
Bone 253: facial_Mouth_LowLip
Bone 254: facial_Jaw
Bone 255: facial_chin
Bone 256: facial_CheekOuter2_l
Bone 257: facial_JawBulge1_l
Bone 258: facial_CheekOuter1_l
Bone 259: facial_JawBulge2_l
Bone 260: facial_CheekOuter2_r
Bone 261: facial_CheekOuter1_r
Bone 262: facial_JawBulge2_r
Bone 263: facial_JawBulge1_r
Bone 264: facial_Brow7_l
Bone 265: facial_Brow4_l
Bone 266: facial_Brow6_l
Bone 267: facial_Brow5_l
Bone 268: facial_Brow8_l
Bone 269: facial_Brow9_l
Bone 270: facial_Brow9_r
Bone 271: facial_Brow8_r
Bone 272: facial_Brow7_r
Bone 273: facial_Brow6_r
Bone 274: facial_Brow4_r
Bone 275: facial_Brow5_r
Bone 276: facial_HeadTop_c
Bone 277: facial_Forehead1_l
Bone 278: facial_Forehead2_l
Bone 279: facial_Forehead3_l
Bone 280: facial_Forehead1_r
Bone 281: facial_Forehead2_r
Bone 282: facial_Forehead3_r
Bone 283: facial_Naso_l
Bone 284: facial_Naso_r
Bone 285: facial_NoseTip
Bone 286: FPPCamera
Bone 287: clavicle_pec_l
Bone 288: spine_04_latissimus_l
Bone 289: clavicle_pec_r
Bone 290: spine_04_latissimus_r
Bone 291: thigh_l
Bone 292: thigh_twist_01_l
Bone 293: thigh_twist_02_l
Bone 294: calf_l
Bone 295: calf_twist_01_l
Bone 296: calf_twist_02_l
Bone 297: Foot_L
Bone 298: ball_l
Bone 299: ankle_fwd_l
Bone 300: ankle_bck_l
Bone 301: calf_kneeBack_l
Bone 302: calf_knee_l
Bone 303: thigh_correctiveRoot_l
Bone 304: thigh_out_l
Bone 305: thigh_in_l
Bone 306: thigh_bck_lwr_l
Bone 307: thigh_bck_lwrCor1_l
Bone 308: thigh_bck_lwrCor11_l
Bone 309: thigh_bck_l
Bone 310: thigh_fwd_l
Bone 311: thigh_fwd_lwr_l
Bone 312: thigh_r
Bone 313: thigh_twist_01_r
Bone 314: thigh_twist_02_r
Bone 315: calf_r
Bone 316: calf_twist_01_r
Bone 317: calf_twist_02_r
Bone 318: calf_kneeBack_r
Bone 319: calf_knee_r
Bone 320: Foot_R
Bone 321: ball_r
Bone 322: ankle_fwd_r
Bone 323: ankle_bck_r
Bone 324: thigh_correctiveRoot_r
Bone 325: thigh_in_r
Bone 326: thigh_fwd_lwr_r
Bone 327: thigh_fwd_r
Bone 328: thigh_bck_lwr_r
Bone 329: thigh_bck_lwrCor1_r
Bone 330: thigh_bck_lwrCor11_r
Bone 331: thigh_bck_r
Bone 332: thigh_out_r
Bone 333: Bone_cloth1_B_c
Bone 334: Bone_cloth2_B_c
Bone 335: Bone_cloth3_B_c
Bone 336: Bone_cloth1End_B_c
Bone 337: Bone_cloth1_A_c
Bone 338: Bone_cloth2_A_c
Bone 339: Bone_cloth3_A_c
Bone 340: Bone_cloth4_A_c
Bone 341: ik_hand_root
Bone 342: ik_hand_gun
Bone 343: ik_hand_r
Bone 344: ik_hand_l
Bone 345: ik_foot_root
Bone 346: ik_foot_l
Bone 347: ik_foot_r
Bone 348: Effect_root
Bone 349: Effect_hand_l
Bone 350: Effect_hand_r
Bone 351: Item_Root
Bone 352: VB ik_foot_root
Bone 353: VB Curves
Bone 354: VB root_head
Bone 355: VB Arm_L
Bone 356: VB Arm_R
Bone 357: VB hand_l_hand_r
Bone 358: VB hand_l_ik_hand_gun
Bone 359: VB VB hand_l_ik_hand_gun_hand_r
Bone 360: VB hand_r_hand_l
Bone 361: VB hand_r_ik_hand_gun
Bone 362: VB VB hand_r_ik_hand_gun_hand_l
Bone 363: VB Head
Bone 364: VB VB Head_ROOT
Bone 365: VB CameraToGunSpace
Bone 366: VB CameraToGun
Bone 367: VB CameraToGunToROOT
Bone 368: VB foot_target_l
Bone 369: VB foot_target_r
Bone 370: VB root_item_r
Bone 371: VB root_hand_r
Bone 372: VB root_ik_hand_gun
Bone 373: VB ik_hand_root_ik_hand_gun
Bone 374: VB FPPCamera
Bone 375: VB FPPCameraReadOnly
Bone 376: VB ADSCamera
Bone 377: VB ADSCameraReadOnly
*/