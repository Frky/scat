#-*- coding: utf-8 -*-

import subprocess
import os
import shutil
from datetime import datetime
from shutil import copyfile

FN_MODE_NAME = "name"
FN_MODE_ADDR = "addr"
FN_MODE_DEFAULT = FN_MODE_NAME

INF_ARITY = 0
INF_TYPE = 1
INF_COUPLE = 2
INF_ALLOC = 3
INF_UAF = 4
INF_MEM_MAP = 5
INF_BASE = 6

INF_ALL = [
            (INF_BASE, "base"),
            (INF_ARITY, "arity"),
            (INF_TYPE, "type"),
            (INF_COUPLE, "couple"),
            (INF_ALLOC, "alloc"),
            (INF_UAF, "uaf"),
            (INF_MEM_MAP, "memmap"),
        ]

def inf_code_to_str(code):
    global INF_ALL
    for c, name in INF_ALL:
        if c == code:
            return name
    return "unknown"


def inf_str_to_code(s):
    global INF_ALL
    for c, name in INF_ALL:
        if name == s:
            return c
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
    if code == INF_MEM_MAP:
        return INF_TYPE
    if code == INF_BASE:
        return -1


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
        self.compile_flags = kwargs["compile_flags"]
        # Name of pintool for arity, type and couple
        for code, pt in INF_ALL:
            if pt + "_obj" in kwargs.keys():
                self.pintool[code] = kwargs[pt + "_obj"]
            else:
                self.pintool[code] = pt + ".so"
            if pt + "_src" in kwargs.keys():
                self.src[code] = kwargs[pt + "_src"]


    def log(self, msg, verbose=True):
        if self.__log is not None and verbose:
            self.__log(msg)


    def __cmd(self, pintool, binary, args, logfile, infile=None):
        if infile is not None:
            infile_opt = "-i {0}".format(infile)
        else:
            infile_opt = ""
        return "{0} {4} -t {1} -o {2} -fn \"{7}\" {3} -- {5} {6}".format(self.pinbin, pintool, logfile, infile_opt, self.cli_options, binary, " ".join(args), self.fn_mode)


    def infer(self, inf_code, binary, args, logfile, infile=None, verbose=True):
        """
            Launch specified inference on binary given in parameter

            @param inf_code code corresponding to the inference to launch
                            (must be an element of the list INF_CODES)

            @param binary   the binary file to analyse (must be a valid path to
                            an executable)

            @param args     arguments to give to the binary

            @param logfile  path to the log file where arity information
                            is stored (must be a valid path)

            @param infile   path to the file where previous inference result is
                            stored (must be a valid path)

            @param verbose  if True, print intermediate steps

        """
        if inf_code == INF_BASE:
            cmd = "{0} {1}".format(binary, " ".join(args))
        else:
            cmd = self.__cmd(self.pintool[inf_code], binary, args, logfile, infile)
        self.log(cmd, verbose)
        start = datetime.now()
        subprocess.call(cmd, shell=True)
        duration = datetime.now() - start
        self.log("Inference results logged in {0}".format(logfile), verbose)
        self.log("Execution time: {0}.{1}s".format(duration.seconds, duration.microseconds), verbose)


    def compile(self, force, debug, trace, pintools=[]):
        """
            Compile all pintools needed

        """
        #TODO os independant

        force_flag = ''
        if force:
            force_flag = '-B'

        if len(pintools) == 0:
            inf = INF_ALL
        else:
            inf = pintools

        for code in [c for c, n in inf if c in self.src.keys()]:
            src_path, src_name = os.path.split(self.src[code])

            obj_path, obj_name = os.path.split(self.pintool[code])
            obj_path = os.path.abspath(obj_path)

            obj_build_path = "{}/build".format(obj_path)
            obj_build_name = src_name[:-4]
            if debug or trace:
                if debug:
                    obj_build_name += '-debug'
                if trace:
                    obj_build_name += '-trace'
            else:
                obj_build_name += '-release'

            if not os.path.exists(obj_build_path):
                os.makedirs(obj_build_path)

            obj_build_file = "{}/{}.so".format(obj_build_path, obj_build_name)
            if os.path.exists(obj_build_file):
                mtime_before = os.stat(obj_build_file).st_mtime
            else:
                mtime_before = 0

            cmd = "make {} PIN_ROOT='{}' SCAT_COMPILE_FLAGS='{}' OBJDIR='{}/' '{}/{}.so'".format(
                    force_flag,
                    self.pinpath,
                    self.compile_flags,
                    obj_build_path,
                    obj_build_path,
                    obj_build_name)

            self.log("Compiling pintool: {0} ...".format(src_name[:-4]))
            with open("/dev/null", 'w') as fnull:
                try:
                    subprocess.check_call(cmd, cwd=src_path, shell=True, stdout=fnull)
                    mtime_now = os.stat("{}/{}.so".format(obj_build_path, obj_build_name)).st_mtime
                    shutil.copyfile(
                            '{}/{}.so'.format(obj_build_path, obj_build_name),
                            '{}/{}'.format(obj_path, obj_name))
                    if mtime_before == mtime_now:
                        self.log("   => Up to date !")
                    else:
                        self.log("   => Done !")
                except subprocess.CalledProcessError as error:
                    self.log("/!\ Compilation exited with non-zero status {} /!\\\n\n".format(error.returncode))
