node("ubuntu || debian") {
    stage ('Gather Unit Test Pre-reqs') {
        // sh "sudo sh -c 'echo \"deb http://mirrors.kernel.org/ubuntu trusty main\" > /etc/apt/sources.list.d/kernel.org.list'"
        sh "sudo apt-get update"
        // CUnit dependencies
        sh "sudo apt-get install -y libtool subversion ftjam autoconf automake make"

        timeout(time: 30, unit: 'SECONDS') {
            sh "svn co svn://svn.code.sf.net/p/cunit/code/trunk cunit"
        }
        sh "cd cunit && autoreconf --install && aclocal " +
           "&& automake && chmod u+x configure " +
           "&& ./configure --prefix=/usr/local && make " +
           "&& sudo make install"
        // Tagfs dependencies
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
           "libtool-bin libsqlite3-dev libdbus-1-dev libdbus-glib-1-dev libattr1-dev"
    }

    stage ('Build and Unit Test') {
        env.TESTS_MACHINE_OUTPUT = 1
        checkout scm
        env.LD_LIBRARY_PATH='/usr/local/lib'
        sh "make tests"
    }
}

node("ubuntu || debian") {
    stage ('Gather Acceptance Test Pre-reqs') {
        sh "sudo apt-get update"
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
           "sqlite3 libdbus-glib-1-dev make fuse attr"
    }

    stage ('Acceptance test') { 
        checkout scm
        env.LD_LIBRARY_PATH='/usr/local/lib'
        wrap([$class: 'AnsiColorBuildWrapper', 'colorMapName': 'XTerm']) {
            sh "make acc-test"
        }
    }
    
}
