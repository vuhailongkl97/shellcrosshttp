# shellcrosshttp
![remote to BBB](remote_bbb.png)
![cat a BBB](catAFile.png)


shell scripting over HTTP 
you can use any supported command on host over HTTP  
example : `ls` `cp`, ...  
Topology

target <==ethernet/wifi==> host

+ step 1  
attach this binary to target and run it.  
+ step 2   
from host open 192.168.0.1:1997 on your browser

