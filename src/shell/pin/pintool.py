#-*- coding: utf-8 -*-

import os
import subprocess
import shutil
from datetime import datetime

from src.shell.analysis.arity import ArityAnalysis
from src.shell.analysis.type import TypeAnalysis
from src.shell.analysis.couple import CoupleAnalysis
from src.shell.analysis.analysis import Analysis

class Pintool(object):

    nb_pintools = 0

    def __init__(self, *args, **kwargs):
        # Check for required parameters in kwargs
        req_param = ["name", "src_path", "obj_path", "pinconf", "log_manager"]
        for param in req_param:
            if param not in kwargs.keys():
                print "ERROR: parameter {0} expected".format(param)
                raise Exception
        self.__name = kwargs["name"]
        self.__src_path = kwargs["src_path"]
        self.__obj_path = kwargs["obj_path"]
        self.__pinconf = kwargs["pinconf"]
        self.__log_manager = kwargs["log_manager"]

        # LOG function provided?
        if "stdout" in kwargs.keys():
            stdout = kwargs["stdout"]
        else:
            # If not, use the local one
            def stdout(msg, verbose):
                return self.__log(msg, "[*] ", verbose)

        self.stdout = stdout

        if "stderr" in kwargs.keys():
            stderr = kwargs["stderr"]
        else:
            # If not, use the local one
            def stderr(msg):
                return self.__log(msg, "*** ", True)
        self.stderr = stderr

        # Set pintool code
        self.__code = Pintool.nb_pintools
        # Increment number of pintools created
        Pintool.nb_pintools += 1

    @property
    def code(self):
        return self.__code

    def __str__(self):
        return self.__name

    def __log(self, msg, pattern, verbose):
        if verbose:
            print(pattern + msg)

    def cli_log(self, binary):
        """
            Generate a command line arguments for pin.
        """
        pin_name = str(self)
        commandline = ""
        debug = self.__log_manager.gen_log(pin_name, binary, "dbg")
        output = self.__log_manager.gen_log(pin_name, binary)
        commandline += "-o {} -logfile {} ".format(output, debug)
        if "dependencies" in (self.
                __log_manager._config["pintool"][pin_name].keys()):
            dependencies = (self.__log_manager
                    ._config["pintool"][pin_name]["dependencies"])
            if isinstance(dependencies, list):
                if '' in dependencies:
                    dependencies.remove('')
                for k in dependencies:
                    log = self.__log_manager.get_log(k, binary)
                    commandline += "-{} {} ".format(k, log)
            else:
                log = self.__log_manager.get_log(dependencies, binary)
                commandline += "-{} {} ".format(dependencies, log)

        return commandline



    def __cmd(self, binary, pin_args, args):
        if pin_args == []:
            pin_args = ""
        if "cli-options" in self.__pinconf.keys():
            cli_options = self.__pinconf["cli-options"]
        else:
            cli_options = ""
        return "{} {} {} -t {} {} -- {} {}".format(
                self.__pinconf["bin"],
                cli_options,
                pin_args,
                self.__obj_path,
                self.cli_log(binary),
                binary,
                " ".join(args)
        )


    def launch(self, binary, args, params=None, verbose=True):
        """
            Launch specified inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable)

            @param args     arguments to give to the binary

            @param params   dictionnary of (parameter, value) values to set parameters
                            for the analysis

            @param verbose  if True, print intermediate steps

        """
        if params is not None:
            pin_args = " ".join(["-{} {}".format(k, v) for k, v in params.items()])
        else:
            pin_args = ""
        cmd = self.__cmd(binary, pin_args, args)
        self.stdout(cmd, verbose)
        #stop = raw_input()
        start = datetime.now()
        subprocess.call(cmd, shell=True)
        duration = datetime.now() - start
        self.stdout("Execution time: {0}.{1}s".format(
            duration.seconds, duration.microseconds
            ), verbose)


    def compile(self, force, debug, trace, verbose):
        """
            Compile all pintools needed

        """
        #TODO os independant

        force_flag = ''
        if force:
            force_flag = '-B'

        src_path, src_name = os.path.split(self.__src_path)
        obj_path, obj_name = os.path.split(self.__obj_path)
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

        if "compile-flags" in self.__pinconf.keys():
            compile_flags = self.__pinconf["compile-flags"]
        else:
            compile_flags = ""
        cmd = "make {} PIN_ROOT='{}' SCAT_COMPILE_FLAGS='{}' OBJDIR='{}/' '{}/{}.so'".format(
                force_flag,
                self.__pinconf["path"],
                compile_flags,
                obj_build_path,
                obj_build_path,
                obj_build_name)

        self.stdout("Compiling pintool: {0} ...".format(src_name[:-4]), verbose=verbose)
        with open("/dev/null", 'w') as fnull:
            try:
                subprocess.check_call(cmd, cwd=src_path, shell=True, stdout=fnull)
                mtime_now = os.stat("{}/{}.so".format(obj_build_path, obj_build_name)).st_mtime
                shutil.copyfile(
                        '{}/{}.so'.format(obj_build_path, obj_build_name),
                        '{}/{}'.format(obj_path, obj_name))
                if mtime_before == mtime_now:
                    self.stdout("\t=> Up to date !", verbose=verbose)
                else:
                    self.stdout("\t=> Done !", verbose)
                return True
            except subprocess.CalledProcessError as error:
                self.stdout("/!\ Compilation exited with non-zero status {} /!\\\n\n".format(error.returncode), verbose=verbose)
                return False

    def get_analysis(self, pgm, data = None):
        logfile = self.__log_manager.get_log(str(self), pgm)
        if str(self) == 'arity':
            return ArityAnalysis(pgm, logfile, data)
        elif str(self) == 'type':
            return TypeAnalysis(pgm, logfile, data)
        elif str(self) == 'couple':
            return CoupleAnalysis(pgm, logfile)
        else:
            return Analysis(pgm, logfile)
