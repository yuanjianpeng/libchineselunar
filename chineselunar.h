/*
 * A dependency-free, pure C library.
 * Convert beween solar date and chinese lunar date.
 * and also supports the 24 solar terms (jieqi).
 */

#ifndef CHINESELUNAR_H
#define CHINESELUNAR_H

struct cl_date {
	short year;	// 1900-2100
	short month;	// 1-12
	short day;	// 1-31
	char leap;	// 0, 1
	char jieqi;	// 0, 1-24
};

extern const char *const jieqi_names[];

void cl_supported_range(int lunar, struct cl_date *from, struct cl_date *to);

/*
 * these APIs return a negative errno in the case of error:
 *     -EINVAL, the date is invalid.
 *     -ERANGE, the date is not in database.
 */

extern int cl_leap_month_of_lunar_year(int year);
extern int cl_days_of_lunar_year(int year);
extern int cl_days_of_lunar_month(int year, int month, int is_leap);

extern int cl_lunar_timespan(struct cl_date from, struct cl_date to, int *days);
extern int cl_lunar_by_span(struct cl_date date, int days, struct cl_date *result);

extern int cl_solar2lunar(struct cl_date solar, struct cl_date *lunar);
extern int cl_lunar2solar(struct cl_date lunar, struct cl_date *solar);

extern int cl_get_jieqi(struct cl_date from, int days, int jieqi, struct cl_date *dates, int N);

#endif

