// items used in assembler as well as C
#ifndef MUAPK_EXP
#define MUAPK_EXP(A) A
#define MUAPK_INT(A0,A1) A0##A1
#define MUAPK_CAT(A0,A1) MUAPK_INT(A0,A1)
#define MUAPK_2_0(A0,A1) A0
#define MUAPK_2_1(A0,A1) A1
#define MUAPK_3_0(A0,A1,A2) A0
#define MUAPK_3_1(A0,A1,A2) A1
#define MUAPK_3_2(A0,A1,A2) A2
#define MUAPK_4_0(A0,A1,A2,A3) A0
#define MUAPK_4_1(A0,A1,A2,A3) A1
#define MUAPK_4_2(A0,A1,A2,A3) A2
#define MUAPK_4_3(A0,A1,A2,A3) A3
#define MUAPK_5_0(A0,A1,A2,A3,A4) A0
#define MUAPK_5_1(A0,A1,A2,A3,A4) A1
#define MUAPK_5_2(A0,A1,A2,A3,A4) A2
#define MUAPK_5_3(A0,A1,A2,A3,A4) A3
#define MUAPK_5_4(A0,A1,A2,A3,A4) A4
#endif // MUAPK_EXP
