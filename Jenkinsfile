node
{
    stage "Build and Unit Test"
    env.TESTS_MACHINE_OUTPUT = 1
    sh "make tests"
}

node
{
    stage "Acceptance test"
    sh "make acc-test"
}
