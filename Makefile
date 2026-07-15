CXX      ?= g++
AR       ?= ar
CPPFLAGS ?= -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS  ?=
PREFIX   ?= /usr/local
DESTDIR  ?=

LIB      ?= libarchspec.a
CLI      ?= archspec-inspect
SUMMARY  ?= archspec-summary
TESTS    ?= archspec-tests

SRCS := $(wildcard src/*.cc)
OBJS := $(SRCS:.cc=.o)

.PHONY: all clean install summary test

all: $(LIB) $(CLI)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(CLI): tools/archspec-inspect.o $(LIB)
	$(CXX) tools/archspec-inspect.o $(LIB) -o $@ $(LDFLAGS)

$(SUMMARY): examples/summary.o $(LIB)
	$(CXX) examples/summary.o $(LIB) -o $@ $(LDFLAGS)

summary: $(SUMMARY)

$(TESTS): tests/core_tests.o $(LIB)
	$(CXX) tests/core_tests.o $(LIB) -o $@ $(LDFLAGS)

test: $(TESTS)
	./$(TESTS)

src/%.o: src/%.cc include/aview.hh src/internal.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

examples/%.o: examples/%.cc include/aview.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

tools/%.o: tools/%.cc include/aview.hh include/archspec/archspec.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

tests/%.o: tests/%.cc include/aview.hh include/archspec/archspec.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/include/archspec $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/share/archspec
	install -m755 $(CLI) $(DESTDIR)$(PREFIX)/bin/$(CLI)
	install -m644 $(LIB) $(DESTDIR)$(PREFIX)/lib/$(LIB)
	install -m644 include/aview.hh $(DESTDIR)$(PREFIX)/include/aview.hh
	install -m644 include/archspec/archspec.hpp $(DESTDIR)$(PREFIX)/include/archspec/archspec.hpp
	install -m644 schema/archspec-report-v1.schema.json $(DESTDIR)$(PREFIX)/share/archspec/

clean:
	rm -f $(OBJS) examples/*.o tools/*.o tests/*.o $(LIB) $(CLI) $(SUMMARY) $(TESTS)
