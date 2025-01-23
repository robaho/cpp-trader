CXX = clang++
INCLUDES = -I ../cpp_orderbook -I ../cpp_fix_codec -I ../cpp_fix_engine -I ../cpp_fixed
CXXFLAGS = -std=c++20 -O0 -Wall -fsanitize=address -fno-omit-frame-pointer -pedantic-errors -g ${INCLUDES}

# CXXFLAGS = -std=c++20 -Wall -pedantic-errors -g ${INCLUDES}
# CXXFLAGS = -std=c++20 -O3 -Wall -pedantic-errors -g ${INCLUDES}
# CXXFLAGS = -std=c++20 -O3 -fprofile-generate -Wall -pedantic-errors -g ${INCLUDES}
# CXXFLAGS = -std=c++20 -O3 -fprofile-use=default.profdata -Wall -pedantic-errors -g ${INCLUDES}

TEST_SRCS = ${wildcard *_test.cpp}
TEST_OBJS = $(addprefix bin/, $(TEST_SRCS:.cpp=.o))
TEST_MAINS = $(addprefix bin/, $(TEST_SRCS:.cpp=))

HEADERS = ${wildcard *.h}

SRCS = cpp-trader.cpp
OBJS = $(addprefix bin/, $(SRCS:.cpp=.o))

MAIN = bin/cpp-trader
MAIN_OBJ = ${basename ${MAIN}}.o

FIX_CODEC = ../cpp_fix_codec/bin/fix_codec.a
FIX_ENGINE = ../cpp_fix_engine/bin/fix_engine.a
ORDERBOOK = ../cpp_orderbook/bin/orderbook.a

LIBS = ${FIX_CODEC} ${FIX_ENGINE} ${ORDERBOOK}

.PRECIOUS: bin/%.o

all: ${MAIN} $(TEST_MAINS) ${LIBS}
	@echo compile finished

test: ${TEST_MAINS}

run_tests: ${TEST_MAINS}
	for main in $^ ; do \
		$$main; \
	done

bin/%.o: %.cpp ${HEADERS}
	@ mkdir -p bin
	${CXX} ${CXXFLAGS} -c $(notdir $(basename $@).cpp) -o $@

${MAIN}: ${LIBS} ${OBJS}
	${CXX} ${CXXFLAGS} ${LIBS} ${OBJS} -o ${MAIN}

bin/%_test: bin/%_test.o
	${CXX} ${CXXFLAGS} $@.o -o $@ 

clean:
	rm -rf bin *~.
