all: bin bin/DLMlib.o bin/DLM-client bin/DLM

bin:
	mkdir bin
bin/DLMlib.o: DLM-lib/DLMlib.h DLM-lib/DLMlib.c
	gcc -c DLM-lib/DLMlib.c -o bin/DLMlib.o
bin/DLM-client: DLM-lib/DLMlib.h bin/DLMlib.o DLM-client/main.c
	gcc -IDLM-lib/ DLM-client/main.c bin/DLMlib.o -o bin/DLM-client
bin/DLM: DLM-lib/DLMlib.h DLM/DLM.hpp DLM/main.cpp
	g++ -IDLM-lib/ DLM/main.cpp -o bin/DLM
clean:
	rm bin/DLMlib.o bin/DLM-client bin/DLM
