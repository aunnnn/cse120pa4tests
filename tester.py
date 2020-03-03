import subprocess
from subprocess import Popen, PIPE, STDOUT
import os
import argparse

# Update number here if you add more tests
N_tests = 17

def run_tests(outputfile, ref_mode=False):
    print('Make clean pa4tests...'),
    if ref_mode:
        subprocess.call('make clean tests OPTION=-DREF', stdout=PIPE, shell=True)
    else:
        subprocess.call('make clean tests', stdout=PIPE, shell=True)

    print('\t\tDone.')

    print('Running all tests...')

    with open(outputfile, 'w') as outFile:
        # Then run all tests
        for i in range(1, N_tests+1):
            print("* Run test " + str(i) + "..."),
            outFile.write('\n-----TEST' + str(i) + '-----\n')
            outFile.flush()
            cmd = ['./tests']
            my_env = os.environ.copy()
            my_env["N"] = str(i)
            # result = Popen(cmd, stdout=outFile, stderr=STDOUT, env=my_env).wait()

            proc = Popen(cmd, stdout=PIPE, stderr=STDOUT, env=my_env)
            
            # Detect failure manually
            is_failed = False
            for line in proc.stdout:
                if 'ASSERTION FAILURE:' in line or 'Kernel Panic!' in line: 
                    is_failed = True
                outFile.write(line)
            proc.wait()

            outFile.flush()
            outFile.write('\n----------------\n')
            outFile.flush()

            if is_failed:
                print("\t\tFailed!?")
            else:
                print("\t\tNo errors encountered, compare with ref to ensure correctness.")

        print("All tests ran (N={}).".format(N_tests))

parser = argparse.ArgumentParser()

# dest is important so we can distinguish which sub-command it is
subparsers = parser.add_subparsers(dest='which')
subparsers.required = True

parser_runtests = subparsers.add_parser('runtests', help='Run all tests, write output to a file. Can also be used to generate ref output.')
parser_runtests.add_argument('-r', '--ref', help='Ref mode, run tests using Prof. version.', action='store_true')

args = parser.parse_args()

print(args)

if args.which == 'runtests':

    if not os.path.exists('./tester'):
        os.makedirs('./tester')
        print("Folder 'testers' created.")
    else:
        print("Folder 'testers' already exists.")

    is_refmode = args.ref
    if is_refmode:
        print("\nRun tests in REF mode (ref_outputs.txt)...\n")
        run_tests('./tester/ref_outputs.txt', ref_mode=True)
        print("Check output at `ref_outputs.txt`.")
    else:
        print("\nRun tests in My mode (test_outputs.txt)...\n")
        run_tests('./tester/test_outputs.txt', ref_mode=False)
        print("Check output at `test_outputs.txt`.")
