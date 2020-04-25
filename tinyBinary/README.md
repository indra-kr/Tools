# tinyBinary

See [cmdReceiver](https://github.com/indra-kr/Tools/tree/master/cmdReceiver)

```
/------------\    
| do exploit | 
\------------/
      |   +-----------------------------+
      +-> | execve("vul",["vul"],NULL); | <- Area of exploit process
          |             ...             | -+
          +-----------------------------+  |
                            +-------------------------+ <- Area of target process
execution of constructor -> | dlopen("evil.so", ...); |
                            |            ...          |    +---------------------------------+ <- .ctors
       execution of main -> |     mprotect(...);      | == |      setgid(0);setuid(0);       |
                            +-------------------------+    | chown("/tmp/.1ndr4-root",0,0);  |
                                                           | chmod("/tmp/.1ndr4-root",04777);|
                                                           +---------------------------------+
                                                                            |
                                                                +-----------------------+ <- main
                                                                | Tatget's main() logic |
                                                                |          ...          |
                                                                +-----------------------+
```
