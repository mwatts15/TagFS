node
{
    stage "Gather unit test pre-reqs"
    #sh "sudo sh -c 'echo \"deb http://mirrors.kernel.org/ubuntu trusty main\" > /etc/apt/sources.list.d/kernel.org.list'"
    sh "sudo apt-get update"
    sh "sudo apt-get install libglib2.0-dev libfuse-dev valgrind perl"
    sh "svn co svn://svn.code.sf.net/p/cunit/code/trunk cunit"
    sh "cd cunit"
    sh "./bootstrap"
    sh "make && sudo make install"
    sh "cd .."
    sh "make"
    sh "fusermount -V"

    stage "Build and Unit Test"
    env.TESTS_MACHINE_OUTPUT = 1
    sh "make tests"
}

node
{
    stage "Gather acceptance test pre-reqs"
    sh "sudo apt-get update"
    sh "sudo apt-get install libglib2.0-dev libfuse-dev valgrind perl"
    sh "sudo modprobe fuse"

    stage "Acceptance test"
    sh "make acc-test"
}
