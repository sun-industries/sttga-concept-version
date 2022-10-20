#emcc -lembind -s MODULARIZE=1 -s ENVIRONMENT='web' ../saga/src/lib/c++/adapter.cpp -o ../saga/src/lib/c++/SGP4.mjs -sEXPORTED_RUNTIME_METHODS=cwrap -sEXPORTED_FUNCTIONS=_twoline2satrec,_increment

emcc -lembind -s MODULARIZE=1 -s ENVIRONMENT='web' ../saga/src/lib/c++/adapter.cpp -o ../saga/src/lib/c++/SGP4.mjs -O3