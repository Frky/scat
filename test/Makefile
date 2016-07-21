TESTS = \
	int_return \
	float_return \
	call_stack_overflow \
	stack_params \
	variadic

all: $(TESTS:=.bin)

%.s: %.c
	gcc -S -masm=intel $<

%.o: %.c
	gcc -g -c $<

%.bin: %.o
	gcc -g -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TESTS:=.s) $(TESTS:=.o) $(TESTS:=.bin)
