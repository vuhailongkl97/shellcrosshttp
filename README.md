# shellcrosshttp

![remote to BBB](assets/remote_bbb.png)
![cat a BBB](assets/catAFile.png)


supported shell scripting over HTTP, upload files
you can use any supported command on host over HTTP  
example : `ls` `cp`, ...  
Topology

target <==ethernet/wifi==> host

+ step 1  
attach this binary to the target and run it.  
+ step 2   
from host open  <target ip>:1997 on your browser. For testing you also can run directly the binary on host and open http://localhost:1997

# Build & run
`make ; ./ss`

# Test
`make test`

# Install 
`make install`

# Note  
* default port is *1997*

* maximum uploaded file's size is 50Kb

* Currently this software support chrome properly.

# Limitation

* Don't try to use keep session shell commands like `top`, `ping`. Currently it will hang, coundn't stop it unless reboot the target or you must able to remote to the remote and restart `ss` program 
