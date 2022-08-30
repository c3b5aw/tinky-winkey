# Ressources:
# https://www.bojankomazec.com/2011/10/hom-to-use-nmake-and-makefile.html

CC		= cl
CFLAGS	= /Wall /WX /EHs /std:c++20

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

BIN_SVC		= svc.exe
SRC_SVC		= $(SRC_DIR)\svc.cpp
OBJ_SVC		= $(SRC_SVC:.cpp=.obj)
OBJ_SVC		= $(OBJ_SVC:src=obj)

BIN_WINKEY	= winkey.exe
SRC_WINKEY	= $(SRC_DIR)\winkey.cpp
OBJ_WINKEY	= $(SRC_WINKEY:.cpp=.obj)
OBJ_WINKEY	= $(OBJ_WINKEY:src=obj)

{$(SRC_DIR)}.cpp{$(OBJ_DIR)}.obj:
	@echo Compiling...
	$(CC) /c /nologo $(CFLAGS) /Fo$(OBJ_DIR)\ /I$(SRC_DIR) $<

$(BIN_SVC): $(OBJ_SVC)
	@echo Linking $(BIN_SVC)
	link /NOLOGO /out:$(BIN_DIR)\$(BIN_SVC) $(OBJ_SVC)

$(BIN_WINKEY): $(OBJ_WINKEY)
	@echo Linking $(BIN_WINKEY)
	link /NOLOGO /out:$(BIN_DIR)\$(BIN_WINKEY) $(OBJ_WINKEY)

create_dirs:
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

move_bins:
	@echo Moving bins..

clean:
	@echo Cleaning objects...
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)

fclean: clean
	@echo Cleaning binaries...
	@if exist $(BIN_SVC) del $(BIN_SVC)
	@if exist $(BIN_WINKEY) del $(BIN_WINKEY)

all: create_dirs $(BIN_SVC) $(BIN_WINKEY) move_bins
re: fclean all