#-*- coding: utf-8 -*-

from cmd2 import Cmd
import os
import sys
import subprocess
from datetime import datetime
import glob

from src.shell.config import parse_config, ConfigFileError
from src.shell.pin import Pin, inf_code_to_str, INF_ARITY, INF_TYPE, INF_COUPLE, get_previous_step, inf_str_to_code
from src.shell.result import Result

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
        # Create a result objet
        self.res = Result(self.log_dir)
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

    
    def get_inputfile(self, inf_code, binary):
        """
            Retrieve the most recent logfile from the given
            step of inference.

            @param inf_code code corresponding to the inference step to 
                            look for

            @param binary   the binary file to analyse

            @ret            a path to the most recent logfile from step

            @raise IOError  if no file from step is found.

        """
        if inf_code not in [INF_TYPE, INF_ARITY]:
            return None
        inf_name = inf_code_to_str(inf_code)
        candidates = [self.log_dir + "/" + fn for fn in os.listdir(self.log_dir) if fn.startswith(os.path.basename(binary) + "_" + inf_name) and fn.endswith(".log")]
        if len(candidates) == 0:
            self.stderr("cannot file result from {0} inference - ensure that you did run every step in order (arity > type > couple) for this binary".format(inf_name))
            raise IOError
        return max(candidates, key=os.path.getmtime)


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


    def inference(self, code, s):
        self.do_checkconfig("")
        if not self.config_ok:
            return
        # parse command into binary + args
        args =  list()
        for i, arg in enumerate(s.split(" ")):
            if i == 0:
                binary = arg
            else:
                args.append(arg)
        try:
            self.__check_path(binary, isdir=False, isexec=True)
        except ValueError:
            return
        try:
            inputfile = self.get_inputfile(get_previous_step(code), binary)
        except IOError:
            return
        self.out("Launching {0} inference on {1}".format(inf_code_to_str(code), binary))
        self.__pin.infer(code, binary, args, self.log_dir + "/" + self.gen_logfile(code, binary), inputfile)


    def __complete_bin(self, text, line, begidx, endidx):
        paths = list()
        for path in glob.glob("/usr/bin/" + text + "*"):
            if os.path.isdir(path):
                continue
            paths.append(path.replace("/usr/bin/", ""))
        for path in glob.glob("/bin/" + text + "*"):
            if os.path.isdir(path):
                continue
            paths.append(path.replace("/bin/", ""))
        return paths



    def __complete_path(self, text, line, begidx, endidx):
        before_arg = line.rfind(" ", 0, begidx)
        if before_arg == -1:
            return 
        fixed = line[before_arg+1:begidx]
        arg = line[before_arg+1:endidx]
        pattern = arg + "*"
        paths = list()
        for path in glob.glob(pattern):
            if os.path.isdir(path) and path[-1] != os.sep:
                path += os.sep
            paths.append(path.replace(fixed, "", 1))
        return paths


    def complete_arity(self, text, line, begidx, endidx):
        if len(line.split(" ")) < 3:
            return self.__complete_bin(text, line, begidx, endidx)
        else:
            return  self.__complete_path(text, line, begidx, endidx)


    def complete_type(self, text, line, begidx, endidx):
        if len(line.split(" ")) < 3:
            return self.__complete_bin(text, line, begidx, endidx)
        else:
            return  self.__complete_path(text, line, begidx, endidx)


    def complete_couple(self, text, line, begidx, endidx):
        if len(line.split(" ")) < 3:
            return self.__complete_bin(text, line, begidx, endidx)
        else:
            return  self.__complete_path(text, line, begidx, endidx)
    

    def do_arity(self, s):
        """
            Launch arity inference on the binary specified as a parameter

        """
        self.inference(INF_ARITY, s)


    def do_type(self, s):
        """
            Launch type inference on the binary specified as a parameter

        """
        self.inference(INF_TYPE, s)


    def do_couple(self, s):
        """
            Launch couple inference on the binary specified as a parameter

        """
        self.inference(INF_COUPLE, s)


    def do_make(self, s):
        """
            Compile pintools

        """
        self.__pin.compile()


    def complete_display(self, text, line, begidx, endidx):
        pgm_inf  = self.res.get_pgm_list() 
        for p, inf in pgm_inf:
            if line.find(p) >= 0:
                return [i for i in inf if i.startswith(text)]
        return [pgm for pgm, inf in pgm_inf if pgm.startswith(text)]


    def do_display(self, s):
        """
            Display results of inference

        """
        args = s.split(" ")
        if len(args) == 0 or args[0] == '':
            for p, inf in self.res.get_pgm_list():
                print p
            return
        pgm = args[0]
        if len(args) == 1:
            for p, inf in self.res.get_pgm_list():
                if p != pgm: 
                    continue
                for i in inf:
                    print i
                return
        inf = args[1]
        inf_code = inf_str_to_code(inf)
        inputfile = self.get_inputfile(inf_code, pgm)
        self.res.compute(pgm, inf_code, inputfile)

