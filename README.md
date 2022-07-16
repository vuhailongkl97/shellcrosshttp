# shellcrosshttp
shell scripting over HTTP 
you can use any supported command on host over HTTP  
example : `ls` `cp`, ...  
Topology

target <==ethernet/wifi==> host

+ step 1  
attach this binary to target and run it.  
+ step 2   
from host create request to target over http like this
```
// call curl from target 
// target IP:port : 192.168.0.1:1997  
// in target : it run curl 192.168.0.41:8080/logo_index.png --ouput /www/web/image/logo_index.png

curl 192.168.0.1:1997/curlQ192.168.0.41:8080/logo_index.pngQ--outputQ/www/web/image/logo_index.png
```

#Note
space = 'Q'
