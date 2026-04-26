/** ------------- IPK 2 - RDT ---------------
#	file:	Makefile
# 	Author: Kristian Luptak <xluptak00>
#	date: 	26.4.2026


#compiling
CC=gcc
CFLAGS= -std=gnu17 -pedantic -Wall -Wextra
FILE_LOC= src/
LIBS=-pthread -lpcap

OBJ= 	ipk-rdt.o			\
		CLIparse.o			\
		
OUT= ipk-rdt

#utils
DEVSHELL=c

ZIP_NAME=xluptak00

TEST_FILE=test.sh


.PHONY: test all clean NixDevShellName zip

# compiling
all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) $(LIBS)

%.o: $(FILE_LOC)%.c
	$(CC) $(CFLAGS) -o $@ -c $<


# write devShell into stdout
NixDevShellName:
	@echo $(DEVSHELL)

# run automatic tests
test: all
	chmod +x $(TEST_FILE)
	./$(TEST_FILE)

# zip
zip:
	zip -r $(ZIP_NAME).zip Makefile LICENSE CHANGELOG.md README.md src -x *.o

# clean
clean:
	rm -rf *.o $(OUT)

