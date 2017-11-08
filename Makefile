mptcp: mpsend.h mpconnect.h mpsend.c mpconnect.c main.c
	clang -Wall -o mptcp main.c mpconnect.c mpsend.c -L$(CURDIR) -lmptcp_32