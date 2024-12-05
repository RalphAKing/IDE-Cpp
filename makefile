all: compile link clean

compile:
	g++ -c main.cpp

link:
	g++ main.o -o main -lcomctl32 -lcomdlg32 -lgdi32 -lshlwapi

clean:
	del main.o


