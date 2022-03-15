import socket
import random
import string
import time

HOST = "127.0.0.1"  
PORT = 9000  
def getRandStr(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return( result_str)
def retRandom():
    if (random.randint(1 ,10) % 2 == 0):
        return p1
    else:
        return p2
        
while(1):
    time.sleep(3)
    p1 = "fup%d"%(random.randint(100 ,100000))
    p2= "fdown%s"%(getRandStr(int(random.randint(5 ,10))))
    print(p1)
    print(p2)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(str.encode(p1 if(random.randint(5 ,10)%2==0) else p2))
        data = s.recv(1024)
        print(f"Received {data!r}")
