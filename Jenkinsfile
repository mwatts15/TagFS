build="build"

if (false) {
node("ubuntu || debian") {
    stage ('Gather Unit Test Pre-reqs') {
        sh "mkdir -p /etc/apt/sources.list.d/"

        // Adding for the latest version of cunit
        sh "sudo sh -c 'echo deb http://ftp.us.debian.org/debian/ sid main non-free contrib > /etc/apt/sources.list.d/sid.list'"
        sh "sudo apt-get update && sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
           "libtool-bin libsqlite3-dev libdbus-1-dev libdbus-glib-1-dev " +
           "libattr1-dev lcov libcunit1-dev make"
    }

    stage ('Build and Unit Test') {
        env.TESTS_MACHINE_OUTPUT = 1
        checkout scm
        env.LD_LIBRARY_PATH='/usr/local/lib'
        withEnv(['NO_VALGRIND=1', 'COVERAGE=1']) {
            sh "make clean tests"
        }
        stash name: 'unit_test_result', includes: 'src/tests/unit-test-results/*/*-Results.xml'
        stash name: 'unit_test_coverage', includes: 'src/tests/test-coverage/**/*'
    }
}

node("ubuntu || debian") {
    stage ('Gather Acceptance Test Pre-reqs') {
        sh "sudo apt-get update && sudo apt-get install -y libglib2.0-dev libfuse-dev valgrind perl " +
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

    if (false) {
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
}
}
node ("ubuntu || debian") {
    stage ('Make source archive') {
        scm.noTags = false
        checkout scm
        sh "sudo apt-get update && sudo apt-get install -y bzip2 make fakeroot"
        sh "make ${build}/tagfs.tar.bz2"
        stash name: 'source_archive', includes: "${build}/tagfs.tar.bz2"
    }

    stage ('Make debian source archives') {
        checkout scm
        sh "make clean && make ${build}/tagfs.orig.tar.bz2 ${build}/tagfs.debian.tar.bz2"
        stash name: 'debian_source_archive', includes: "${build}/tagfs_*.orig.tar.bz2,${build}/tagfs_*.debian.tar.bz2"
    }
}

node ("ubuntu || debian") {
    stage ('Make debian package') {
        checkout scm
        unstash 'debian_source_archive'
        sh "sudo apt-get update && sudo apt-get install -y bzip2 make libglib2.0-dev libfuse-dev perl fakeroot " +
           "devscripts libtool-bin libsqlite3-dev libdbus-1-dev libdbus-glib-1-dev libattr1-dev"
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

        if (false) {
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
    }
    stage ('Store artifacts') {
        unstash 'source_archive'
        unstash 'debian source_archive'
        unstash 'debian package'
        archiveArtifacts artifacts: "${build}/*", fingerprint: true, onlyIfSuccessful: true
    }
}

