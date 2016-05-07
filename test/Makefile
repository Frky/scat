TESTS = \
	int_return \
	float_return \
	call_stack_overflow \
	stack_params

all: $(TESTS:=.bin)

%.s: %.c
	gcc -S -masm=intel $<

%.o: %.c
	gcc -c $<

%.bin: %.o
	gcc -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TESTS:=.s) $(TESTS:=.o) $(TESTS:=.bin)
