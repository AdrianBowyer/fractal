# New Fractal Makefile

GRAPHICS=/home/ensab/Graphics
NEW_FRAC = .

SRC = $(NEW_FRAC)/src
OBJ = $(NEW_FRAC)/obj
BIN = $(NEW_FRAC)/bin
INC = $(NEW_FRAC)/include

II = -I$(INC)
CC = g++

FL = -g -I$(INC) -I$(GRAPHICS)/include -O2 -I/usr/include/freetype2 \
	-D_REENTRANT -D_FILE_OFFSET_BITS=64

GB = $(GRAPHICS)/obj/graphics.o -L/usr/lib -L/usr/local/lib -L/usr/X11R6/lib -lMagick++ \
	-lMagick -ltiff -ljpeg -lpng -ldpstk -ldps -lXext -lXt -lSM -lICE -lX11 \
	-lbz2 -lz -lpthread -lm

f_test:	$(OBJ)/new_frac.o $(OBJ)/f_test.o
	$(CC) $(FL) $(GB) -o $(BIN)/f_test $(OBJ)/f_test.o $(OBJ)/new_frac.o

$(OBJ)/f_test.o:	$(INC)/new_frac.h $(SRC)/f_test.cpp
	$(CC) -c $(FL) $(II) -o $(OBJ)/f_test.o $(SRC)/f_test.cpp

$(OBJ)/new_frac.o:	$(INC)/new_frac.h $(SRC)/new_frac.cpp
	$(CC) -c $(FL) $(II) -o $(OBJ)/new_frac.o $(SRC)/new_frac.cpp


