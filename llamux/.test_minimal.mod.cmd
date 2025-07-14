savedcmd_/root/Idea/llamux/test_minimal.mod := printf '%s\n'   test_minimal.o | awk '!x[$$0]++ { print("/root/Idea/llamux/"$$0) }' > /root/Idea/llamux/test_minimal.mod
