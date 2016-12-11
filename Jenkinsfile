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
        withEnv(['COVERAGE=1']) {
            sh "make tests"
        }
        stash name: 'unit_test_result', includes: 'src/tests/unit-test-results/*/*-Results.xml'
        stash name: 'unit_test_coverage', includes: 'test-coverage/**/*'
    }
}

node("ubuntu || debian") {
    stage ('Gather Acceptance Test Pre-reqs') {
        sh "sudo apt-get update"
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
           "sqlite3 libdbus-glib-1-dev make fuse attr libxml-writer-perl " +
           "libdatetime-perl"
        env.LD_LIBRARY_PATH='/usr/local/lib'
    }

    stage ('Acceptance test') {
        checkout scm
        withEnv(['NO_VALGRIND=1', 'SHOW_LOGS=1', 'KEEP_LOGS=1']) {
            wrap([$class: 'AnsiColorBuildWrapper', 'colorMapName': 'XTerm']) {
                sh "make acc-test"
            }
        }
        stash name: 'acc_test_result', includes: 'src/tests/acc-test-results/junit-acc-test-results.xml'
    }

    if (currentBuild.result == null) {
        stage ('Acceptance test with valgrind') {
            checkout scm
            wrap([$class: 'AnsiColorBuildWrapper', 'colorMapName': 'XTerm']) {
                sh "make acc-test"
            }
            stash name: 'acc_test_with_valgrind_result', includes: 'src/tests/acc-test-results/junit-acc-test-results.xml'
        }
    }
}

node ("master") {
    stage ('Publish test results') {
        unstash 'unit_test_result'
        step([$class: 'XUnitPublisher',
              testTimeMargin: '3000',
              thresholdMode: 1,
              thresholds: [[$class: 'FailedThreshold',
                            failureNewThreshold: '',
                            failureThreshold: '2',
                            unstableNewThreshold: '',
                            unstableThreshold: '1'],
                           [$class: 'SkippedThreshold',
                            failureNewThreshold: '',
                            failureThreshold: '',
                            unstableNewThreshold: '',
                            unstableThreshold: '10']],
              tools: [[$class: 'CUnitJunitHudsonTestType',
                       deleteOutputFiles: true,
                       failIfNotNew: true,
                       pattern: 'src/tests/unit-test-results/*/*-Results.xml',
                       skipNoTestFiles: false,
                       stopProcessingIfError: true]]])

        unstash 'acc_test_result'
        junit 'src/tests/acc-test-results/junit-acc-test-results.xml'

        unstash 'acc_test_with_valgrind_result'
        junit 'src/tests/acc-test-results/junit-acc-test-results.xml'
        unstash 'unit_test_coverage'
        publishHTML([allowMissing: false,
                     alwaysLinkToLastBuild: false,
                     keepAll: false,
                     reportDir: 'src/tests/test-coverage',
                     reportFiles: 'index.html',
                     reportName: 'Unit Test Coverage'])
    }

    if (build.result == null)
    {
        stage ('Build and store source archive') {
            sh "make tagfs.tar.bz2"
            archive includes: "tagfs.tar.bz2"
        }
    }
}

