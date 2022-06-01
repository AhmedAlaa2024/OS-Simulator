build:
	gcc clk.c -o clk.out
	gcc utils.c priority_queue.c process_generator.c -o process_generator.out
	gcc utils.c priority_queue.c LinkedList.c scheduler.c -o scheduler.out -lm
	gcc utils.c process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
