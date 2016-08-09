
import os, glob
import subprocess

from src.shell.parser.type import TypeLogParser


class ScatTest(object):
    
    DELIMITOR = "// ORACLE "

    def __init__(self, **kwargs):
        self.src = dict()
        self.bin = dict()
        self.log = dict()
        self.root = dict()
        self.__log = kwargs["log"]
        if "proto" in kwargs.keys():
            if "root" in kwargs["proto"].keys():
                self.root["proto"] = kwargs["proto"]["root"]
            if "src" in kwargs["proto"].keys():
                self.src["proto"] = kwargs["proto"]["src"]
            if "bin" in kwargs["proto"].keys():
                self.bin["proto"] = kwargs["proto"]["bin"]
            if "log" in kwargs["proto"].keys():
                self.log["proto"] = kwargs["proto"]["log"]

    def __log_file(self, bin_path, pintool):
        return "{2}/{0}_{1}.log".format(os.path.basename(bin_path), pintool, self.log["proto"])

    def __ok(self, fname):
        self.__log("\t[ok] {0}".format(fname))

    def __ko(self, fname, oracle, infered):
        self.__log("\t[ko] {0} -- (real) {1} | (infered) {2}".format(fname, oracle, infered))

    def __compare(self, fname, oracle, infered):
        if oracle == infered:
            self.__ok(fname)
            return 0
        else:
            self.__ko(fname, oracle, infered)
            return 1

    def oracle(self, src_path, log_path):
        """
            Get the oracle from source code
            Each line of the oracle describes the prototype of 
            one function, and must start with ScatTest.DELIMITOR

            Example of oracle (in source file):

            // ORACLE VOID foo(INT, ADDR)

        """
        oracle = dict()
        # Parse results
        log = TypeLogParser(log_path)
        # Local number of failures
        ko = 0
        # Local number of tries
        tot = 0
        # Parse oracle 
        with open(src_path, "r") as src:
            for line in src.readlines():
                if line.startswith(ScatTest.DELIMITOR) > 0:
                    line = line.replace(ScatTest.DELIMITOR, "")
                    # Get output type
                    out = line[:line.index(" ")]
                    # Get function name
                    fname = line[line.index(" ") + 1:line.index("(")]
                    # Get parameters
                    params = line[line.index("(") + 1:line.index(")")].replace(" ", "").split(",")
                    if params == ["VOID"]:
                        params = []
                    oracle[fname] = [out] + params
                    ko += self.__compare(fname, oracle[fname], log.get_proto(fname))
                    tot += 1
        return ko, tot

    def unit_test(self, src_path, bin_path, pintools):
        self.__log("{0}".format(bin_path))
        for p in pintools:
            p.launch(
                    bin_path, 
                    "", 
                    False,  # Set this to True for verbose debug
                )
        return self.oracle(src_path, self.__log_file(bin_path, pintools[1]))

    def make(self):
        self.__log("Compiling tests ...")
        cmd = "make"
        with open("/dev/null", 'w') as fnull:
            subprocess.call(cmd, cwd=self.root["proto"], shell=True, stdout=fnull)

    def proto(self, infer):
        # Compile all 
        self.make()
        # Global number of failures
        ko = 0
        # Global number of tries
        tot = 0
        for bin_path in glob.glob(self.bin["proto"] + "/*"):
            # Ignore .init file
            if os.path.basename(bin_path) == ".init":
                pass
            # Get corresponding source code
            src_path = os.path.join(self.src["proto"], "{0}.c".format(os.path.basename(bin_path)))
            loc_ko, loc_tot = self.unit_test(src_path, bin_path, infer)
            ko += loc_ko
            tot += loc_tot
        self.__log("TEST RESULTS: {0}/{1} ({2:0.2f}%)".format(tot - ko, tot, (tot - ko) / (0.01*tot)))

