all: newAMZ

newAMZ:
	@echo " ---> make newAMZ"
	g++ --std=c++17 -Ofast -DNDEBUG -Ifeaturescpp -Iheuristic -fopenmp -I../src/libs -Ilibs -Ilibs/optframe/src -fconcepts heuristic/newmainAMZ.cpp -o ./build/app_amz
	@echo " ---> run app_amz"
	mkdir -p build/
	(cd .. && ./src/build/app_amz)