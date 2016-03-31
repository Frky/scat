TESTS = \
	int_return \
	float_return \
	call_stack_overflow

all: $(TESTS:=.bin)

%.s: %.c
	gcc -S $<

%.o: %.c
	gcc -c $<

%.bin: %.o
	gcc -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TESTS:=.s) $(TESTS:=.o) $(TESTS:=.bin)
