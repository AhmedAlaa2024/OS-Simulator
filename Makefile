build:
	gcc clk.c -o clk.out
	gcc utils.c priority_queue.c process_generator.c -o process_generator.out
	gcc utils.c priority_queue.c scheduler.c -o scheduler.out
	gcc utils.c process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./test_generator.out
	./process_generator.out
