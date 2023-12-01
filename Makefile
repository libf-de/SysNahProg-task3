# Compiler und Flags definieren
CC = g++
CFLAGS = -Wall -g
CXXFLAGS = -std=c++20 -Wall -g
LEX = flex
YACC = bison
YFLAGS = -d

# Zieldatei
TARGET = myshell

# Objektdateien
OBJS = lex.yy.o parse.tab.o main.o

all: $(TARGET) Makefile

run: $(TARGET)
	./$(TARGET)

# Regel zum Erstellen des Hauptprogramms
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Regel für parse.tab.c und parse.tab.h
parse.tab.c parse.tab.h: parse.y
	$(YACC) $(YFLAGS) parse.y

# Regel für lex.yy.c
lex.yy.c: scan.l parse.tab.h
	$(LEX) --header-file=lex.yy.h scan.l

# Regel für lex.yy.o
lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c

# Regel für parse.tab.o
parse.tab.o: parse.tab.c
	$(CC) $(CFLAGS) -c parse.tab.c

# Regel für main.o
main.o: main.cpp parse.tab.h
	$(CC) $(CFLAGS) -c main.cpp

delay: delay.c
	$(CC) $(CFLAGS) delay.c -o delay

# Regel für clean
clean:
	rm -f $(TARGET) $(OBJS) lex.yy.c parse.tab.c parse.tab.h
