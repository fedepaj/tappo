# tappo
What the name syas.

# Network setup
``` 
sudo ./RIOT/dist/tools/tapsetup/tapsetup
sudo ip a a fec0:affe::1/64 dev tapbr0
sudo ip -6 r add <tap0 address> dev tapbr0
```
