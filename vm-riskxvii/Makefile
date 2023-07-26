TARGET = vm_riskxvii

CC = gcc

CFLAGS     = -c -Wall -Wvla -Werror -O1 -ffunction-sections -fdata-sections -std=c11
LDFLAGS    = -s
SRC        = vm_riskxvii.c
OBJ        = $(SRC:.c=.o)

all:$(TARGET)

$(TARGET):$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

.SUFFIXES: .c .o

.c.o:
	 $(CC) $(CFLAGS) $<

run:
	./$(TARGET)

tests: $(TARGET)
	@echo "Ready to run tests, please use make run_tests"

run_tests: $(TARGET)
	@echo "#### Start tests ${TARGET}! ####"
	@echo ""
	@for testfile in tests/*.mi; do \
		OUT=$${testfile%.mi}.out; \
		IMAGE=$$testfile; \
		./$(TARGET) $$IMAGE | diff - $$OUT && echo "Testing $$testfile: SUCCESS!" || echo "Testing $$testfile: FAILURE."; \
	done

	@echo ""
	@echo "#### Testing completed! ####"
	@echo ""

clean:
	rm -f *.o *.obj $(TARGET) *.gcov *.gcno *.gcda
