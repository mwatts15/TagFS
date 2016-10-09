node
{
    stage "Gather bulid pre-reqs"
    sh "sudo sh -c 'echo \"deb http://mirrors.kernel.org/ubuntu trusty main\" >> /etc/apt/sources.list'"
    sh "sudo apt-get update"
    sh "sudo apt-get install libglib2.0-dev libfuse-dev valgrind perl"
    sh "svn co svn://svn.code.sf.net/p/cunit/code/trunk cunit"
    sh "sudo depmod"
    sh "cd cunit"
    sh "./bootstrap"
    sh "make && sudo make install"
    sh "cd .."
    sh "make"
    sh "fusermount -V"
    sh "sudo modprobe fuse"

    stage "Build and Unit Test""
    env.TESTS_MACHINE_OUTPUT = 1"
    sh "make tests"
}

node
{
    stage "Acceptance test"
    sh "make acc-test"
}
