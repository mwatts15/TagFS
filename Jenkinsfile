build="build"
node("ubuntu || debian") {
    stage ('Gather Unit Test Pre-reqs') {
        // sh "sudo sh -c 'echo \"deb http://mirrors.kernel.org/ubuntu trusty main\" > /etc/apt/sources.list.d/kernel.org.list'"
        sh "sudo apt-get update"
        // CUnit dependencies
        sh "sudo apt-get install -y libtool curl ftjam autoconf automake make bzip2"
        sh "curl http://starstation.home:8080/job/cunit-source/lastSuccessfulBuild/artifact/cunit.tar.bz2 -O"
        sh "tar xvf cunit.tar.bz2 && cd cunit && autoreconf --install && aclocal " +
           "&& automake && chmod u+x configure " +
           "&& ./configure --prefix=/usr/local && make " +
           "&& sudo make install"
        // Tagfs dependencies
        sh "sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
           "libtool-bin libsqlite3-dev libdbus-1-dev libdbus-glib-1-dev " +
           "libattr1-dev lcov"
    }

    stage ('Build and Unit Test') {
        env.TESTS_MACHINE_OUTPUT = 1
        checkout scm
        env.LD_LIBRARY_PATH='/usr/local/lib'
        withEnv(['COVERAGE=1']) {
            sh "make clean tests"
        }
        stash name: 'unit_test_result', includes: 'src/tests/unit-test-results/*/*-Results.xml'
        stash name: 'unit_test_coverage', includes: 'src/tests/test-coverage/**/*'
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
                sh "make clean acc-test"
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

node ("ubuntu || debian") {
    stage ('Make source archive') {
        checkout scm
        sh "sudo apt-get install -y bzip2 make"
        sh "make ${build}/tagfs.tar.bz2"
        stash name: 'source_archive', includes: "${build}/tagfs.tar.bz2"
    }

    stage ('Make debian source archives') {
        checkout scm
        sh "make clean ${build}/tagfs.orig.tar.bz2 ${build}/tagfs.debian.tar.bz2"
        stash name: 'debian_source_archive', includes: "${build}/tagfs_*.orig.tar.bz2,${build}/tagfs_*.debian.tar.bz2"
    }
}

node ("ubuntu || debian") {
    stage ('Make debian package') {
        checkout scm
        unstash 'debian_source_archive'
        sh "make ${build}/tagfs.deb"
        stash name: 'debian_package', includes: "${build}/tagfs_*.deb"
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

    stage ('Store artifacts') {
        unstash 'source_archive'
        unstash 'debian source_archive'
        unstash 'debian package'
        archiveArtifacts artifacts: "${build}/*", fingerprint: true, onlyIfSuccessful: true
    }
}

