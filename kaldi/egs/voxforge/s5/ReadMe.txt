脚本执行：

1. 执行脚本  ./getdata.sh   下载语料库和对应的数据文件，为了数据准备过程做准备

2. 修改cmd.sh

	export train_cmd=run.pl
	export decode_cmd=run.pl
	export mkgraph_cmd=run.pl

3. 此训练过程需要srilm 工具构建语言模型文件

	按照网址下载srilm.tgz，然后运行install_srilm.sh
	提示安装
	
	sudo ./install_sequitur.sh
	sudo apt-get install swig

	这两个库一定要安装，在数据准备通过此工具生成
	
4. 网站下载的数据里面可能会有些数据的性别信息不全，需要手动改下  

	对应的路径： extracted/*/etc/READEME  Gender字段
	
	
5. make_trans.py文件修改(s5/local)

	30 行的file方法改成open方法
	
	其他的print 输出命令关系不大


	
	