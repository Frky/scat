#-*- coding: utf-8 -*-

from cmd2 import Cmd
import os
import sys
import subprocess
from datetime import datetime

from src.shell.config import parse_config, ConfigFileError
from src.shell.pin import Pin, inf_code_to_str, INF_ARITY, INF_TYPE, INF_COUPLE

class ScatShell(Cmd):


    prompt = 'scat > '


    def __init__(self, config_path="config.yaml"):
        try:
            # Parse configuration file and get result
            self.config = parse_config(config_path)
        except ConfigFileError:
            exit()
        # Before we checked the config, it is considered KO
        self.config_ok = False
        # Set the log directory
        self.log_dir = self.config["log"]["path"]
        # Create a pin object with pin executable path
        self.__pin = Pin(
                            pinpath=self.config["pin"]["path"], 
                            arity_src=self.config["pin"]["pintool-src"]["arity"],
                            type_src=self.config["pin"]["pintool-src"]["type"],
                            couple_src=self.config["pin"]["pintool-src"]["couple"],
                            arity_obj=self.config["pin"]["pintool-obj"]["arity"],
                            type_obj=self.config["pin"]["pintool-obj"]["type"],
                            couple_obj=self.config["pin"]["pintool-obj"]["couple"],
                            log=self.out
                        )
        # Init shell 
        Cmd.__init__(self, completekey='tab')


    def emptyline(self):
        pass


    def out(self, msg):
        sys.stdout.write("[*] " + msg + "\n")


    def stderr(self, msg):
        sys.stderr.write("*** " + msg + "\n")


    def gen_logfile(self, inf_code, binary):
        inf_name = inf_code_to_str(inf_code)
        timestamp = datetime.now().strftime("%s")
        return "{0}_{1}_{2}.log".format(os.path.basename(binary), inf_name, timestamp)


    def __check_path(self, fpath, **kwargs):
        """
            Perform some verifications of a path (e.g. exists ? is a directory ?)
            Raise ValueError if some error occur

        """
        # By default, file is not required to be a directory
        isdir = False
        if "isdir" in kwargs.keys():
            isdir = kwargs["isdir"]
        # By default, file is not required to be executable
        isexec = False
        if "isexec" in kwargs.keys():
            isexec = kwargs["isexec"]
        # Check if path is not empty
        if fpath == "":
            self.stderr("You must specify a path")
            raise ValueError
        # If executable, check if we can execute (exists + permission X)
        if isexec and subprocess.call("type " + fpath, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) != 0:
            self.stderr("Specified target ({0}) is not found or not executable - check permissions".format(fpath))
            raise ValueError
        else:
            return
        # Check path existance
        if not os.path.exists(fpath):
            self.stderr("Specified target ({0}) does not exist".format(fpath))
            raise ValueError
        # If dir is mandatory, perform additional check
        if isdir and not os.path.isdir(fpath):
            self.stderr("Specified target ({0}) is not a directory".format(fpath))
            raise ValueError


    def do_checkconfig(self, s):
        """
            Check the configuration file, and in particular:
                - the log directory (path, permissions)
                - pin (path, permissions)

        """
        #TODO check also pintool paths => call pin.check_config
        try:
            self.__check_path(self.log_dir, isdir=True)
            self.__check_path(self.__pin.pinpath, isexec=True)
        except ValueError:
            self.config_ok = False
            return
        self.config_ok = True


    def help_setlogdir(self):
        print self.do_checkconfig.__doc__.replace("\n", "")


    def do_setlogdir(self, directory):
        """
            Specify path to log directory. Target must exists.

        """
        try:
            self.__check_path(directory, isdir=False)
        except ValueError:
            return
        self.log_dir = directory


    def help_setlogdir(self):
        print self.do_setlogdir.__doc__.replace("\n", "")


    def inference(self, code, binary):
        self.do_checkconfig("")
        if not self.config_ok:
            return
        try:
            self.__check_path(binary, isdir=False, isexec=True)
        except ValueError:
            return
        self.out("Launching {0} inference on {1}".format(inf_code_to_str(code), binary))
        self.__pin.infer(code, binary, self.log_dir + "/" + self.gen_logfile(code, binary))


    def do_arity(self, binary):
        """
            Launch arity inference on the binary specified as a parameter

        """
        self.inference(INF_ARITY, binary)


    def do_type(self, binary):
        """
            Launch type inference on the binary specified as a parameter

        """
        self.inference(INF_TYPE, binary)


    def do_couple(self, binary):
        """
            Launch couple inference on the binary specified as a parameter

        """
        self.inference(INF_COUPLE, binary)


    def do_make(self, s):
        """
            Compile pintools

        """
        self.__pin.compile()
