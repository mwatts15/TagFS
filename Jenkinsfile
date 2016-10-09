node("ubuntu")
{
    stage ('Gather unit test pre-reqs') {
        // sh "sudo sh -c 'echo \"deb http://mirrors.kernel.org/ubuntu trusty main\" > /etc/apt/sources.list.d/kernel.org.list'"
        sh "sudo apt-get update"
        // Tagfs dependencies
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl"
        // CUnit dependencies
        sh "sudo apt-get install -y libtool subversion ftjam autoconf automake"

        sh "svn co svn://svn.code.sf.net/p/cunit/code/trunk cunit"
        sh "cd cunit && autoreconf --install && aclocal " +
           "&& automake && chmod u+x configure " +
           "&& ./configure --prefix=/usr/local && make " +
           "&& sudo make install"
    }

    stage ('Build and Unit Test')
    {
        env.TESTS_MACHINE_OUTPUT = 1
        sh "make tests"
    }
}

node("ubuntu")
{
    stage ('Gather acceptance test pre-reqs') {
        sh "sudo apt-get update"
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl"
        sh "sudo modprobe fuse"

        stage "Acceptance test"
        sh "make acc-test"
    }
}
