#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "chineselunar.h"

static char *bignum[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九", "十" };

static char *lunar_month_name(int month, int leap)
{
	static char result[128];

	result[0] = '\0';
	if (leap)
		strcat(result, "闰");
	if (month == 1)
		strcat(result, "正");
	else if (month == 11)
		strcat(result, "冬");
	else if (month == 12)
		strcat(result, "腊");
	else
		strcat(result, bignum[month-1]);

	return result;
}

static char *lunar_day_name(int day)
{
	static char result[128];

	result[0] = '\0';

	if (day <= 10) {
		strcat(result, "初");
		strcat(result, bignum[day-1]);
	}
	else if (day < 20) {
		strcat(result, "十");
		strcat(result, bignum[day-11]);
	}
	else if (day < 30) {
		strcat(result, "廿");
		if (day == 20)
			strcat(result, "十");
		else
			strcat(result, bignum[day-21]);
	}
	else {
		strcat(result, "三十");
	}

	return result;
}

static char *wday_name(struct cl_date date)
{
	/*
	 * 提供一个基准日期，知道它是星期几
	 * 使用cl_lunar_timespan()接口，
	 * 可以计算出和基准日期间隔的天数，
	 * 从而算出对应的星期
	 */
	struct cl_date base = { 2026, 4, 1 };
	int wday = 6;
	int days = 0;
	int ret;

	ret = cl_lunar_timespan(base, date, &days);
	if (ret < 0)
		return "";

	wday += days;
	wday %= 7;
	if (wday < 0)
		wday += 7;

	if (wday == 6)
		return "日";
	else
		return bignum[wday];
}

int main(int argc, char **argv)
{
	struct cl_date date = { .month = 1, .day = 1, };
	struct cl_date lunar_base;
	int today = 0;
	int ret;
	int i;

	if (argc < 2) {
		struct tm *tm;
		time_t now = time(NULL);

		tm = localtime(&now);
		if (!tm) {
			perror("localtime() failed");
			return 1;
		}
		date.year = tm->tm_year + 1900;
		date.month = tm->tm_mon + 1;
		today = tm->tm_mday;
	}
	else {
		date.year = atoi(argv[1]);
		if (argc > 2)
			date.month = atoi(argv[2]);
	}

	ret = cl_solar2lunar(date, &lunar_base);
	if (ret < 0) {
		if (ret == -ERANGE)
			fprintf(stderr, "错误：不支持这个日期\n");
		else if (ret == -EINVAL)
			fprintf(stderr, "错误：不存在的日期\n");
		return 1;
	}

	printf("公历%d年%d月:\n", date.year, date.month);

	for (i = 0; i < 31; i++) {
		struct cl_date solar, lunar, jieqi;
		const char *jq = "";
		char c = ' ';
		int n;

		ret = cl_lunar_by_span(lunar_base, i, &lunar);
		if (ret < 0)
			// break if out of range
			break;

		cl_lunar2solar(lunar, &solar);
		if (solar.month != date.month)
			// break at next month
			break;

		if (solar.day == today)
			// highlight today
			c = '*';

		n = cl_get_jieqi(lunar, 1, 0, &jieqi, 1);
		if (n == 1)
			jq = jieqi_names[jieqi.jieqi-1];

		printf("%c %2d日    %s月%s    星期%s %s\n",
			c, solar.day,
			lunar_month_name(lunar.month, lunar.leap),
			lunar_day_name(lunar.day),
			wday_name(lunar), jq);
	}

	return 0;
}

