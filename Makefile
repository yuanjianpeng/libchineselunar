all: libchineselunar.a calendar

%.o: %.c chineselunar.h
	$(CC) -c -o $@ $< -Wall

libchineselunar.a: chineselunar.o
	$(AR) rcs $@ $^

calendar: calendar.o libchineselunar.a
	$(CC) -o $@ $< -L. -lchineselunar

clean:
	rm -f chineselunar.o libchineselunar.a calendar.o calendar

