# memIni
readini from memory,sync inifile to memory 5 seconds

//
//		  create by liwei
//			内存获取ini配置，模块内并定时同步ini文件到内存中	 ,优点:
//			1、减少io操作，提升性能； 	 比windows 函数提升20倍
//          2、避免其他进程在修改ini文件时和windows系统获取配置的GetPrivateProfileInt等函数文件冲突，导致ini 文件被清零
//
//          注意： ini 文件中如果存在重复项，原先windows函数不会出问题，但tcini会抛出异常，异常ini的行号并返回在	lpErrCallBack 函数中		
//                ini 文件的注释不应该使用"//"，应该使用";","#"
//
//			程序启动时会备份一个.tcini文件
