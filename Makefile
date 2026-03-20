CXX = g++
CXXFLAGS = -I./tests/mock_libs/ -I./tests/mock_libs/IRremote -I./tests/mock_libs/SD -I./tests/mock_libs/EEPROM -I./tests/mock_libs/WiFi -I./tests/mock_libs/SPI -Wall

TESTS = tests/test_unit tests/test_basic tests/test_sd tests/test_evolution

all: $(TESTS)

tests/test_unit: tests/test_unit.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

tests/test_basic: tests/test_basic.cpp tests/mock_arduino.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/test_sd: tests/test_sd.cpp tests/mock_arduino.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/test_evolution: tests/test_evolution.cpp tests/mock_arduino.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

run_tests: all
	./tests/test_unit
	./tests/test_basic
	./tests/test_sd
	./tests/test_evolution

clean:
	rm -f $(TESTS)
