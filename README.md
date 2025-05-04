rule 1: make sure you understand the question clearly

the main task is modify gbn to sr -> duplicate them first

systems components:

Application Layer (Layer 5) on Host A
        ↓
Transport Layer (Layer 4) - code this
        ↓
Network Layer (Layer 3) - Emulator
        ↓
[Network - can corrupt/lose packets]
        ↓
Network Layer (Layer 3) - Emulator
        ↓
Transport Layer (Layer 4) - code this
        ↓
Application Layer (Layer 5) on Host B

Sender(A) functions:

A_output(): called when app has data to send

A_input(): called when ACk arrives from Receiver
A_timerinterrupt(): called when timer expires
A_intit(): initalize sender state

Receiver(B) Functions:
B_input(): called when packet arrive from sender
B_init(): initialize receiver state
