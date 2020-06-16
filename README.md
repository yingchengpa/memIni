# memIni

## 说明

read inifile from cache,sync inifile to cache per 5 seconds.

create by liwei

## 优点 

内存获取ini配置，并定时同步ini文件到内存中,优点:

- 减少io操作，提升性能； 	 比windows 函数提升20倍

- 避免其他进程在修改ini文件时和windows系统获取配置的GetPrivateProfileInt等函数文件冲突，导致ini文件被清零

## 注意
ini 文件中如果存在重复项，原先windows函数不会出问题，但memini会抛出异常，异常ini的行号并返回在 lpErrCallBack 函数中	

- ini 文件的注释不应该使用"//"，应该使用";"或者"#";

~~~
error :

//this is a key 
[svrcfg]
ip=192.168.1.1

right :

#this is a key 
[svrcfg]
ip=192.168.1.1
~~~

- ini 文件不能存在重复项
  
~~~
error:
[svrcfg]
ip=192.168.1.1

[svrcfg]
ip=192.168.1.2

right: 
[svrcfg]
ip=192.168.1.1

[svrcfg1]
ip=192.168.1.2
~~~
