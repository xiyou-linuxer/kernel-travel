#!/usr/bin/busybox sh

set -x

function check_ret() {
    ret=$?
    if [ $ret -eq 0 ]; then 
        echo "=================== $1 success ==================="
    else
        echo "=================== $1 return $ret ==================="
        exit 128
    fi
}

function test_1() {
    echo "==== GIT TEST 1 ==="
    ./git --help
    check_ret "git --help"
    echo "==== FINISH TEST 1 ===="
}

function test_2() {
    echo "==== GIT TEST 2 ==="
    rm -rf test_git
    mkdir test_git 
    cd test_git
    ../git init
    check_ret "git init"
    ls .git
    check_ret "ls .git"
    echo -e "AAAAA11111\nBBBBB11111\n" > a.txt
    ../git add a.txt
    check_ret "git add a.txt"
    ../git commit -m "init commit"
    check_ret "git commit -m \"init commit\""
    echo -e "AAAAA11111\nBBBBB11111\nCCCCC22222" > a.txt
    ../git diff a.txt
    ../git add a.txt
    check_ret "git add a.txt"
    ../git commit -m "update a.txt"
    check_ret "git commit -m \"update a.txt\""
    ../git reflog
    check_ret "git reflog"
    cd ..
    echo "==== FINISH TEST 2 ===="
}

# GIT_REPO=192.168.10.210/student
function test_3() {
    echo "=== START TEST 3 ==="
    rm -rf os-2024-http
    git clone http://$GIT_REPO/os-2024-http
    check_ret "git clone http://$GIT_REPO/os-2024-http"
    cat os-2024-http/README.md

    rm -rf os-2024-http
    mkdir os-2024-http
    cd os-2024-http
    git init
    git remote add origin http://$GIT_REPO/os-2024-http
    check_ret "git remote add origin http://$GIT_REPO/os-2024-http"
    git pull origin main
    check_ret "git pull origin main"
    cat README.md
    cd ..
    echo "=== FINISH TEST 3 ===="
}

# GIT_REPO_SSH=192.168.10.210:2222/student
function test_4() {
    echo "=== START TEST 4 ===="
    rm -rf os-2024-ssh
    #git clone ssh://user@$GIT_REPO/os-2024-ssh
git clone --config core.sshCommand="ssh -i git-key/students.key" ssh://git@$GIT_REPO_SSH/os-2024-ssh

    check_ret "git clone ssh://git@$GIT_REPO_SSH/os-2024-ssh"
    cat os-2024-http/README.md

    #rm -rf os-2024-ssh
    #mkdir os-2024-ssh
    #cd os-2024-ssh
    #git init
    #git remote add origin ssh://git@$GIT_REPO_SSH/os-2024-ssh
    #check_ret "git remote add origin  ssh://git@$GIT_REPO_SSH/os-2024-ssh"
    #git pull origin main
    #check_ret
}

test_1
test_2
# test_3
# test_4

