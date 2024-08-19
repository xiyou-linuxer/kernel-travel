all:
	mkdir -p test
	loongarch64-linux-gnu-gcc git_hash.c -o ./test/git --static

local:
	mkdir -p test
	gcc git_hash.c -g -o ./test/git
	# cp ./git-test.sh ./test/
	# chmod +x ./test/git-test.sh  # 设置可执行权限
	# cd test; ./git-test.sh


clean:
	rm -rf test
