#include "chineselunar.h"
#define ERANGE 34
#define EINVAL 22

const char *const jieqi_names[] = {
	"立春", "雨水", "惊蛰", "春分", "清明", "谷雨",
	"立夏", "小满", "芒种", "夏至", "小暑", "大暑",
	"立秋", "处暑", "白露", "秋分", "寒露", "霜降",
	"立冬", "小雪", "大雪", "冬至", "小寒", "大寒"
};

/* -------------- database ---------------- */

// 数据来源：香港天文台
// https://www.hko.gov.hk/tc/gts/time/conversion1_text.htm

static const struct cl_date solar_base = { 1901,  1,  1, };
static const struct cl_date lunar_base = { 1900, 11, 11, };
static const struct cl_date solar_last = { 2100, 12, 31, };
static const struct cl_date lunar_last = { 2100, 12,  1, };
static const struct cl_date jieqi_base = { 1900, 11, 16, 0, 23 };

/*
 * 32bits per year.
 *     0-18, 19bit, offset days to solar base
 *    19-31, 13bit, days of month (including lunar month),
 *                  mask, 0: 29days, 1: 30days.
 */
#define OFF_MASK	0x7ffff
#define DAY_SHIFT	19
static const unsigned lunar_years[1+200] = {
	0x40000000,
	0x3a900031, 0x75280193, 0x595002f6, 0x32580475,
	0x54d805d7, 0xd530073a, 0x2b5008ba, 0x5ac80a1c,
	0x5d500b7f, 0x3a900cff, 0x6d280e61, 0x59280fe1,
	0x52581143, 0xd25812a5, 0x15681425, 0x2b581587,
	0x2da816ea, 0x6d48186a, 0xf49019cd, 0x74901b4d,
	0x69281caf, 0x69681e11, 0x52b01f91, 0x15b020f3,
	0xd6a82255, 0x36a023d6, 0x75482538, 0x7a50269b,
	0x7490281b, 0x3530297d, 0x29582afc, 0x52b82c5e,
	0xcab02dc1, 0x5ad02f41, 0x36a030a4, 0xbb083206,
	0x3a483386, 0xd89834e8, 0x54983668, 0x295837ca,
	0xa8d8392c, 0x55683aac, 0x2b503c0f, 0xed283d71,
	0x5d203ef2, 0x5a484054, 0x6a5841b6, 0x54a84336,
	0x55684498, 0x29b04618, 0x5568477a, 0xd65048dd,
	0x2d904a5d, 0x6d284bbf, 0xf5104d22, 0x6a504ea2,
	0x2ca85004, 0x54b85183, 0x2ab052e6, 0x2ba85448,
	0x56a855c8, 0x3690572b, 0x3aa8588d, 0x75285a0d,
	0x32505b70, 0x32785cd1, 0x54d85e51, 0x56d05fb4,
	0x2b506134, 0x5b486296, 0x5d9063f9, 0x5a906579,
	0x592866db, 0x5958683d, 0x525869bd, 0x55586b1f,
	0x15686c9f, 0x2b686e01, 0xad486f64, 0x6d4870e4,
	0x6c907247, 0x74a873a9, 0x69287529, 0x7268768b,
	0x52b0780b, 0x15b0796d, 0x17a87acf, 0x36a87c4f,
	0x75487db2, 0x7a907f15, 0x74908095, 0x693081f7,
	0x29708359, 0x52b884d8, 0x56b0863b, 0x1ad087bb,
	0x36a8891d, 0x5b488a80, 0x3a488c00, 0x34988d62,
	0x54d88ec4, 0x29589044, 0x52d891a6, 0x55709309,
	0x2b509489, 0x6ea895eb, 0x5d20976c, 0x5a4898ce,
	0x6a989a30, 0x54a89bb0, 0x29689d12, 0x2ae89e74,
	0x55a89ff4, 0x5d50a157, 0x2e90a2d7, 0x6d28a439,
	0xf450a59c, 0x6a50a71c, 0x64a8a87e, 0x54f0a9e0,
	0x2ab0ab60, 0x55a8acc2, 0x56d0ae25, 0x3690afa5,
	0x3b28b107, 0x3928b287, 0x3258b3e9, 0x32b8b54b,
	0x6558b6cb, 0x2ad0b82e, 0x2b70b990, 0x5b48bb10,
	0x7a90bc73, 0x5a90bdf3, 0x5928bf55, 0xe858c0b7,
	0x5258c237, 0x2558c399, 0x15d8c4fb, 0x2d68c67b,
	0x5b50c7de, 0x6d50c941, 0x6c90cac1, 0x7528cc23,
	0x6928cda3, 0x52a8cf05, 0xd268d067, 0x25b0d1e7,
	0x2da8d349, 0xb690d4ac, 0x7648d62c, 0x7c90d78f,
	0x7490d90f, 0x6930da71, 0xa8b0dbd3, 0x52b8dd52,
	0x2ab0deb5, 0x9b28e017, 0x3aa8e197, 0x3a48e2fa,
	0x3a58e45c, 0x3498e5dc, 0x5558e73e, 0x2958e8be,
	0x52d8ea20, 0x55d0eb83, 0x2b50ed03, 0x5b28ee65,
	0x5d50efc8, 0x5a50f148, 0x6ca8f2aa, 0x54a8f42a,
	0x2968f58c, 0x2b68f6ee, 0x55a8f86e, 0x2d50f9d1,
	0x2ea8fb33, 0x6d28fcb3, 0x6a50fe16, 0x7268ff78,
	0x64b100f8, 0x6671025a, 0x2ab103da, 0x55a9053c,
	0xd691069f, 0x3691081f, 0x75290981, 0x39510ae4,
	0x34590c63, 0x34b90dc5, 0x25590f45, 0x2ad910a7,
	0xaab1120a, 0x5b51138a, 0x3a9114ed, 0x5ca9164f,
	0x5a2917cf, 0x54591931, 0x52791a93, 0x25591c13,
};

/*
 * store leap month of each years.
 * 0, no leap; 1-12 leap month. needs 4bits
 * 8 years per 32bits.
 */
static const unsigned leap_month[200/8+1] = {
	0x04005000, 0x05006020, 0x05007020, 0x06020040,
	0x07030050, 0x20040060, 0x30050070, 0x40060080,
	0x50070300, 0x60080400, 0x600a0400, 0x80300500,
	0x00400500, 0x00500702, 0x00600904, 0x00602004,
	0x00b03005, 0x02005006, 0x03005007, 0x04006008,
	0x05007030, 0x06008040, 0x07030040, 0x08040050,
	0x20040060, 0x00000000
};

/*
 * store the margin between two jieqi.
 * 0: 14days, 1: 15days, 2: 16days. need 2bits
 */
static const unsigned jieqi_span[200*24/16] = {
	0xa6999544, 0x65151559, 0x55599a69, 0x6a665554,
	0x55515556, 0x456669a6, 0xa6999545, 0x65151559,
	0x55599a99, 0x6a665554, 0x55515556, 0x456669a6,
	0xa6999545, 0x65151559, 0x55599a99, 0x6a665554,
	0x55515556, 0x456669a6, 0xa6999551, 0x95451559,
	0x55599a99, 0x9a695854, 0x55545556, 0x519669a6,
	0xa6a59551, 0x95454565, 0x15599a99, 0x9a695855,
	0x55545556, 0x519669a6, 0xa6a65551, 0x95454565,
	0x15599a99, 0x9a696515, 0x55545556, 0x519669a6,
	0xa6a65551, 0x95454565, 0x1559a699, 0x9a696515,
	0x55545559, 0x55566a66, 0x69a65551, 0x95454566,
	0x1559a699, 0x9a696515, 0x55545559, 0x55566a66,
	0x69a65551, 0x95454566, 0x1559a699, 0x9a996515,
	0x55545559, 0x55566a66, 0x69a65551, 0x95454566,
	0x1559a699, 0x9a996515, 0x55545559, 0x55566a66,
	0x69a65551, 0x95454566, 0x1559a699, 0x9a996515,
	0x55545559, 0x55566a69, 0x69a65551, 0x95515196,
	0x1565a699, 0x9a999545, 0x58545559, 0x55569a69,
	0x69a65554, 0x55515196, 0x1565a6a6, 0x9a999545,
	0x58545559, 0x55569a69, 0x69a65554, 0x55515196,
	0x4565a6a6, 0xa6999545, 0x64551559, 0x55569a69,
	0x69a65554, 0x55515196, 0x456669a6, 0xa6999545,
	0x65151559, 0x55599a69, 0x6a665554, 0x55515556,
	0x456669a6, 0xa6999545, 0x65151559, 0x55599a69,
	0x6a665554, 0x55515556, 0x456669a6, 0xa6999545,
	0x65151559, 0x55599a99, 0x6a665554, 0x55515556,
	0x456669a6, 0xa6999545, 0x65151559, 0x55599a99,
	0x6a695554, 0x55515556, 0x456669a6, 0xa6999545,
	0x95151565, 0x55599a99, 0x9a695554, 0x55515556,
	0x459669a6, 0xa6a59545, 0x95151565, 0x55599a99,
	0x9a695554, 0x55515556, 0x519669a6, 0xa6a65551,
	0x95451565, 0x5559a699, 0x9a695854, 0x55545556,
	0x519669a6, 0xa9a65551, 0x95454565, 0x1559a699,
	0x9a696455, 0x55545559, 0x51966a66, 0x69a65551,
	0x95454566, 0x1559a699, 0x9a696515, 0x55545559,
	0x55566a66, 0x69a65551, 0x95454566, 0x1559a699,
	0x9a996515, 0x55545559, 0x55566a66, 0x69a65551,
	0x95454566, 0x1559a699, 0x9a996515, 0x55545559,
	0x55566a66, 0x69a65551, 0x95454566, 0x1559a699,
	0x9a996515, 0x55545559, 0x55569a69, 0x69a65551,
	0x95454566, 0x1565a699, 0x9a999515, 0x55545559,
	0x55569a69, 0x69a65551, 0x95454596, 0x1565a6a5,
	0xa6999515, 0x55545559, 0x55569a69, 0x69a65551,
	0x55515196, 0x1565a6a6, 0xa6999545, 0x58545559,
	0x55569a69, 0x6a665554, 0x55515196, 0x156669a6,
	0xa6999545, 0x64551559, 0x55599a69, 0x6a665554,
	0x55515196, 0x456669a6, 0xa6999545, 0x65151559,
	0x55599a99, 0x6a665554, 0x55515556, 0x456669a6,
	0xa6999545, 0x65151559, 0x55599a99, 0x6a665554,
	0x55515556, 0x456669a6, 0xa6999545, 0x65151559,
	0x55599a99, 0x9a695554, 0x55515556, 0x456669a6,
	0xa6999545, 0x65151559, 0x55599a99, 0x9a695554,
	0x55515556, 0x456669a6, 0xa6a59545, 0x95151565,
	0x55599a99, 0x9a695554, 0x55515556, 0x459669a6,
	0xa6a65545, 0x95151565, 0x5559a699, 0x9a695554,
	0x55515556, 0x51966a66, 0x69a65551, 0x95451566,
	0x5559a699, 0x9a696454, 0x55545559, 0x51966a66,
	0x69a65551, 0x95451566, 0x5559a699, 0x9a996454,
	0x55545559, 0x51966a66, 0x69a65551, 0x95454566,
	0x1559a699, 0x9a996455, 0x55545559, 0x55566a66,
	0x69a65551, 0x95454566, 0x1559a699, 0x9a996515,
	0x55545559, 0x55566a66, 0x69a65551, 0x95454566,
	0x1559a699, 0x9a996515, 0x55545559, 0x55569a69,
	0x69a65551, 0x95454566, 0x1565a6a5, 0x9a996515,
	0x55545559, 0x55569a69, 0x69a65551, 0x95454596,
	0x1565a6a5, 0xa6999515, 0x55545559, 0x55569a69,
	0x69a65551, 0x55454596, 0x1565a9a6, 0xa6999515,
	0x55545559, 0x55569a69, 0x6a665551, 0x55455196,
	0x156669a6, 0xa6999545, 0x64545559, 0x55599a69,
	0x6a665554, 0x55515196, 0x156669a6, 0xa6999545,
	0x64545559, 0x55599a99, 0x6a665554, 0x55515196,
	0x456669a6, 0xa6999545, 0x64551559, 0x55599a99,
	0x6a665554, 0x55515556, 0x456669a6, 0xa6999545,
	0x65151559, 0x55599a99, 0x9a665554, 0x55515556,
	0x456669a6, 0xa6999545, 0x65151559, 0x55599a99,
};

/*
 * for each 24 jieqi, store a index,
 * offset days to solar base.
 */
static const unsigned jieqi_index[200] = {
	0x00000005, 0x00000172, 0x000002df, 0x0000044d,
	0x000005ba, 0x00000727, 0x00000894, 0x00000a02,
	0x00000b6f, 0x00000cdc, 0x00000e49, 0x00000fb7,
	0x00001124, 0x00001291, 0x000013fe, 0x0000156b,
	0x000016d9, 0x00001846, 0x000019b3, 0x00001b20,
	0x00001c8e, 0x00001dfb, 0x00001f68, 0x000020d5,
	0x00002243, 0x000023b0, 0x0000251d, 0x0000268a,
	0x000027f8, 0x00002965, 0x00002ad2, 0x00002c3f,
	0x00002dad, 0x00002f1a, 0x00003087, 0x000031f4,
	0x00003362, 0x000034cf, 0x0000363c, 0x000037a9,
	0x00003917, 0x00003a84, 0x00003bf1, 0x00003d5e,
	0x00003ecc, 0x00004039, 0x000041a6, 0x00004313,
	0x00004480, 0x000045ee, 0x0000475b, 0x000048c8,
	0x00004a35, 0x00004ba3, 0x00004d10, 0x00004e7d,
	0x00004fea, 0x00005158, 0x000052c5, 0x00005432,
	0x0000559f, 0x0000570d, 0x0000587a, 0x000059e7,
	0x00005b54, 0x00005cc2, 0x00005e2f, 0x00005f9c,
	0x00006109, 0x00006277, 0x000063e4, 0x00006551,
	0x000066be, 0x0000682c, 0x00006999, 0x00006b06,
	0x00006c73, 0x00006de1, 0x00006f4e, 0x000070bb,
	0x00007228, 0x00007396, 0x00007503, 0x00007670,
	0x000077dd, 0x0000794a, 0x00007ab8, 0x00007c25,
	0x00007d92, 0x00007eff, 0x0000806d, 0x000081da,
	0x00008347, 0x000084b4, 0x00008622, 0x0000878f,
	0x000088fc, 0x00008a69, 0x00008bd7, 0x00008d44,
	0x00008eb1, 0x0000901e, 0x0000918c, 0x000092f9,
	0x00009466, 0x000095d3, 0x00009741, 0x000098ae,
	0x00009a1b, 0x00009b88, 0x00009cf6, 0x00009e63,
	0x00009fd0, 0x0000a13d, 0x0000a2ab, 0x0000a418,
	0x0000a585, 0x0000a6f2, 0x0000a85f, 0x0000a9cd,
	0x0000ab3a, 0x0000aca7, 0x0000ae14, 0x0000af82,
	0x0000b0ef, 0x0000b25c, 0x0000b3c9, 0x0000b537,
	0x0000b6a4, 0x0000b811, 0x0000b97e, 0x0000baec,
	0x0000bc59, 0x0000bdc6, 0x0000bf33, 0x0000c0a1,
	0x0000c20e, 0x0000c37b, 0x0000c4e8, 0x0000c656,
	0x0000c7c3, 0x0000c930, 0x0000ca9d, 0x0000cc0b,
	0x0000cd78, 0x0000cee5, 0x0000d052, 0x0000d1c0,
	0x0000d32d, 0x0000d49a, 0x0000d607, 0x0000d774,
	0x0000d8e2, 0x0000da4f, 0x0000dbbc, 0x0000dd29,
	0x0000de97, 0x0000e004, 0x0000e171, 0x0000e2de,
	0x0000e44c, 0x0000e5b9, 0x0000e726, 0x0000e893,
	0x0000ea01, 0x0000eb6e, 0x0000ecdb, 0x0000ee48,
	0x0000efb6, 0x0000f123, 0x0000f290, 0x0000f3fd,
	0x0000f56b, 0x0000f6d8, 0x0000f845, 0x0000f9b2,
	0x0000fb20, 0x0000fc8d, 0x0000fdfa, 0x0000ff67,
	0x000100d5, 0x00010242, 0x000103af, 0x0001051c,
	0x00010689, 0x000107f7, 0x00010964, 0x00010ad1,
	0x00010c3e, 0x00010dac, 0x00010f19, 0x00011086,
	0x000111f3, 0x00011361, 0x000114ce, 0x0001163b,
	0x000117a8, 0x00011916, 0x00011a83, 0x00011bf0,
};

/* -------------- helper routine ---------------- */

static inline unsigned LEAP_MONTH(int year)
{
	int i = year - lunar_base.year;
	int j = i / 8;
	int k = i % 8;
	return (leap_month[j] >> (k * 4)) & 0xf;
}

static inline unsigned YEAR_OFF(int year)
{
	return lunar_years[year - lunar_base.year] & OFF_MASK;
}

static int popcount(unsigned x) {
#if 0
	return __builtin_popcount(x);
#else
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = (x + (x >> 8)) & 0x00FF00FF;
	x = (x + (x >> 16)) & 0x0000FFFF;
	return x;
#endif
}

static unsigned MONTH_OFF(int year, int month, int is_leap)
{
	unsigned mask = lunar_years[year - lunar_base.year] >> DAY_SHIFT;
	unsigned leap_day = mask & (1 << 12), leap = LEAP_MONTH(year);
	int n = is_leap ? month : (month - 1);
	int off;

	mask &= (1U << n) - 1;
	off = 29 * n + popcount(mask);

	if (leap && month > leap)
		off += leap_day ? 30 : 29;

	return off;
}

static unsigned MONTH_DAYS(int year, int month, int is_leap)
{
	int m = is_leap ? 12 : (month - 1);
	unsigned a = lunar_years[year - lunar_base.year];
	unsigned b = (1 << (m + DAY_SHIFT));
	return (a & b) ? 30 : 29;
}

static int check_lunar(int year, int month, int is_leap)
{
	if (year < lunar_base.year || year > lunar_last.year)
		return -ERANGE;
	if (month < 1 || month > 12)
		return -EINVAL;
	if (is_leap) {
		if (LEAP_MONTH(year) != month)
			return -EINVAL;
	}
	else {
		if (year == lunar_base.year) {
			if (month < lunar_base.month)
				return -ERANGE;
		}
		else if (year == lunar_last.year) {
			if (month > lunar_last.month)
				return -ERANGE;
		}
	}
	return 0;
}

static int lunar_2_offset(struct cl_date d)
{
	int ret;

	ret = check_lunar(d.year, d.month, d.leap);
	if (ret)
		return ret;

	if (d.day < 1 || d.day > MONTH_DAYS(d.year, d.month, d.leap))
		return -EINVAL;

	if (d.year == lunar_base.year) {
		if (d.month == lunar_base.month) {
			if (d.day < lunar_base.day)
				return -ERANGE;
			ret = d.day - lunar_base.day;
		}
		else {
			int i;
			ret = MONTH_DAYS(lunar_base.year, lunar_base.month, 0) - lunar_base.day + 1;
			for (i = lunar_base.month + 1; i < d.month; i++)
				ret += MONTH_DAYS(lunar_base.year, i, 0);
			ret += d.day - 1;
		}
	}
	else {
		ret = YEAR_OFF(d.year);
		ret += MONTH_OFF(d.year, d.month, d.leap);
		ret += d.day - 1;
	}

	return ret;
}

static int offset_2_lunar(int days, struct cl_date *result)
{
	int y, m, d, leap = 0;
	int l = YEAR_OFF(lunar_base.year + 1);
	int r;

	if (days < 0)
		return -ERANGE;
	else if (days < l) {
		y = lunar_base.year;

		r = MONTH_DAYS(lunar_base.year, lunar_base.month, 0) - lunar_base.day + 1;
		if (days < r) {
			m = lunar_base.month;
			d = lunar_base.day + days - 1;
		}
		else {
			d = days - r;
			for (m = lunar_base.month + 1; m <= 12; m++) {
				r = MONTH_DAYS(y, m, 0);
				if (d <= r)
					break;
				d -= r;
			}
		}
	}
	else {
		y = lunar_base.year + 1 + (days - l) / 385;
		if (y > lunar_last.year)
			return -ERANGE;
		while (y < lunar_last.year && days >= YEAR_OFF(y+1))
			y++;
		days -= YEAR_OFF(y);
		r = LEAP_MONTH(y);

		// guess a month
		m = days / 30 + 1;
		if (r && m > r) {
			m--;
			if (m == r)
				leap = 1;
		}
		// check next month
		if (r && m == r && leap == 0) {
			if (days >= MONTH_OFF(y, m, 1))
				leap = 1;
		}
		else {
			if (m < 12 && days >= MONTH_OFF(y, m+1, 0)) {
				leap = 0;
				m++;
			}
		}
		if (m > 12)
			return -ERANGE;

		d = days - MONTH_OFF(y, m, leap);

		if (y == lunar_last.year) {
			if (m > lunar_last.month)
				return -ERANGE;
			if (m == lunar_last.month && d >= lunar_last.day)
				return -ERANGE;
		}
	}

	result->year = y;
	result->month = m;
	result->day = d + 1;
	result->leap = leap;
	return 0;
}

static int inline is_leap_year(int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

static int s_year_off(int year)
{
	int n = year - 1901;
	return n * 365 + n / 4;
}

static const unsigned short smo[12] =      {0,31,59,90,120,151,181,212,243,273,304,334};
static const unsigned short smo_leap[12] = {0,31,60,91,121,152,182,213,244,274,305,335};

static inline int s_month_off(int leap, int month)
{
	return leap ? smo_leap[month-1]: smo[month-1];
}

static const unsigned char smd[12] =      {31,28,31,30,31,30,31,31,30,31,30,31};
static const unsigned char smd_leap[12] = {31,29,31,30,31,30,31,31,30,31,30,31};

static inline int s_month_days(int leap, int month)
{
	return leap ? smd_leap[month-1]: smd[month-1];
}

static int solar_2_offset(struct cl_date d)
{
	int off, leap = is_leap_year(d.year);

	if (d.year < solar_base.year || d.year > solar_last.year)
		return -ERANGE;
	if (d.month < 1 || d.month > 12)
		return -EINVAL;
	if (d.day < 1 || d.day > s_month_days(leap, d.month))
		return -EINVAL;

	off = s_year_off(d.year);
	off += s_month_off(leap, d.month);
	off += d.day - 1;
	return off;
}

static int offset_2_solar(int days, struct cl_date *result)
{
	int y, m;
	int is_leap;

	y = days / 366 + solar_base.year;
	if (y < solar_base.year || y > solar_last.year)
		return -ERANGE;
	if (y < solar_last.year && days >= s_year_off(y+1))
		y++;

	is_leap = is_leap_year(y);
	days -= s_year_off(y);

	m = days / 31 + 1;
	if (m < 1 || m > 12)
		return -ERANGE;
	if (m < 12 && days >= s_month_off(is_leap, m+1))
		m++;

	days -= s_month_off(is_leap, m);
	if (days < 0 || days >= s_month_days(is_leap, m))
		return -ERANGE;

	result->year = y;
	result->month = m;
	result->day = days + 1;
	return 0;
}

/* -------------------- APIs ------------------ */

void cl_supported_range(int lunar, struct cl_date *from, struct cl_date *to)
{
	if (from)
		*from = lunar ? lunar_base : solar_base;
	if (to)
		*to = lunar ? lunar_last : solar_last;
}

int cl_leap_month(int year)
{
	if (year < lunar_base.year || year > lunar_last.year)
		return -ERANGE;

	return LEAP_MONTH(year);
}

int cl_days_of_lunar_year(int year)
{
	if (year <= lunar_base.year || year >= lunar_last.year)
		return -ERANGE;

	return YEAR_OFF(year+1) - YEAR_OFF(year);
}

int cl_days_of_lunar_month(int year, int month, int is_leap)
{
	int ret;

	ret = check_lunar(year, month, is_leap);
	if (ret)
		return ret;

	return MONTH_DAYS(year, month, is_leap);
}

int cl_lunar_timespan(struct cl_date from, struct cl_date to, int *days)
{
	int off0, off1;

	off0 = lunar_2_offset(from);
	if (off0 < 0)
		return off0;

	off1 = lunar_2_offset(to);
	if (off1 < 0)
		return off1;

	*days = off1 - off0;
	return 0;
}

int cl_lunar_by_span(struct cl_date date, int days, struct cl_date *result)
{
	int off;

	off = lunar_2_offset(date);
	if (off < 0)
		return off;

	return offset_2_lunar(off + days, result);
}

int cl_solar2lunar(struct cl_date solar, struct cl_date *lunar)
{
	int off;

	off = solar_2_offset(solar);
	if (off < 0)
		return off;

	return offset_2_lunar(off, lunar);
}

int cl_lunar2solar(struct cl_date lunar, struct cl_date *solar)
{
	int off;

	off = lunar_2_offset(lunar);
	if (off < 0)
		return off;

	return offset_2_solar(off, solar);
}

int cl_get_jieqi(struct cl_date from, int days, int jieqi, struct cl_date *dates, int N)
{
	int off;
	int l = 0, r = 199;
	int cursor;
	int n = 0;
	int j = jieqi_base.jieqi;

	if (days < 0)
		return -EINVAL;

	off = lunar_2_offset(from);
	if (off < 0)
		return off;

	// binary search a nearest index
	while (l < r) {
		int m = (l + r) / 2;
		cursor = jieqi_index[m];	// offset of this index
		if (off < cursor)
			r = m;
		else {
			if (l == m)
				break;
			l = m;
		}
	}

	l *= 24;	// idx of jieqi_span[]
	r = off + days;

	while (cursor < r) {
		if (cursor >= off && (jieqi == 0 || j == jieqi)) {
			if (n < N) {
				offset_2_lunar(cursor, &dates[n]);
				dates[n].jieqi = j;
			}
			n++;
		}

		// update jieqi_span[] index
		l++;
		if (l >= 16 * sizeof(jieqi_span)/sizeof(jieqi_span[0]))
			break;

		// update current jieqi
		j++;
		if (j == 25)
			j = 1;

		// update offset
		cursor += 14 + ((jieqi_span[l/16] >> (2*(l%16))) & 0x3);
	}

	return n;
}

