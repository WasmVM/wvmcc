/* LANG-5.2.4.1-01 -- 5.2.4.1p1: representative stress program exercising the
 * C17 minimum translation limits within a single translation unit:
 *   127 nesting levels of blocks; 63 nesting levels of conditional inclusion;
 *   63 nesting levels of parenthesized expressions; 12 pointer declarators
 *   modifying a type; 63 significant characters in an internal identifier;
 *   1023 case labels in a switch; 1023 enumeration constants in one enum;
 *   1023 members in one struct; 127 parameters in a function definition and
 *   127 arguments in a call; 127 parameters/arguments of a macro; 511
 *   identifiers with block scope in one block; a string literal of 4095
 *   characters; an object of 65535 bytes.
 * (15 levels of #include nesting and 4095 external identifiers need
 *  multiple files / excessive bulk and are exercised representatively
 *  elsewhere.)  GENERATED FILE -- see catalog row for provenance.
 * Returns 0 on success, a distinct non-zero code per failed check. */

/* 63 nesting levels of conditional inclusion */
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#if 1
#define COND_INCL_63 1
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#ifndef COND_INCL_63
#define COND_INCL_63 0
#endif

/* 127 parameters in a macro definition, 127 arguments in its invocation */
#define SUM127(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38, p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57, p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76, p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95, p96, p97, p98, p99, p100, p101, p102, p103, p104, p105, p106, p107, p108, p109, p110, p111, p112, p113, p114, p115, p116, p117, p118, p119, p120, p121, p122, p123, p124, p125, p126) ((long)p0 + (long)p1 + (long)p2 + (long)p3 + (long)p4 + (long)p5 + (long)p6 + (long)p7 + (long)p8 + (long)p9 + (long)p10 + (long)p11 + (long)p12 + (long)p13 + (long)p14 + (long)p15 + (long)p16 + (long)p17 + (long)p18 + (long)p19 + (long)p20 + (long)p21 + (long)p22 + (long)p23 + (long)p24 + (long)p25 + (long)p26 + (long)p27 + (long)p28 + (long)p29 + (long)p30 + (long)p31 + (long)p32 + (long)p33 + (long)p34 + (long)p35 + (long)p36 + (long)p37 + (long)p38 + (long)p39 + (long)p40 + (long)p41 + (long)p42 + (long)p43 + (long)p44 + (long)p45 + (long)p46 + (long)p47 + (long)p48 + (long)p49 + (long)p50 + (long)p51 + (long)p52 + (long)p53 + (long)p54 + (long)p55 + (long)p56 + (long)p57 + (long)p58 + (long)p59 + (long)p60 + (long)p61 + (long)p62 + (long)p63 + (long)p64 + (long)p65 + (long)p66 + (long)p67 + (long)p68 + (long)p69 + (long)p70 + (long)p71 + (long)p72 + (long)p73 + (long)p74 + (long)p75 + (long)p76 + (long)p77 + (long)p78 + (long)p79 + (long)p80 + (long)p81 + (long)p82 + (long)p83 + (long)p84 + (long)p85 + (long)p86 + (long)p87 + (long)p88 + (long)p89 + (long)p90 + (long)p91 + (long)p92 + (long)p93 + (long)p94 + (long)p95 + (long)p96 + (long)p97 + (long)p98 + (long)p99 + (long)p100 + (long)p101 + (long)p102 + (long)p103 + (long)p104 + (long)p105 + (long)p106 + (long)p107 + (long)p108 + (long)p109 + (long)p110 + (long)p111 + (long)p112 + (long)p113 + (long)p114 + (long)p115 + (long)p116 + (long)p117 + (long)p118 + (long)p119 + (long)p120 + (long)p121 + (long)p122 + (long)p123 + (long)p124 + (long)p125 + (long)p126)

/* 1023 enumeration constants in a single enum */
enum big_enum {
    K0, K1, K2, K3, K4, K5, K6, K7,
    K8, K9, K10, K11, K12, K13, K14, K15,
    K16, K17, K18, K19, K20, K21, K22, K23,
    K24, K25, K26, K27, K28, K29, K30, K31,
    K32, K33, K34, K35, K36, K37, K38, K39,
    K40, K41, K42, K43, K44, K45, K46, K47,
    K48, K49, K50, K51, K52, K53, K54, K55,
    K56, K57, K58, K59, K60, K61, K62, K63,
    K64, K65, K66, K67, K68, K69, K70, K71,
    K72, K73, K74, K75, K76, K77, K78, K79,
    K80, K81, K82, K83, K84, K85, K86, K87,
    K88, K89, K90, K91, K92, K93, K94, K95,
    K96, K97, K98, K99, K100, K101, K102, K103,
    K104, K105, K106, K107, K108, K109, K110, K111,
    K112, K113, K114, K115, K116, K117, K118, K119,
    K120, K121, K122, K123, K124, K125, K126, K127,
    K128, K129, K130, K131, K132, K133, K134, K135,
    K136, K137, K138, K139, K140, K141, K142, K143,
    K144, K145, K146, K147, K148, K149, K150, K151,
    K152, K153, K154, K155, K156, K157, K158, K159,
    K160, K161, K162, K163, K164, K165, K166, K167,
    K168, K169, K170, K171, K172, K173, K174, K175,
    K176, K177, K178, K179, K180, K181, K182, K183,
    K184, K185, K186, K187, K188, K189, K190, K191,
    K192, K193, K194, K195, K196, K197, K198, K199,
    K200, K201, K202, K203, K204, K205, K206, K207,
    K208, K209, K210, K211, K212, K213, K214, K215,
    K216, K217, K218, K219, K220, K221, K222, K223,
    K224, K225, K226, K227, K228, K229, K230, K231,
    K232, K233, K234, K235, K236, K237, K238, K239,
    K240, K241, K242, K243, K244, K245, K246, K247,
    K248, K249, K250, K251, K252, K253, K254, K255,
    K256, K257, K258, K259, K260, K261, K262, K263,
    K264, K265, K266, K267, K268, K269, K270, K271,
    K272, K273, K274, K275, K276, K277, K278, K279,
    K280, K281, K282, K283, K284, K285, K286, K287,
    K288, K289, K290, K291, K292, K293, K294, K295,
    K296, K297, K298, K299, K300, K301, K302, K303,
    K304, K305, K306, K307, K308, K309, K310, K311,
    K312, K313, K314, K315, K316, K317, K318, K319,
    K320, K321, K322, K323, K324, K325, K326, K327,
    K328, K329, K330, K331, K332, K333, K334, K335,
    K336, K337, K338, K339, K340, K341, K342, K343,
    K344, K345, K346, K347, K348, K349, K350, K351,
    K352, K353, K354, K355, K356, K357, K358, K359,
    K360, K361, K362, K363, K364, K365, K366, K367,
    K368, K369, K370, K371, K372, K373, K374, K375,
    K376, K377, K378, K379, K380, K381, K382, K383,
    K384, K385, K386, K387, K388, K389, K390, K391,
    K392, K393, K394, K395, K396, K397, K398, K399,
    K400, K401, K402, K403, K404, K405, K406, K407,
    K408, K409, K410, K411, K412, K413, K414, K415,
    K416, K417, K418, K419, K420, K421, K422, K423,
    K424, K425, K426, K427, K428, K429, K430, K431,
    K432, K433, K434, K435, K436, K437, K438, K439,
    K440, K441, K442, K443, K444, K445, K446, K447,
    K448, K449, K450, K451, K452, K453, K454, K455,
    K456, K457, K458, K459, K460, K461, K462, K463,
    K464, K465, K466, K467, K468, K469, K470, K471,
    K472, K473, K474, K475, K476, K477, K478, K479,
    K480, K481, K482, K483, K484, K485, K486, K487,
    K488, K489, K490, K491, K492, K493, K494, K495,
    K496, K497, K498, K499, K500, K501, K502, K503,
    K504, K505, K506, K507, K508, K509, K510, K511,
    K512, K513, K514, K515, K516, K517, K518, K519,
    K520, K521, K522, K523, K524, K525, K526, K527,
    K528, K529, K530, K531, K532, K533, K534, K535,
    K536, K537, K538, K539, K540, K541, K542, K543,
    K544, K545, K546, K547, K548, K549, K550, K551,
    K552, K553, K554, K555, K556, K557, K558, K559,
    K560, K561, K562, K563, K564, K565, K566, K567,
    K568, K569, K570, K571, K572, K573, K574, K575,
    K576, K577, K578, K579, K580, K581, K582, K583,
    K584, K585, K586, K587, K588, K589, K590, K591,
    K592, K593, K594, K595, K596, K597, K598, K599,
    K600, K601, K602, K603, K604, K605, K606, K607,
    K608, K609, K610, K611, K612, K613, K614, K615,
    K616, K617, K618, K619, K620, K621, K622, K623,
    K624, K625, K626, K627, K628, K629, K630, K631,
    K632, K633, K634, K635, K636, K637, K638, K639,
    K640, K641, K642, K643, K644, K645, K646, K647,
    K648, K649, K650, K651, K652, K653, K654, K655,
    K656, K657, K658, K659, K660, K661, K662, K663,
    K664, K665, K666, K667, K668, K669, K670, K671,
    K672, K673, K674, K675, K676, K677, K678, K679,
    K680, K681, K682, K683, K684, K685, K686, K687,
    K688, K689, K690, K691, K692, K693, K694, K695,
    K696, K697, K698, K699, K700, K701, K702, K703,
    K704, K705, K706, K707, K708, K709, K710, K711,
    K712, K713, K714, K715, K716, K717, K718, K719,
    K720, K721, K722, K723, K724, K725, K726, K727,
    K728, K729, K730, K731, K732, K733, K734, K735,
    K736, K737, K738, K739, K740, K741, K742, K743,
    K744, K745, K746, K747, K748, K749, K750, K751,
    K752, K753, K754, K755, K756, K757, K758, K759,
    K760, K761, K762, K763, K764, K765, K766, K767,
    K768, K769, K770, K771, K772, K773, K774, K775,
    K776, K777, K778, K779, K780, K781, K782, K783,
    K784, K785, K786, K787, K788, K789, K790, K791,
    K792, K793, K794, K795, K796, K797, K798, K799,
    K800, K801, K802, K803, K804, K805, K806, K807,
    K808, K809, K810, K811, K812, K813, K814, K815,
    K816, K817, K818, K819, K820, K821, K822, K823,
    K824, K825, K826, K827, K828, K829, K830, K831,
    K832, K833, K834, K835, K836, K837, K838, K839,
    K840, K841, K842, K843, K844, K845, K846, K847,
    K848, K849, K850, K851, K852, K853, K854, K855,
    K856, K857, K858, K859, K860, K861, K862, K863,
    K864, K865, K866, K867, K868, K869, K870, K871,
    K872, K873, K874, K875, K876, K877, K878, K879,
    K880, K881, K882, K883, K884, K885, K886, K887,
    K888, K889, K890, K891, K892, K893, K894, K895,
    K896, K897, K898, K899, K900, K901, K902, K903,
    K904, K905, K906, K907, K908, K909, K910, K911,
    K912, K913, K914, K915, K916, K917, K918, K919,
    K920, K921, K922, K923, K924, K925, K926, K927,
    K928, K929, K930, K931, K932, K933, K934, K935,
    K936, K937, K938, K939, K940, K941, K942, K943,
    K944, K945, K946, K947, K948, K949, K950, K951,
    K952, K953, K954, K955, K956, K957, K958, K959,
    K960, K961, K962, K963, K964, K965, K966, K967,
    K968, K969, K970, K971, K972, K973, K974, K975,
    K976, K977, K978, K979, K980, K981, K982, K983,
    K984, K985, K986, K987, K988, K989, K990, K991,
    K992, K993, K994, K995, K996, K997, K998, K999,
    K1000, K1001, K1002, K1003, K1004, K1005, K1006, K1007,
    K1008, K1009, K1010, K1011, K1012, K1013, K1014, K1015,
    K1016, K1017, K1018, K1019, K1020, K1021, K1022
};

/* 1023 members in a single struct */
struct big_struct {
    int m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15;
    int m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31;
    int m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47;
    int m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63;
    int m64, m65, m66, m67, m68, m69, m70, m71, m72, m73, m74, m75, m76, m77, m78, m79;
    int m80, m81, m82, m83, m84, m85, m86, m87, m88, m89, m90, m91, m92, m93, m94, m95;
    int m96, m97, m98, m99, m100, m101, m102, m103, m104, m105, m106, m107, m108, m109, m110, m111;
    int m112, m113, m114, m115, m116, m117, m118, m119, m120, m121, m122, m123, m124, m125, m126, m127;
    int m128, m129, m130, m131, m132, m133, m134, m135, m136, m137, m138, m139, m140, m141, m142, m143;
    int m144, m145, m146, m147, m148, m149, m150, m151, m152, m153, m154, m155, m156, m157, m158, m159;
    int m160, m161, m162, m163, m164, m165, m166, m167, m168, m169, m170, m171, m172, m173, m174, m175;
    int m176, m177, m178, m179, m180, m181, m182, m183, m184, m185, m186, m187, m188, m189, m190, m191;
    int m192, m193, m194, m195, m196, m197, m198, m199, m200, m201, m202, m203, m204, m205, m206, m207;
    int m208, m209, m210, m211, m212, m213, m214, m215, m216, m217, m218, m219, m220, m221, m222, m223;
    int m224, m225, m226, m227, m228, m229, m230, m231, m232, m233, m234, m235, m236, m237, m238, m239;
    int m240, m241, m242, m243, m244, m245, m246, m247, m248, m249, m250, m251, m252, m253, m254, m255;
    int m256, m257, m258, m259, m260, m261, m262, m263, m264, m265, m266, m267, m268, m269, m270, m271;
    int m272, m273, m274, m275, m276, m277, m278, m279, m280, m281, m282, m283, m284, m285, m286, m287;
    int m288, m289, m290, m291, m292, m293, m294, m295, m296, m297, m298, m299, m300, m301, m302, m303;
    int m304, m305, m306, m307, m308, m309, m310, m311, m312, m313, m314, m315, m316, m317, m318, m319;
    int m320, m321, m322, m323, m324, m325, m326, m327, m328, m329, m330, m331, m332, m333, m334, m335;
    int m336, m337, m338, m339, m340, m341, m342, m343, m344, m345, m346, m347, m348, m349, m350, m351;
    int m352, m353, m354, m355, m356, m357, m358, m359, m360, m361, m362, m363, m364, m365, m366, m367;
    int m368, m369, m370, m371, m372, m373, m374, m375, m376, m377, m378, m379, m380, m381, m382, m383;
    int m384, m385, m386, m387, m388, m389, m390, m391, m392, m393, m394, m395, m396, m397, m398, m399;
    int m400, m401, m402, m403, m404, m405, m406, m407, m408, m409, m410, m411, m412, m413, m414, m415;
    int m416, m417, m418, m419, m420, m421, m422, m423, m424, m425, m426, m427, m428, m429, m430, m431;
    int m432, m433, m434, m435, m436, m437, m438, m439, m440, m441, m442, m443, m444, m445, m446, m447;
    int m448, m449, m450, m451, m452, m453, m454, m455, m456, m457, m458, m459, m460, m461, m462, m463;
    int m464, m465, m466, m467, m468, m469, m470, m471, m472, m473, m474, m475, m476, m477, m478, m479;
    int m480, m481, m482, m483, m484, m485, m486, m487, m488, m489, m490, m491, m492, m493, m494, m495;
    int m496, m497, m498, m499, m500, m501, m502, m503, m504, m505, m506, m507, m508, m509, m510, m511;
    int m512, m513, m514, m515, m516, m517, m518, m519, m520, m521, m522, m523, m524, m525, m526, m527;
    int m528, m529, m530, m531, m532, m533, m534, m535, m536, m537, m538, m539, m540, m541, m542, m543;
    int m544, m545, m546, m547, m548, m549, m550, m551, m552, m553, m554, m555, m556, m557, m558, m559;
    int m560, m561, m562, m563, m564, m565, m566, m567, m568, m569, m570, m571, m572, m573, m574, m575;
    int m576, m577, m578, m579, m580, m581, m582, m583, m584, m585, m586, m587, m588, m589, m590, m591;
    int m592, m593, m594, m595, m596, m597, m598, m599, m600, m601, m602, m603, m604, m605, m606, m607;
    int m608, m609, m610, m611, m612, m613, m614, m615, m616, m617, m618, m619, m620, m621, m622, m623;
    int m624, m625, m626, m627, m628, m629, m630, m631, m632, m633, m634, m635, m636, m637, m638, m639;
    int m640, m641, m642, m643, m644, m645, m646, m647, m648, m649, m650, m651, m652, m653, m654, m655;
    int m656, m657, m658, m659, m660, m661, m662, m663, m664, m665, m666, m667, m668, m669, m670, m671;
    int m672, m673, m674, m675, m676, m677, m678, m679, m680, m681, m682, m683, m684, m685, m686, m687;
    int m688, m689, m690, m691, m692, m693, m694, m695, m696, m697, m698, m699, m700, m701, m702, m703;
    int m704, m705, m706, m707, m708, m709, m710, m711, m712, m713, m714, m715, m716, m717, m718, m719;
    int m720, m721, m722, m723, m724, m725, m726, m727, m728, m729, m730, m731, m732, m733, m734, m735;
    int m736, m737, m738, m739, m740, m741, m742, m743, m744, m745, m746, m747, m748, m749, m750, m751;
    int m752, m753, m754, m755, m756, m757, m758, m759, m760, m761, m762, m763, m764, m765, m766, m767;
    int m768, m769, m770, m771, m772, m773, m774, m775, m776, m777, m778, m779, m780, m781, m782, m783;
    int m784, m785, m786, m787, m788, m789, m790, m791, m792, m793, m794, m795, m796, m797, m798, m799;
    int m800, m801, m802, m803, m804, m805, m806, m807, m808, m809, m810, m811, m812, m813, m814, m815;
    int m816, m817, m818, m819, m820, m821, m822, m823, m824, m825, m826, m827, m828, m829, m830, m831;
    int m832, m833, m834, m835, m836, m837, m838, m839, m840, m841, m842, m843, m844, m845, m846, m847;
    int m848, m849, m850, m851, m852, m853, m854, m855, m856, m857, m858, m859, m860, m861, m862, m863;
    int m864, m865, m866, m867, m868, m869, m870, m871, m872, m873, m874, m875, m876, m877, m878, m879;
    int m880, m881, m882, m883, m884, m885, m886, m887, m888, m889, m890, m891, m892, m893, m894, m895;
    int m896, m897, m898, m899, m900, m901, m902, m903, m904, m905, m906, m907, m908, m909, m910, m911;
    int m912, m913, m914, m915, m916, m917, m918, m919, m920, m921, m922, m923, m924, m925, m926, m927;
    int m928, m929, m930, m931, m932, m933, m934, m935, m936, m937, m938, m939, m940, m941, m942, m943;
    int m944, m945, m946, m947, m948, m949, m950, m951, m952, m953, m954, m955, m956, m957, m958, m959;
    int m960, m961, m962, m963, m964, m965, m966, m967, m968, m969, m970, m971, m972, m973, m974, m975;
    int m976, m977, m978, m979, m980, m981, m982, m983, m984, m985, m986, m987, m988, m989, m990, m991;
    int m992, m993, m994, m995, m996, m997, m998, m999, m1000, m1001, m1002, m1003, m1004, m1005, m1006, m1007;
    int m1008, m1009, m1010, m1011, m1012, m1013, m1014, m1015, m1016, m1017, m1018, m1019, m1020, m1021, m1022;
};
static struct big_struct sb;

/* an object of 65535 bytes */
static unsigned char big_obj[65535];

/* 63 significant initial characters in an internal identifier */
static int iabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijab = 63;

/* 127 nesting levels of blocks (function body is level 1) */
static int deep_blocks(void)
{
    int n = 0;
    n++;
    { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; { n++; 
    }}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}
    return n;
}

/* 1023 case labels for one switch statement */
static int big_switch(int v)
{
    switch (v) {
    case 0: return 1; case 1: return 3; case 2: return 5; case 3: return 7; case 4: return 9; case 5: return 11; case 6: return 13; case 7: return 15;
    case 8: return 17; case 9: return 19; case 10: return 21; case 11: return 23; case 12: return 25; case 13: return 27; case 14: return 29; case 15: return 31;
    case 16: return 33; case 17: return 35; case 18: return 37; case 19: return 39; case 20: return 41; case 21: return 43; case 22: return 45; case 23: return 47;
    case 24: return 49; case 25: return 51; case 26: return 53; case 27: return 55; case 28: return 57; case 29: return 59; case 30: return 61; case 31: return 63;
    case 32: return 65; case 33: return 67; case 34: return 69; case 35: return 71; case 36: return 73; case 37: return 75; case 38: return 77; case 39: return 79;
    case 40: return 81; case 41: return 83; case 42: return 85; case 43: return 87; case 44: return 89; case 45: return 91; case 46: return 93; case 47: return 95;
    case 48: return 97; case 49: return 99; case 50: return 101; case 51: return 103; case 52: return 105; case 53: return 107; case 54: return 109; case 55: return 111;
    case 56: return 113; case 57: return 115; case 58: return 117; case 59: return 119; case 60: return 121; case 61: return 123; case 62: return 125; case 63: return 127;
    case 64: return 129; case 65: return 131; case 66: return 133; case 67: return 135; case 68: return 137; case 69: return 139; case 70: return 141; case 71: return 143;
    case 72: return 145; case 73: return 147; case 74: return 149; case 75: return 151; case 76: return 153; case 77: return 155; case 78: return 157; case 79: return 159;
    case 80: return 161; case 81: return 163; case 82: return 165; case 83: return 167; case 84: return 169; case 85: return 171; case 86: return 173; case 87: return 175;
    case 88: return 177; case 89: return 179; case 90: return 181; case 91: return 183; case 92: return 185; case 93: return 187; case 94: return 189; case 95: return 191;
    case 96: return 193; case 97: return 195; case 98: return 197; case 99: return 199; case 100: return 201; case 101: return 203; case 102: return 205; case 103: return 207;
    case 104: return 209; case 105: return 211; case 106: return 213; case 107: return 215; case 108: return 217; case 109: return 219; case 110: return 221; case 111: return 223;
    case 112: return 225; case 113: return 227; case 114: return 229; case 115: return 231; case 116: return 233; case 117: return 235; case 118: return 237; case 119: return 239;
    case 120: return 241; case 121: return 243; case 122: return 245; case 123: return 247; case 124: return 249; case 125: return 251; case 126: return 253; case 127: return 255;
    case 128: return 257; case 129: return 259; case 130: return 261; case 131: return 263; case 132: return 265; case 133: return 267; case 134: return 269; case 135: return 271;
    case 136: return 273; case 137: return 275; case 138: return 277; case 139: return 279; case 140: return 281; case 141: return 283; case 142: return 285; case 143: return 287;
    case 144: return 289; case 145: return 291; case 146: return 293; case 147: return 295; case 148: return 297; case 149: return 299; case 150: return 301; case 151: return 303;
    case 152: return 305; case 153: return 307; case 154: return 309; case 155: return 311; case 156: return 313; case 157: return 315; case 158: return 317; case 159: return 319;
    case 160: return 321; case 161: return 323; case 162: return 325; case 163: return 327; case 164: return 329; case 165: return 331; case 166: return 333; case 167: return 335;
    case 168: return 337; case 169: return 339; case 170: return 341; case 171: return 343; case 172: return 345; case 173: return 347; case 174: return 349; case 175: return 351;
    case 176: return 353; case 177: return 355; case 178: return 357; case 179: return 359; case 180: return 361; case 181: return 363; case 182: return 365; case 183: return 367;
    case 184: return 369; case 185: return 371; case 186: return 373; case 187: return 375; case 188: return 377; case 189: return 379; case 190: return 381; case 191: return 383;
    case 192: return 385; case 193: return 387; case 194: return 389; case 195: return 391; case 196: return 393; case 197: return 395; case 198: return 397; case 199: return 399;
    case 200: return 401; case 201: return 403; case 202: return 405; case 203: return 407; case 204: return 409; case 205: return 411; case 206: return 413; case 207: return 415;
    case 208: return 417; case 209: return 419; case 210: return 421; case 211: return 423; case 212: return 425; case 213: return 427; case 214: return 429; case 215: return 431;
    case 216: return 433; case 217: return 435; case 218: return 437; case 219: return 439; case 220: return 441; case 221: return 443; case 222: return 445; case 223: return 447;
    case 224: return 449; case 225: return 451; case 226: return 453; case 227: return 455; case 228: return 457; case 229: return 459; case 230: return 461; case 231: return 463;
    case 232: return 465; case 233: return 467; case 234: return 469; case 235: return 471; case 236: return 473; case 237: return 475; case 238: return 477; case 239: return 479;
    case 240: return 481; case 241: return 483; case 242: return 485; case 243: return 487; case 244: return 489; case 245: return 491; case 246: return 493; case 247: return 495;
    case 248: return 497; case 249: return 499; case 250: return 501; case 251: return 503; case 252: return 505; case 253: return 507; case 254: return 509; case 255: return 511;
    case 256: return 513; case 257: return 515; case 258: return 517; case 259: return 519; case 260: return 521; case 261: return 523; case 262: return 525; case 263: return 527;
    case 264: return 529; case 265: return 531; case 266: return 533; case 267: return 535; case 268: return 537; case 269: return 539; case 270: return 541; case 271: return 543;
    case 272: return 545; case 273: return 547; case 274: return 549; case 275: return 551; case 276: return 553; case 277: return 555; case 278: return 557; case 279: return 559;
    case 280: return 561; case 281: return 563; case 282: return 565; case 283: return 567; case 284: return 569; case 285: return 571; case 286: return 573; case 287: return 575;
    case 288: return 577; case 289: return 579; case 290: return 581; case 291: return 583; case 292: return 585; case 293: return 587; case 294: return 589; case 295: return 591;
    case 296: return 593; case 297: return 595; case 298: return 597; case 299: return 599; case 300: return 601; case 301: return 603; case 302: return 605; case 303: return 607;
    case 304: return 609; case 305: return 611; case 306: return 613; case 307: return 615; case 308: return 617; case 309: return 619; case 310: return 621; case 311: return 623;
    case 312: return 625; case 313: return 627; case 314: return 629; case 315: return 631; case 316: return 633; case 317: return 635; case 318: return 637; case 319: return 639;
    case 320: return 641; case 321: return 643; case 322: return 645; case 323: return 647; case 324: return 649; case 325: return 651; case 326: return 653; case 327: return 655;
    case 328: return 657; case 329: return 659; case 330: return 661; case 331: return 663; case 332: return 665; case 333: return 667; case 334: return 669; case 335: return 671;
    case 336: return 673; case 337: return 675; case 338: return 677; case 339: return 679; case 340: return 681; case 341: return 683; case 342: return 685; case 343: return 687;
    case 344: return 689; case 345: return 691; case 346: return 693; case 347: return 695; case 348: return 697; case 349: return 699; case 350: return 701; case 351: return 703;
    case 352: return 705; case 353: return 707; case 354: return 709; case 355: return 711; case 356: return 713; case 357: return 715; case 358: return 717; case 359: return 719;
    case 360: return 721; case 361: return 723; case 362: return 725; case 363: return 727; case 364: return 729; case 365: return 731; case 366: return 733; case 367: return 735;
    case 368: return 737; case 369: return 739; case 370: return 741; case 371: return 743; case 372: return 745; case 373: return 747; case 374: return 749; case 375: return 751;
    case 376: return 753; case 377: return 755; case 378: return 757; case 379: return 759; case 380: return 761; case 381: return 763; case 382: return 765; case 383: return 767;
    case 384: return 769; case 385: return 771; case 386: return 773; case 387: return 775; case 388: return 777; case 389: return 779; case 390: return 781; case 391: return 783;
    case 392: return 785; case 393: return 787; case 394: return 789; case 395: return 791; case 396: return 793; case 397: return 795; case 398: return 797; case 399: return 799;
    case 400: return 801; case 401: return 803; case 402: return 805; case 403: return 807; case 404: return 809; case 405: return 811; case 406: return 813; case 407: return 815;
    case 408: return 817; case 409: return 819; case 410: return 821; case 411: return 823; case 412: return 825; case 413: return 827; case 414: return 829; case 415: return 831;
    case 416: return 833; case 417: return 835; case 418: return 837; case 419: return 839; case 420: return 841; case 421: return 843; case 422: return 845; case 423: return 847;
    case 424: return 849; case 425: return 851; case 426: return 853; case 427: return 855; case 428: return 857; case 429: return 859; case 430: return 861; case 431: return 863;
    case 432: return 865; case 433: return 867; case 434: return 869; case 435: return 871; case 436: return 873; case 437: return 875; case 438: return 877; case 439: return 879;
    case 440: return 881; case 441: return 883; case 442: return 885; case 443: return 887; case 444: return 889; case 445: return 891; case 446: return 893; case 447: return 895;
    case 448: return 897; case 449: return 899; case 450: return 901; case 451: return 903; case 452: return 905; case 453: return 907; case 454: return 909; case 455: return 911;
    case 456: return 913; case 457: return 915; case 458: return 917; case 459: return 919; case 460: return 921; case 461: return 923; case 462: return 925; case 463: return 927;
    case 464: return 929; case 465: return 931; case 466: return 933; case 467: return 935; case 468: return 937; case 469: return 939; case 470: return 941; case 471: return 943;
    case 472: return 945; case 473: return 947; case 474: return 949; case 475: return 951; case 476: return 953; case 477: return 955; case 478: return 957; case 479: return 959;
    case 480: return 961; case 481: return 963; case 482: return 965; case 483: return 967; case 484: return 969; case 485: return 971; case 486: return 973; case 487: return 975;
    case 488: return 977; case 489: return 979; case 490: return 981; case 491: return 983; case 492: return 985; case 493: return 987; case 494: return 989; case 495: return 991;
    case 496: return 993; case 497: return 995; case 498: return 997; case 499: return 999; case 500: return 1001; case 501: return 1003; case 502: return 1005; case 503: return 1007;
    case 504: return 1009; case 505: return 1011; case 506: return 1013; case 507: return 1015; case 508: return 1017; case 509: return 1019; case 510: return 1021; case 511: return 1023;
    case 512: return 1025; case 513: return 1027; case 514: return 1029; case 515: return 1031; case 516: return 1033; case 517: return 1035; case 518: return 1037; case 519: return 1039;
    case 520: return 1041; case 521: return 1043; case 522: return 1045; case 523: return 1047; case 524: return 1049; case 525: return 1051; case 526: return 1053; case 527: return 1055;
    case 528: return 1057; case 529: return 1059; case 530: return 1061; case 531: return 1063; case 532: return 1065; case 533: return 1067; case 534: return 1069; case 535: return 1071;
    case 536: return 1073; case 537: return 1075; case 538: return 1077; case 539: return 1079; case 540: return 1081; case 541: return 1083; case 542: return 1085; case 543: return 1087;
    case 544: return 1089; case 545: return 1091; case 546: return 1093; case 547: return 1095; case 548: return 1097; case 549: return 1099; case 550: return 1101; case 551: return 1103;
    case 552: return 1105; case 553: return 1107; case 554: return 1109; case 555: return 1111; case 556: return 1113; case 557: return 1115; case 558: return 1117; case 559: return 1119;
    case 560: return 1121; case 561: return 1123; case 562: return 1125; case 563: return 1127; case 564: return 1129; case 565: return 1131; case 566: return 1133; case 567: return 1135;
    case 568: return 1137; case 569: return 1139; case 570: return 1141; case 571: return 1143; case 572: return 1145; case 573: return 1147; case 574: return 1149; case 575: return 1151;
    case 576: return 1153; case 577: return 1155; case 578: return 1157; case 579: return 1159; case 580: return 1161; case 581: return 1163; case 582: return 1165; case 583: return 1167;
    case 584: return 1169; case 585: return 1171; case 586: return 1173; case 587: return 1175; case 588: return 1177; case 589: return 1179; case 590: return 1181; case 591: return 1183;
    case 592: return 1185; case 593: return 1187; case 594: return 1189; case 595: return 1191; case 596: return 1193; case 597: return 1195; case 598: return 1197; case 599: return 1199;
    case 600: return 1201; case 601: return 1203; case 602: return 1205; case 603: return 1207; case 604: return 1209; case 605: return 1211; case 606: return 1213; case 607: return 1215;
    case 608: return 1217; case 609: return 1219; case 610: return 1221; case 611: return 1223; case 612: return 1225; case 613: return 1227; case 614: return 1229; case 615: return 1231;
    case 616: return 1233; case 617: return 1235; case 618: return 1237; case 619: return 1239; case 620: return 1241; case 621: return 1243; case 622: return 1245; case 623: return 1247;
    case 624: return 1249; case 625: return 1251; case 626: return 1253; case 627: return 1255; case 628: return 1257; case 629: return 1259; case 630: return 1261; case 631: return 1263;
    case 632: return 1265; case 633: return 1267; case 634: return 1269; case 635: return 1271; case 636: return 1273; case 637: return 1275; case 638: return 1277; case 639: return 1279;
    case 640: return 1281; case 641: return 1283; case 642: return 1285; case 643: return 1287; case 644: return 1289; case 645: return 1291; case 646: return 1293; case 647: return 1295;
    case 648: return 1297; case 649: return 1299; case 650: return 1301; case 651: return 1303; case 652: return 1305; case 653: return 1307; case 654: return 1309; case 655: return 1311;
    case 656: return 1313; case 657: return 1315; case 658: return 1317; case 659: return 1319; case 660: return 1321; case 661: return 1323; case 662: return 1325; case 663: return 1327;
    case 664: return 1329; case 665: return 1331; case 666: return 1333; case 667: return 1335; case 668: return 1337; case 669: return 1339; case 670: return 1341; case 671: return 1343;
    case 672: return 1345; case 673: return 1347; case 674: return 1349; case 675: return 1351; case 676: return 1353; case 677: return 1355; case 678: return 1357; case 679: return 1359;
    case 680: return 1361; case 681: return 1363; case 682: return 1365; case 683: return 1367; case 684: return 1369; case 685: return 1371; case 686: return 1373; case 687: return 1375;
    case 688: return 1377; case 689: return 1379; case 690: return 1381; case 691: return 1383; case 692: return 1385; case 693: return 1387; case 694: return 1389; case 695: return 1391;
    case 696: return 1393; case 697: return 1395; case 698: return 1397; case 699: return 1399; case 700: return 1401; case 701: return 1403; case 702: return 1405; case 703: return 1407;
    case 704: return 1409; case 705: return 1411; case 706: return 1413; case 707: return 1415; case 708: return 1417; case 709: return 1419; case 710: return 1421; case 711: return 1423;
    case 712: return 1425; case 713: return 1427; case 714: return 1429; case 715: return 1431; case 716: return 1433; case 717: return 1435; case 718: return 1437; case 719: return 1439;
    case 720: return 1441; case 721: return 1443; case 722: return 1445; case 723: return 1447; case 724: return 1449; case 725: return 1451; case 726: return 1453; case 727: return 1455;
    case 728: return 1457; case 729: return 1459; case 730: return 1461; case 731: return 1463; case 732: return 1465; case 733: return 1467; case 734: return 1469; case 735: return 1471;
    case 736: return 1473; case 737: return 1475; case 738: return 1477; case 739: return 1479; case 740: return 1481; case 741: return 1483; case 742: return 1485; case 743: return 1487;
    case 744: return 1489; case 745: return 1491; case 746: return 1493; case 747: return 1495; case 748: return 1497; case 749: return 1499; case 750: return 1501; case 751: return 1503;
    case 752: return 1505; case 753: return 1507; case 754: return 1509; case 755: return 1511; case 756: return 1513; case 757: return 1515; case 758: return 1517; case 759: return 1519;
    case 760: return 1521; case 761: return 1523; case 762: return 1525; case 763: return 1527; case 764: return 1529; case 765: return 1531; case 766: return 1533; case 767: return 1535;
    case 768: return 1537; case 769: return 1539; case 770: return 1541; case 771: return 1543; case 772: return 1545; case 773: return 1547; case 774: return 1549; case 775: return 1551;
    case 776: return 1553; case 777: return 1555; case 778: return 1557; case 779: return 1559; case 780: return 1561; case 781: return 1563; case 782: return 1565; case 783: return 1567;
    case 784: return 1569; case 785: return 1571; case 786: return 1573; case 787: return 1575; case 788: return 1577; case 789: return 1579; case 790: return 1581; case 791: return 1583;
    case 792: return 1585; case 793: return 1587; case 794: return 1589; case 795: return 1591; case 796: return 1593; case 797: return 1595; case 798: return 1597; case 799: return 1599;
    case 800: return 1601; case 801: return 1603; case 802: return 1605; case 803: return 1607; case 804: return 1609; case 805: return 1611; case 806: return 1613; case 807: return 1615;
    case 808: return 1617; case 809: return 1619; case 810: return 1621; case 811: return 1623; case 812: return 1625; case 813: return 1627; case 814: return 1629; case 815: return 1631;
    case 816: return 1633; case 817: return 1635; case 818: return 1637; case 819: return 1639; case 820: return 1641; case 821: return 1643; case 822: return 1645; case 823: return 1647;
    case 824: return 1649; case 825: return 1651; case 826: return 1653; case 827: return 1655; case 828: return 1657; case 829: return 1659; case 830: return 1661; case 831: return 1663;
    case 832: return 1665; case 833: return 1667; case 834: return 1669; case 835: return 1671; case 836: return 1673; case 837: return 1675; case 838: return 1677; case 839: return 1679;
    case 840: return 1681; case 841: return 1683; case 842: return 1685; case 843: return 1687; case 844: return 1689; case 845: return 1691; case 846: return 1693; case 847: return 1695;
    case 848: return 1697; case 849: return 1699; case 850: return 1701; case 851: return 1703; case 852: return 1705; case 853: return 1707; case 854: return 1709; case 855: return 1711;
    case 856: return 1713; case 857: return 1715; case 858: return 1717; case 859: return 1719; case 860: return 1721; case 861: return 1723; case 862: return 1725; case 863: return 1727;
    case 864: return 1729; case 865: return 1731; case 866: return 1733; case 867: return 1735; case 868: return 1737; case 869: return 1739; case 870: return 1741; case 871: return 1743;
    case 872: return 1745; case 873: return 1747; case 874: return 1749; case 875: return 1751; case 876: return 1753; case 877: return 1755; case 878: return 1757; case 879: return 1759;
    case 880: return 1761; case 881: return 1763; case 882: return 1765; case 883: return 1767; case 884: return 1769; case 885: return 1771; case 886: return 1773; case 887: return 1775;
    case 888: return 1777; case 889: return 1779; case 890: return 1781; case 891: return 1783; case 892: return 1785; case 893: return 1787; case 894: return 1789; case 895: return 1791;
    case 896: return 1793; case 897: return 1795; case 898: return 1797; case 899: return 1799; case 900: return 1801; case 901: return 1803; case 902: return 1805; case 903: return 1807;
    case 904: return 1809; case 905: return 1811; case 906: return 1813; case 907: return 1815; case 908: return 1817; case 909: return 1819; case 910: return 1821; case 911: return 1823;
    case 912: return 1825; case 913: return 1827; case 914: return 1829; case 915: return 1831; case 916: return 1833; case 917: return 1835; case 918: return 1837; case 919: return 1839;
    case 920: return 1841; case 921: return 1843; case 922: return 1845; case 923: return 1847; case 924: return 1849; case 925: return 1851; case 926: return 1853; case 927: return 1855;
    case 928: return 1857; case 929: return 1859; case 930: return 1861; case 931: return 1863; case 932: return 1865; case 933: return 1867; case 934: return 1869; case 935: return 1871;
    case 936: return 1873; case 937: return 1875; case 938: return 1877; case 939: return 1879; case 940: return 1881; case 941: return 1883; case 942: return 1885; case 943: return 1887;
    case 944: return 1889; case 945: return 1891; case 946: return 1893; case 947: return 1895; case 948: return 1897; case 949: return 1899; case 950: return 1901; case 951: return 1903;
    case 952: return 1905; case 953: return 1907; case 954: return 1909; case 955: return 1911; case 956: return 1913; case 957: return 1915; case 958: return 1917; case 959: return 1919;
    case 960: return 1921; case 961: return 1923; case 962: return 1925; case 963: return 1927; case 964: return 1929; case 965: return 1931; case 966: return 1933; case 967: return 1935;
    case 968: return 1937; case 969: return 1939; case 970: return 1941; case 971: return 1943; case 972: return 1945; case 973: return 1947; case 974: return 1949; case 975: return 1951;
    case 976: return 1953; case 977: return 1955; case 978: return 1957; case 979: return 1959; case 980: return 1961; case 981: return 1963; case 982: return 1965; case 983: return 1967;
    case 984: return 1969; case 985: return 1971; case 986: return 1973; case 987: return 1975; case 988: return 1977; case 989: return 1979; case 990: return 1981; case 991: return 1983;
    case 992: return 1985; case 993: return 1987; case 994: return 1989; case 995: return 1991; case 996: return 1993; case 997: return 1995; case 998: return 1997; case 999: return 1999;
    case 1000: return 2001; case 1001: return 2003; case 1002: return 2005; case 1003: return 2007; case 1004: return 2009; case 1005: return 2011; case 1006: return 2013; case 1007: return 2015;
    case 1008: return 2017; case 1009: return 2019; case 1010: return 2021; case 1011: return 2023; case 1012: return 2025; case 1013: return 2027; case 1014: return 2029; case 1015: return 2031;
    case 1016: return 2033; case 1017: return 2035; case 1018: return 2037; case 1019: return 2039; case 1020: return 2041; case 1021: return 2043; case 1022: return 2045;
    default: return -1;
    }
}

/* 127 parameters in one function definition */
static long sum127(int p0, int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11, int p12, int p13, int p14, int p15,
                   int p16, int p17, int p18, int p19, int p20, int p21, int p22, int p23, int p24, int p25, int p26, int p27, int p28, int p29, int p30, int p31,
                   int p32, int p33, int p34, int p35, int p36, int p37, int p38, int p39, int p40, int p41, int p42, int p43, int p44, int p45, int p46, int p47,
                   int p48, int p49, int p50, int p51, int p52, int p53, int p54, int p55, int p56, int p57, int p58, int p59, int p60, int p61, int p62, int p63,
                   int p64, int p65, int p66, int p67, int p68, int p69, int p70, int p71, int p72, int p73, int p74, int p75, int p76, int p77, int p78, int p79,
                   int p80, int p81, int p82, int p83, int p84, int p85, int p86, int p87, int p88, int p89, int p90, int p91, int p92, int p93, int p94, int p95,
                   int p96, int p97, int p98, int p99, int p100, int p101, int p102, int p103, int p104, int p105, int p106, int p107, int p108, int p109, int p110, int p111,
                   int p112, int p113, int p114, int p115, int p116, int p117, int p118, int p119, int p120, int p121, int p122, int p123, int p124, int p125, int p126)
{
    long s = 0;
    s += p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9 + p10 + p11 + p12 + p13 + p14 + p15;
    s += p16 + p17 + p18 + p19 + p20 + p21 + p22 + p23 + p24 + p25 + p26 + p27 + p28 + p29 + p30 + p31;
    s += p32 + p33 + p34 + p35 + p36 + p37 + p38 + p39 + p40 + p41 + p42 + p43 + p44 + p45 + p46 + p47;
    s += p48 + p49 + p50 + p51 + p52 + p53 + p54 + p55 + p56 + p57 + p58 + p59 + p60 + p61 + p62 + p63;
    s += p64 + p65 + p66 + p67 + p68 + p69 + p70 + p71 + p72 + p73 + p74 + p75 + p76 + p77 + p78 + p79;
    s += p80 + p81 + p82 + p83 + p84 + p85 + p86 + p87 + p88 + p89 + p90 + p91 + p92 + p93 + p94 + p95;
    s += p96 + p97 + p98 + p99 + p100 + p101 + p102 + p103 + p104 + p105 + p106 + p107 + p108 + p109 + p110 + p111;
    s += p112 + p113 + p114 + p115 + p116 + p117 + p118 + p119 + p120 + p121 + p122 + p123 + p124 + p125 + p126;
    return s;
}

int main(void)
{
    if (deep_blocks() != 127) return 1;          /* 127 nested blocks */
    if (COND_INCL_63 != 1) return 2;             /* 63 nested #if */

    /* 63 nesting levels of parenthesized expressions */
    {
        int v = (((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((1)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));
        if (v != 1) return 3;
    }

    /* 12 pointer declarators modifying a basic type */
    {
        int t = 99;
        int *d1 = &t;
        int **d2 = &d1;
        int ***d3 = &d2;
        int ****d4 = &d3;
        int *****d5 = &d4;
        int ******d6 = &d5;
        int *******d7 = &d6;
        int ********d8 = &d7;
        int *********d9 = &d8;
        int **********d10 = &d9;
        int ***********d11 = &d10;
        int ************d12 = &d11;
        if (************d12 != 99) return 4;
    }

    if (iabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijab != 63) return 5;                       /* 63-char identifier */

    if (big_switch(511) != 1023) return 6;       /* 1023 case labels */
    if (big_switch(0) != 1 || big_switch(1022) != 2045) return 7;

    if (K0 != 0 || K511 != 511 || K1022 != 1022) return 8;  /* 1023 enum constants */

    sb.m0 = 7; sb.m511 = 21; sb.m1022 = 42;      /* 1023 struct members */
    if (sb.m0 != 7 || sb.m511 != 21 || sb.m1022 != 42) return 9;

    /* 127 arguments in one function call (sum 0..126 == 8001) */
    if (sum127(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
               16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
               32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
               48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
               64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
               80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
               96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
               112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126) != 8001) return 10;

    /* 127 arguments in one macro invocation */
    if (SUM127(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
               16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
               32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
               48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
               64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
               80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
               96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
               112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126) != 8001) return 11;

    /* 511 identifiers with block scope in one block */
    {
        int b0 = 0, b1 = 1, b2 = 2, b3 = 3, b4 = 4, b5 = 5, b6 = 6, b7 = 7, b8 = 8, b9 = 9, b10 = 10, b11 = 11, b12 = 12, b13 = 13, b14 = 14, b15 = 15;
        int b16 = 16, b17 = 17, b18 = 18, b19 = 19, b20 = 20, b21 = 21, b22 = 22, b23 = 23, b24 = 24, b25 = 25, b26 = 26, b27 = 27, b28 = 28, b29 = 29, b30 = 30, b31 = 31;
        int b32 = 32, b33 = 33, b34 = 34, b35 = 35, b36 = 36, b37 = 37, b38 = 38, b39 = 39, b40 = 40, b41 = 41, b42 = 42, b43 = 43, b44 = 44, b45 = 45, b46 = 46, b47 = 47;
        int b48 = 48, b49 = 49, b50 = 50, b51 = 51, b52 = 52, b53 = 53, b54 = 54, b55 = 55, b56 = 56, b57 = 57, b58 = 58, b59 = 59, b60 = 60, b61 = 61, b62 = 62, b63 = 63;
        int b64 = 64, b65 = 65, b66 = 66, b67 = 67, b68 = 68, b69 = 69, b70 = 70, b71 = 71, b72 = 72, b73 = 73, b74 = 74, b75 = 75, b76 = 76, b77 = 77, b78 = 78, b79 = 79;
        int b80 = 80, b81 = 81, b82 = 82, b83 = 83, b84 = 84, b85 = 85, b86 = 86, b87 = 87, b88 = 88, b89 = 89, b90 = 90, b91 = 91, b92 = 92, b93 = 93, b94 = 94, b95 = 95;
        int b96 = 96, b97 = 97, b98 = 98, b99 = 99, b100 = 100, b101 = 101, b102 = 102, b103 = 103, b104 = 104, b105 = 105, b106 = 106, b107 = 107, b108 = 108, b109 = 109, b110 = 110, b111 = 111;
        int b112 = 112, b113 = 113, b114 = 114, b115 = 115, b116 = 116, b117 = 117, b118 = 118, b119 = 119, b120 = 120, b121 = 121, b122 = 122, b123 = 123, b124 = 124, b125 = 125, b126 = 126, b127 = 127;
        int b128 = 128, b129 = 129, b130 = 130, b131 = 131, b132 = 132, b133 = 133, b134 = 134, b135 = 135, b136 = 136, b137 = 137, b138 = 138, b139 = 139, b140 = 140, b141 = 141, b142 = 142, b143 = 143;
        int b144 = 144, b145 = 145, b146 = 146, b147 = 147, b148 = 148, b149 = 149, b150 = 150, b151 = 151, b152 = 152, b153 = 153, b154 = 154, b155 = 155, b156 = 156, b157 = 157, b158 = 158, b159 = 159;
        int b160 = 160, b161 = 161, b162 = 162, b163 = 163, b164 = 164, b165 = 165, b166 = 166, b167 = 167, b168 = 168, b169 = 169, b170 = 170, b171 = 171, b172 = 172, b173 = 173, b174 = 174, b175 = 175;
        int b176 = 176, b177 = 177, b178 = 178, b179 = 179, b180 = 180, b181 = 181, b182 = 182, b183 = 183, b184 = 184, b185 = 185, b186 = 186, b187 = 187, b188 = 188, b189 = 189, b190 = 190, b191 = 191;
        int b192 = 192, b193 = 193, b194 = 194, b195 = 195, b196 = 196, b197 = 197, b198 = 198, b199 = 199, b200 = 200, b201 = 201, b202 = 202, b203 = 203, b204 = 204, b205 = 205, b206 = 206, b207 = 207;
        int b208 = 208, b209 = 209, b210 = 210, b211 = 211, b212 = 212, b213 = 213, b214 = 214, b215 = 215, b216 = 216, b217 = 217, b218 = 218, b219 = 219, b220 = 220, b221 = 221, b222 = 222, b223 = 223;
        int b224 = 224, b225 = 225, b226 = 226, b227 = 227, b228 = 228, b229 = 229, b230 = 230, b231 = 231, b232 = 232, b233 = 233, b234 = 234, b235 = 235, b236 = 236, b237 = 237, b238 = 238, b239 = 239;
        int b240 = 240, b241 = 241, b242 = 242, b243 = 243, b244 = 244, b245 = 245, b246 = 246, b247 = 247, b248 = 248, b249 = 249, b250 = 250, b251 = 251, b252 = 252, b253 = 253, b254 = 254, b255 = 255;
        int b256 = 256, b257 = 257, b258 = 258, b259 = 259, b260 = 260, b261 = 261, b262 = 262, b263 = 263, b264 = 264, b265 = 265, b266 = 266, b267 = 267, b268 = 268, b269 = 269, b270 = 270, b271 = 271;
        int b272 = 272, b273 = 273, b274 = 274, b275 = 275, b276 = 276, b277 = 277, b278 = 278, b279 = 279, b280 = 280, b281 = 281, b282 = 282, b283 = 283, b284 = 284, b285 = 285, b286 = 286, b287 = 287;
        int b288 = 288, b289 = 289, b290 = 290, b291 = 291, b292 = 292, b293 = 293, b294 = 294, b295 = 295, b296 = 296, b297 = 297, b298 = 298, b299 = 299, b300 = 300, b301 = 301, b302 = 302, b303 = 303;
        int b304 = 304, b305 = 305, b306 = 306, b307 = 307, b308 = 308, b309 = 309, b310 = 310, b311 = 311, b312 = 312, b313 = 313, b314 = 314, b315 = 315, b316 = 316, b317 = 317, b318 = 318, b319 = 319;
        int b320 = 320, b321 = 321, b322 = 322, b323 = 323, b324 = 324, b325 = 325, b326 = 326, b327 = 327, b328 = 328, b329 = 329, b330 = 330, b331 = 331, b332 = 332, b333 = 333, b334 = 334, b335 = 335;
        int b336 = 336, b337 = 337, b338 = 338, b339 = 339, b340 = 340, b341 = 341, b342 = 342, b343 = 343, b344 = 344, b345 = 345, b346 = 346, b347 = 347, b348 = 348, b349 = 349, b350 = 350, b351 = 351;
        int b352 = 352, b353 = 353, b354 = 354, b355 = 355, b356 = 356, b357 = 357, b358 = 358, b359 = 359, b360 = 360, b361 = 361, b362 = 362, b363 = 363, b364 = 364, b365 = 365, b366 = 366, b367 = 367;
        int b368 = 368, b369 = 369, b370 = 370, b371 = 371, b372 = 372, b373 = 373, b374 = 374, b375 = 375, b376 = 376, b377 = 377, b378 = 378, b379 = 379, b380 = 380, b381 = 381, b382 = 382, b383 = 383;
        int b384 = 384, b385 = 385, b386 = 386, b387 = 387, b388 = 388, b389 = 389, b390 = 390, b391 = 391, b392 = 392, b393 = 393, b394 = 394, b395 = 395, b396 = 396, b397 = 397, b398 = 398, b399 = 399;
        int b400 = 400, b401 = 401, b402 = 402, b403 = 403, b404 = 404, b405 = 405, b406 = 406, b407 = 407, b408 = 408, b409 = 409, b410 = 410, b411 = 411, b412 = 412, b413 = 413, b414 = 414, b415 = 415;
        int b416 = 416, b417 = 417, b418 = 418, b419 = 419, b420 = 420, b421 = 421, b422 = 422, b423 = 423, b424 = 424, b425 = 425, b426 = 426, b427 = 427, b428 = 428, b429 = 429, b430 = 430, b431 = 431;
        int b432 = 432, b433 = 433, b434 = 434, b435 = 435, b436 = 436, b437 = 437, b438 = 438, b439 = 439, b440 = 440, b441 = 441, b442 = 442, b443 = 443, b444 = 444, b445 = 445, b446 = 446, b447 = 447;
        int b448 = 448, b449 = 449, b450 = 450, b451 = 451, b452 = 452, b453 = 453, b454 = 454, b455 = 455, b456 = 456, b457 = 457, b458 = 458, b459 = 459, b460 = 460, b461 = 461, b462 = 462, b463 = 463;
        int b464 = 464, b465 = 465, b466 = 466, b467 = 467, b468 = 468, b469 = 469, b470 = 470, b471 = 471, b472 = 472, b473 = 473, b474 = 474, b475 = 475, b476 = 476, b477 = 477, b478 = 478, b479 = 479;
        int b480 = 480, b481 = 481, b482 = 482, b483 = 483, b484 = 484, b485 = 485, b486 = 486, b487 = 487, b488 = 488, b489 = 489, b490 = 490, b491 = 491, b492 = 492, b493 = 493, b494 = 494, b495 = 495;
        int b496 = 496, b497 = 497, b498 = 498, b499 = 499, b500 = 500, b501 = 501, b502 = 502, b503 = 503, b504 = 504, b505 = 505, b506 = 506, b507 = 507, b508 = 508, b509 = 509, b510 = 510;
        long s = 0;
        s += b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8 + b9 + b10 + b11 + b12 + b13 + b14 + b15;
        s += b16 + b17 + b18 + b19 + b20 + b21 + b22 + b23 + b24 + b25 + b26 + b27 + b28 + b29 + b30 + b31;
        s += b32 + b33 + b34 + b35 + b36 + b37 + b38 + b39 + b40 + b41 + b42 + b43 + b44 + b45 + b46 + b47;
        s += b48 + b49 + b50 + b51 + b52 + b53 + b54 + b55 + b56 + b57 + b58 + b59 + b60 + b61 + b62 + b63;
        s += b64 + b65 + b66 + b67 + b68 + b69 + b70 + b71 + b72 + b73 + b74 + b75 + b76 + b77 + b78 + b79;
        s += b80 + b81 + b82 + b83 + b84 + b85 + b86 + b87 + b88 + b89 + b90 + b91 + b92 + b93 + b94 + b95;
        s += b96 + b97 + b98 + b99 + b100 + b101 + b102 + b103 + b104 + b105 + b106 + b107 + b108 + b109 + b110 + b111;
        s += b112 + b113 + b114 + b115 + b116 + b117 + b118 + b119 + b120 + b121 + b122 + b123 + b124 + b125 + b126 + b127;
        s += b128 + b129 + b130 + b131 + b132 + b133 + b134 + b135 + b136 + b137 + b138 + b139 + b140 + b141 + b142 + b143;
        s += b144 + b145 + b146 + b147 + b148 + b149 + b150 + b151 + b152 + b153 + b154 + b155 + b156 + b157 + b158 + b159;
        s += b160 + b161 + b162 + b163 + b164 + b165 + b166 + b167 + b168 + b169 + b170 + b171 + b172 + b173 + b174 + b175;
        s += b176 + b177 + b178 + b179 + b180 + b181 + b182 + b183 + b184 + b185 + b186 + b187 + b188 + b189 + b190 + b191;
        s += b192 + b193 + b194 + b195 + b196 + b197 + b198 + b199 + b200 + b201 + b202 + b203 + b204 + b205 + b206 + b207;
        s += b208 + b209 + b210 + b211 + b212 + b213 + b214 + b215 + b216 + b217 + b218 + b219 + b220 + b221 + b222 + b223;
        s += b224 + b225 + b226 + b227 + b228 + b229 + b230 + b231 + b232 + b233 + b234 + b235 + b236 + b237 + b238 + b239;
        s += b240 + b241 + b242 + b243 + b244 + b245 + b246 + b247 + b248 + b249 + b250 + b251 + b252 + b253 + b254 + b255;
        s += b256 + b257 + b258 + b259 + b260 + b261 + b262 + b263 + b264 + b265 + b266 + b267 + b268 + b269 + b270 + b271;
        s += b272 + b273 + b274 + b275 + b276 + b277 + b278 + b279 + b280 + b281 + b282 + b283 + b284 + b285 + b286 + b287;
        s += b288 + b289 + b290 + b291 + b292 + b293 + b294 + b295 + b296 + b297 + b298 + b299 + b300 + b301 + b302 + b303;
        s += b304 + b305 + b306 + b307 + b308 + b309 + b310 + b311 + b312 + b313 + b314 + b315 + b316 + b317 + b318 + b319;
        s += b320 + b321 + b322 + b323 + b324 + b325 + b326 + b327 + b328 + b329 + b330 + b331 + b332 + b333 + b334 + b335;
        s += b336 + b337 + b338 + b339 + b340 + b341 + b342 + b343 + b344 + b345 + b346 + b347 + b348 + b349 + b350 + b351;
        s += b352 + b353 + b354 + b355 + b356 + b357 + b358 + b359 + b360 + b361 + b362 + b363 + b364 + b365 + b366 + b367;
        s += b368 + b369 + b370 + b371 + b372 + b373 + b374 + b375 + b376 + b377 + b378 + b379 + b380 + b381 + b382 + b383;
        s += b384 + b385 + b386 + b387 + b388 + b389 + b390 + b391 + b392 + b393 + b394 + b395 + b396 + b397 + b398 + b399;
        s += b400 + b401 + b402 + b403 + b404 + b405 + b406 + b407 + b408 + b409 + b410 + b411 + b412 + b413 + b414 + b415;
        s += b416 + b417 + b418 + b419 + b420 + b421 + b422 + b423 + b424 + b425 + b426 + b427 + b428 + b429 + b430 + b431;
        s += b432 + b433 + b434 + b435 + b436 + b437 + b438 + b439 + b440 + b441 + b442 + b443 + b444 + b445 + b446 + b447;
        s += b448 + b449 + b450 + b451 + b452 + b453 + b454 + b455 + b456 + b457 + b458 + b459 + b460 + b461 + b462 + b463;
        s += b464 + b465 + b466 + b467 + b468 + b469 + b470 + b471 + b472 + b473 + b474 + b475 + b476 + b477 + b478 + b479;
        s += b480 + b481 + b482 + b483 + b484 + b485 + b486 + b487 + b488 + b489 + b490 + b491 + b492 + b493 + b494 + b495;
        s += b496 + b497 + b498 + b499 + b500 + b501 + b502 + b503 + b504 + b505 + b506 + b507 + b508 + b509 + b510;
        if (s != 130305) return 12;   /* sum 0..510 */
    }

    /* a string literal of 4095 characters (sizeof counts the null) */
    {
        static const char s4095[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        if (sizeof s4095 != 4096) return 13;
        if (s4095[0] != 'x' || s4095[4094] != 'x' || s4095[4095] != 0) return 14;
    }

    /* an object of 65535 bytes */
    if (sizeof big_obj != 65535) return 15;
    big_obj[0] = 1; big_obj[65534] = 2;
    if (big_obj[0] != 1 || big_obj[65534] != 2) return 16;

    return 0;
}
