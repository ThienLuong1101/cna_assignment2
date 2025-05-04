#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting Selective Repeat Protocol Tests${NC}\n"

# Function to run a test and capture output
run_test() {
    test_name=$1
    test_input=$2
    
    echo -e "${YELLOW}Running $test_name...${NC}"
    echo "$test_input" | ./sr > test_output.txt 2>&1
    
    # Check for errors
    if grep -q "panic\|error\|fault" test_output.txt; then
        echo -e "${RED}❌ $test_name failed with errors${NC}"
        grep -E "panic|error|fault" test_output.txt
    else
        echo -e "${GREEN}✓ $test_name completed${NC}"
        
        # Show summary statistics
        echo "Statistics:"
        grep -E "number of valid|number of packet resends|number of correct packets|number of messages delivered" test_output.txt
    fi
    
    # Save full output for later review
    mv test_output.txt "$test_name.txt"
    echo ""
}

# First, compile the program
echo -e "${YELLOW}Compiling sr.c...${NC}"
gcc -Wall -ansi -pedantic -o sr emulator.c sr.c
if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed! Please fix errors before testing.${NC}"
    exit 1
fi
echo -e "${GREEN}Compilation successful!${NC}\n"

# Test 1: No loss, no corruption (baseline)
run_test "Test1_No_Errors" "10
0.0
0.0
10
2"

# Test 2: Packet loss only
run_test "Test2_Packet_Loss" "10
0.2
0.0
10
2"

# Test 3: Packet corruption only
run_test "Test3_Packet_Corruption" "10
0.0
0.2
10
2"

# Test 4: Both loss and corruption
run_test "Test4_Mixed_Errors" "15
0.1
0.1
8
2"

# Test 5: High loss stress test
run_test "Test5_High_Loss" "10
0.4
0.0
5
2"

# Test 6: Window stress test
run_test "Test6_Window_Stress" "30
0.1
0.1
2
1"

# Test 7: Out-of-order test
run_test "Test7_Out_of_Order" "8
0.3
0.0
10
2"

# Test 8: Window boundary test
run_test "Test8_Window_Boundary" "6
0.1
0.1
5
2"

# Test 9: Sequence wraparound test
run_test "Test9_Seq_Wraparound" "20
0.0
0.0
1
1"

echo -e "${GREEN}All tests completed!${NC}"
echo -e "\nTest outputs saved as: Test*.txt"
echo -e "\nReview the full outputs for detailed protocol behavior."