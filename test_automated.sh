#!/bin/bash

echo "Testing Specific SR Behaviors"
echo "==========================="

# Compile
gcc -Wall -ansi -pedantic -o sr emulator.c sr.c

# Test 1: Verify no double timer starts
echo -e "\n1. Timer Management Test"
echo "Testing for double timer starts..."
echo -e "5\n0.0\n0.0\n5\n2" | ./sr | grep -i "warning" > timer_test.txt
if [ -s timer_test.txt ]; then
    echo "❌ Timer warnings found:"
    cat timer_test.txt
else
    echo "✓ No timer warnings - correct!"
fi

# Test 2: Verify selective retransmission
echo -e "\n2. Selective Retransmission Test"
echo "Testing that only lost packets are retransmitted..."
echo -e "5\n0.3\n0.0\n10\n2" | ./sr > selective_test.txt
lost_count=$(grep -c "packet being lost" selective_test.txt)
resent_count=$(grep -c "resending packet" selective_test.txt)
echo "Packets lost: $lost_count"
echo "Packets resent: $resent_count"
if [ $resent_count -le $lost_count ]; then
    echo "✓ Selective retransmission working"
else
    echo "❌ Too many retransmissions"
fi

# Test 3: Verify packets_received increment
echo -e "\n3. Packet Count Test (Critical for autograder)"
echo "Verifying packet counting matches expected behavior..."
echo -e "3\n0.0\n0.0\n10\n2" | ./sr > count_test.txt
total_packets=$(grep -c "correctly received, send ACK" count_test.txt)
final_count=$(grep "number of correct packets received at B:" count_test.txt | awk '{print $NF}')
echo "Messages processed: $total_packets"
echo "Final count: $final_count"
if [ "$total_packets" -eq "$final_count" ]; then
    echo "✓ Packet counting correct"
else
    echo "❌ Packet counting mismatch"
fi

# Test 4: Run exact autograder test 3
echo -e "\n4. Exact Autograder Test 3"
echo "Running exact autograder test 3 configuration..."
echo -e "3\n0.9\n0.0\n15\n0" | ./sr > autograder_test3.txt
echo "Output saved to autograder_test3.txt"
echo "Check for these critical points:"
echo "- No 'outside window' messages"
echo "- Correct packet count (should be high, ~29)"
echo "- All packets marked as 'correctly received'"

echo -e "\nTesting complete. Review output files for details."