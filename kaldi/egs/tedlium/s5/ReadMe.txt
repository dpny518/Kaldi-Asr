脚本运行记录：

	1. 运行./run.sh  下载对应的音频数据后

	2. 修改词典内容，将乱码和空的单词对应的音素  去掉，不然准备字典文件（lexion.txt）会失败

	3. 将词典文件拷贝到 db/cantab-TEDLIUM目录下 改名cantab-TEDLIUM.dct

	4. 将语言模型文件压缩

		压缩命令：tar zcvf cantab-TEDLIUM-pruned.lm3.gz cantab-TEDLIUM-unpruned.lm4
