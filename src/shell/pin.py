#-*- coding: utf-8 -*-

import subprocess
import os
import shutil
from shutil import copyfile

FN_MODE_NAME = "name"
FN_MODE_ADDR = "addr"
FN_MODE_DEFAULT = FN_MODE_NAME

INF_ARITY = 0
INF_TYPE = 1
INF_COUPLE = 2
INF_ALLOC = 3
INF_UAF = 4

INF_CODES = [INF_ARITY, INF_TYPE, INF_COUPLE, INF_ALLOC, INF_UAF]

def inf_code_to_str(code):
    if code == INF_ARITY:
        return "arity"
    if code == INF_TYPE:
        return "type"
    if code == INF_COUPLE:
        return "couple"
    if code == INF_ALLOC:
        return "alloc"
    if code == INF_UAF:
        return "uaf"
    return "unknown"


def inf_str_to_code(s):
    if s == "arity":
        return INF_ARITY
    if s == "type":
        return INF_TYPE
    if s == "couple":
        return INF_COUPLE
    if s == "alloc":
        return INF_ALLOC
    if s == "uaf":
        return INF_UAF
    return -1


def get_previous_step(code):
    if code == INF_ARITY:
        return -1
    if code == INF_TYPE:
        return INF_ARITY
    if code == INF_COUPLE:
        return INF_TYPE
    if code == INF_ALLOC:
        return INF_COUPLE
    if code == INF_UAF:
        return INF_TYPE


class Pin(object):
    """
        This class defines an interface between python and
        our implementation using pin.

    """

    def __init__(self, **kwargs):
        # Log function
        if "log" in kwargs.keys():
            self.__log = kwargs["log"]
        else:
            self.__log = None
        # Path to pin folder
        if "pinpath" in kwargs.keys():
            self.pinpath = kwargs["pinpath"]
        else:
            self.pinpath = ""
        # Path to Pin executable
        if "pinbin" in kwargs.keys():
            self.pinbin = kwargs["pinbin"]
        else:
            self.pinbin = ""
        # Path to pin makefile
        if "respath" in kwargs.keys():
            self.respath = kwargs["respath"]
        else:
            self.respath = ""
        if "fn_mode" in kwargs.keys():
            self.fn_mode = kwargs["fn_mode"]
        else:
            self.fn_mode = FN_MODE_DEFAULT
        # Dictionary for pintool names and src
        self.pintool = dict()
        self.src = dict()
        self.cli_options = kwargs["options"]
        # Name of pintool for arity, type and couple
        for code in INF_CODES:
            pt = inf_code_to_str(code)
            if pt + "_obj" in kwargs.keys():
                self.pintool[code] = kwargs[pt + "_obj"]
            else:
                self.pintool[code] = pt + ".so"
            if pt + "_src" in kwargs.keys():
                self.src[code] = kwargs[pt + "_src"]


    def log(self, msg):
        if self.__log is not None:
            self.__log(msg)


    def __cmd(self, pintool, binary, args, logfile, infile=None):
        if infile is not None:
            infile_opt = "-i {0}".format(infile)
        else:
            infile_opt = ""
        return "{0} {4} -t {1} -o {2} -fn \"{7}\" {3} -- {5} {6}".format(self.pinbin, pintool, logfile, infile_opt, self.cli_options, binary, " ".join(args), self.fn_mode)


    def infer(self, inf_code, binary, args, logfile, infile=None):
        """
            Launch specified inference on binary given in parameter

            @param inf_code code corresponding to the inference to launch
                            (must be an element of the list INF_CODES)

            @param binary   the binary file to analyse (must be a valid path to
                            an executable path

            @param args     arguments to give to the binary

            @param logfile  path to the log file where arity information
                            is stored (must be a valid path)

            @param infile   path to the file where previous inference result is
                            stored (must be a valid path)

        """
        cmd = self.__cmd(self.pintool[inf_code], binary, args, logfile, infile)
        self.log(cmd)
        subprocess.call(cmd, shell=True)
        self.log("Inference results logged in {0}".format(logfile))


    def compile(self):
        """
            Compile all pintools needed

        """
        #TODO NOT FLEXIBLE AT ALL
        #TODO
        #    - os independant
        #    - configuration-file adaptable
        #    - check compilation success
        wd = self.pinpath + "/source/tools/pinalloc/"
        # Create pinalloc directory if does not exist
        if not os.path.exists(wd):
            os.mkdir(wd)
        # Create obj-intel64 if does not exist
        if not os.path.exists(wd + "/obj-intel64"):
            os.mkdir(wd + "/obj-intel64")
        # Link makefile if does not exist
        for makefile in ["makefile", "makefile.rules"]:
            if not os.path.exists(wd + makefile):
                shutil.copyfile(self.respath + "/" + makefile, wd + "/" + makefile)
        # Add utils directory
        shutil.rmtree(wd + "/utils", True)
        os.mkdir(wd + "/utils")
        for dirpath, dirnames, filenames in os.walk("./src/pintool/utils"):
            for fname in filenames:
                copyfile(dirpath + "/" + fname, wd + "/utils/" + fname)
        for code in [c for c in INF_CODES if c in self.src.keys()]:
            pfile = self.src[code]
            self.log("Compiling {0} ...".format(pfile))
            copyfile(pfile, wd + os.path.basename(pfile))
            cmd = "make obj-intel64/" + os.path.basename(pfile)[:-3] + "so"
            with open("/dev/null", 'w') as fnull:
                subprocess.call(cmd, cwd=wd, shell=True, stdout=fnull)
            copyfile(wd + "obj-intel64/" + os.path.basename(pfile)[:-3] + "so", self.pintool[code])

