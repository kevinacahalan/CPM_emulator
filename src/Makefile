CC:=gcc
CXX:=g++

ifeq ($(OS),Windows_NT)
NATIVEPATH = $(subst /,\,$1)
DIRNAME    = $(filter-out %:,$(patsubst %\,%,$(dir $1)))
RM         = del /f 2>nul
RMDIR      = call && (if exist $1 rmdir /s /q $1)
MKDIR      = call && (if not exist $1 mkdir $1)
PREFIX    ?= C:
CP         = copy /y
CPDIR      = xcopy /e /i /q /r /y /b
QUOTE_ARG  = "$(subst ",',$1)"#'
else
NATIVEPATH = $(subst \,/,$1)
DIRNAME    = $(patsubst %/,%,$(dir $1))
RM         = rm -f
RMDIR      = rm -rf $1
MKDIR      = mkdir -p $1
PREFIX    ?= $(HOME)
CP         = cp
CPDIR      = cp -r
QUOTE_ARG  = '$(subst ','\'',$1)'#'
endif


#NAME := $(notdir $(CURDIR))
NAME := CPM_emu

.SUFFIXES:
.SUFFIXES: .o .s .S .asm .l .y .c .cpp .cxx .cc .c++

C_SRC    := $(wildcard *.c)
CPP_SRC  := $(wildcard *.cpp *.cxx *.cc *.c++)
ASM_SRC  := $(wildcard *.asm)
S_SRC    := $(wildcard *.S)
LEX_SRC  := $(wildcard *.l)
YACC_SRC := $(wildcard *.y)

C_OBJ    := $(patsubst %.c,%.o,$(C_SRC))
CPP_OBJ  := $(addsuffix .o,$(basename $(CPP_SRC)))
ASM_OBJ  := $(patsubst %.c,%.o,$(ASM_SRC))
S_OBJ    := $(patsubst %.c,%.o,$(S_SRC))
LEX_OBJ  := $(patsubst %.c,%.o,$(LEX_SRC))
YACC_OBJ := $(patsubst %.c,%.o,$(YACC_SRC))

H_SRC    := $(wildcard *.h *.hh *.hpp *.h++)
LINKER   := $(if $(CPP_SRC),$(CXX),$(CC))

CFLAGS   := -Og -g3 -W -Wall -Wshadow -Wstrict-prototypes -Wmissing-prototypes
CXXFLAGS := -Og -g3 -W -Wall -Wshadow

.PHONY : clean all

all: $(NAME)
	@echo The name is \"$(NAME)\".

$(NAME): $(C_OBJ) $(CPP_OBJ) $(ASM_OBJ) $(S_OBJ) $(LEX_OBJ) $(YACC_OBJ)
	$(LINKER) $(CFLAGS) -o $@ $^

clean :
	-$(RM) *.o *.obj *.exe DEADJOE $(NAME) *~







ifeq ($(OS),Windows_NT)


else
# Linux & mac


endif