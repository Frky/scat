#-*- coding: utf-8 -*-

import glob
import os
import sys
import subprocess
from cmd2 import Cmd
from confiture import Confiture, ConfigFileError
from datetime import datetime

from src.shell.command.memcomb import MemComb
from src.shell.command.couple import Couple
from src.shell.data.data import Data
from src.shell.pin.pintool import Pintool
from src.shell.result import Result
from src.shell.test import ScatTest

class ScatShell(Cmd):

    prompt = 'scat > '

    def __init__(self, config_path="config/config.yaml"):
        self.config_path = config_path
        conf = Confiture("config/templates/general.yaml")
        # Parse configuration file and get result
        self.config = conf.check_and_get(config_path)
        # Before we checked the config, it is considered KO
        self.config_ok = False
        # Set the log directory
        self.log_dir = self.config["log"]["path"]
        # Create a result object
        self.res = Result(self.log_dir)

        # Available pintools
        self.__pintools = dict()
        for pintool in self.config["pintool"]:
            # Check the needed two arguments
            req = ["src", "obj"]
            for r in req:
                if r not in self.config["pintool"][pintool].keys():
                    #TODO
                    raise Exception
            src = self.config["pintool"][pintool]["src"]
            obj = self.config["pintool"][pintool]["obj"]
            # Check potential extra argument
            if "prev_step" in self.config["pintool"][pintool].keys():
                prev_step = self.config["pintool"][pintool]["prev_step"]
            else:
                prev_step = None
            # Create pintool object
            pintool_obj = Pintool(
                                    name=pintool,
                                    src_path=src,
                                    obj_path=obj,
                                    pinconf=self.config["pin"],
                                    stdout=self.out,
                                    stderr=self.out,
                                    log_dir=self.log_dir,
                                    prev_step=prev_step,
                                )
            self.__pintools[pintool] = pintool_obj

        # Create a test object
        # Testing options
        kwargs = dict()
        kwargs["log"] = self.out
        if "test" in self.config.keys() and "proto" in self.config["test"]:
            kwargs["proto"] = self.config["test"]["proto"]
        self.test = ScatTest(**kwargs)

        # Init shell
        Cmd.__init__(self, completekey='tab')

    def emptyline(self):
        pass

    #========== LOG functions ==========#

    def out(self, msg, verbose=True, crlf=True):
        """
            Print message on standard input, with formatting.

            @param msg  message to print

        """
        if verbose:
            sys.stdout.write("[*] " + msg)
            if crlf:
                sys.stdout.write("\n")

    def stderr(self, msg):
        """
            Print message on standard error, with formatting.

            @param msg  message to print

        """
        sys.stderr.write("*** " + msg + "\n")

    #========== Check functions ==========#

    def __check_path(self, fpath, **kwargs):
        """
            Perform some verifications of a path (e.g. exists? is a directory?)

            @param fpath            path of the file/dir to check

            @param (opt) isdir      True => check if fpath is directory

            @param (opt) isexec     True => check if fpath is executable

            @raise ValueError if some error occur

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

    #========== Completion functions ==========#

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

    def __get_pgm_and_inf(self, s):
        args = s.split(" ")
        if len(args) == 0 or args[0] == '':
            for p, inf in self.res.get_pgm_list():
                print(p)
            raise ValueError
        pgm = args[0]
        if len(args) == 1:
            for p, inf in self.res.get_pgm_list():
                if p != pgm:
                    continue
                for i in inf:
                    print(i)
                raise ValueError
        # This line might raise KeyError if pintool is not found
        # This exception should be handled by the caller
        pintool = self.__pintools[args[1]]
        return pgm, pintool

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

    #========== checkconfig ==========#

    def help_checkconfig(self):
        print(self.do_checkconfig.__doc__.replace("\n", ""))

    def do_checkconfig(self, s):
        """
            Check the configuration file, and in particular:
                - the log directory (path, permissions)
                - pin (path, permissions)

        """
        #TODO check also pintool paths => call pin.check_config
        try:
            self.__check_path(self.log_dir, isdir=True)
            self.__check_path(self.config["pin"]["bin"], isexec=True)
        except ValueError:
            self.config_ok = False
            return
        self.config_ok = True

    #========== setlogdir ==========#

    def help_setlogdir(self):
        print(self.do_setlogdir.__doc__.replace("\n", ""))

    def do_setlogdir(self, directory):
        """
            Specify path to log directory. Target must exists.

        """
        try:
            self.__check_path(directory, isdir=False)
        except ValueError:
            return
        self.log_dir = directory

    #========== make ==========#

    def help_make(self):
        print(self.do_make.__doc__)

    def do_make(self, s):
        """
            (Re)compile pintools. With no argument, this command recompiles
            every pintool registered.

            You can also specify the pintools you want to compile (e.g. make arity type)

        """
        force = False
        debug = False
        trace = False

        if s != "":
            args = s.split(" ")
        else:
            args = list()

        to_compile = list()

        for arg in args:
            if arg == '-f' or arg == '--force' or arg == '-B':
                force = True
                continue
            if arg == '-d' or arg == '--debug':
                debug = True
                continue
            if arg == '-t' or arg == '--trace':
                trace = True
                continue

            try:
                p = self.__pintools[arg]
            except KeyError:
                self.stderr("pintool {0} is unknown".format(name))
                return
            to_compile.append(p)

        # If no pintool given in argument, compile everything
        if len(to_compile) == 0:
            to_compile = self.__pintools.values()

        # Compile pintools
        for p in to_compile:
            p.compile(force, debug, trace)

    #========== display ==========#

    def help_display(self):
        print(self.do_display.__doc__.replace("\n", ""))

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
        try:
            pgm, pintool = self.__get_pgm_and_inf(s)
        except ValueError:
            return
        except KeyError:
            #TODO explicit message (w/ pintool and binary details)
            self.stderr("Pintool error")

        print pintool
        self.res.compute(pgm, pintool)

    #========== parsedata ==========#

    def help_parsedata(self):
        print(self.do_parsedata.__doc__.replace("\n", ""))

    def do_parsedata(self, s):
        """
            Parse source code to test inference results

        """
        # TODO check number of args
        # TODO completion on args
        split = s.split(" ")
        if len(split) == 1:
            binary, srcdir = split[0], None
        else:
            binary, srcdir = split

        pgm = os.path.basename(binary)

        # Check CLANG configuration
        conf = Confiture("config/templates/clang.yaml")
        conf.check("config/config.yaml")
        # Create a parser object
        data = Data(self.config["clang"]["data-path"], pgm)
        data.parse(binary, self.config["clang"]["lib-path"], srcdir)
        data.dump()

    #========== testing ==========#

    def help_test(self):
        print(self.do_test.__doc__.replace("\n", ""))

    def complete_test(self, text, line, begidx, endidx):
        # TODO
        pass

    def do_test(self, s):
        # TODO documentation
        # TODO check that config is specified in config file (+template)
        self.test.proto([self.__pintools["arity"], self.__pintools["type"]])

    #========== accuracy ==========#

    def help_accuracy(self):
        print(self.do_accuracy.__doc__.replace("\n", ""))

    def complete_accuracy(self, text, line, begidx, endidx):
        return self.complete_display(text, line, begidx, endidx)

    def do_accuracy(self, s):
        """
            Analyse the results of inference for a given program,
            by comparison with binary and source code.

        """
        try:
            pgm, pintool = self.__get_pgm_and_inf(s)
        except ValueError:
            return
        except KeyError:
            #TODO explicit message (w/ pintool and binary details)
            self.stderr("Pintool error")

        # Check CLANG configuration
        conf = Confiture("config/templates/clang.yaml")
        conf.check("config/config.yaml")
        try:
            data = Data(self.config["clang"]["data-path"], pgm)
            data.load()
        except IOError:
            data = None
        if data is None:
            self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
            return
        self.res.accuracy(pgm, pintool, data)

    #========== mismatch ==========#

    def help_mismatch(self):
        print(self.do_accuracy.__doc__.replace("\n", ""))

    def complete_mismatch(self, text, line, begidx, endidx):
        return self.complete_display(text, line, begidx, endidx)

    def do_mismatch(self, s):
        """
            Displays all mismatch for a given program,
            by comparison with binary and source code.

        """
        try:
            pgm, pintool = self.__get_pgm_and_inf(s)
        except ValueError:
            return
        except KeyError:
            #TODO explicit message (w/ pintool and binary details)
            self.stderr("Pintool error")

        # Check CLANG configuration
        conf = Confiture("config/templates/clang.yaml")
        conf.check("config/config.yaml")
        try:
            data = Data(self.config["clang"]["data-path"], pgm)
            data.load()
        except IOError:
            data = None
        if data is None:
            self.stderr("error: you must parse source code of \"{0}\" first (use parsedata)".format(pgm))
            return

        self.res.mismatch(pgm, pintool, data)

    #========== new pintool ==========#

    def complete_launch(self, text, line, begidx, endidx):
        if len(line.split(" ")) < 3:
            return filter(
                            lambda x: x.startswith(text),
                            map(lambda x: str(x), self.__pintools),
                        )
        elif len(line.split(" ")) < 4:
            return self.__complete_bin(text, line, begidx, endidx)
        else:
            return  self.__complete_path(text, line, begidx, endidx)

    def do_launch(self, s):
        split = s.split(" ")
        index = 0

        debug = False
        trace = False
        while index < len(split) and split[index].startswith("-"):
            arg = split[index]
            if arg == '-d' or arg == '--debug':
                debug = True
            elif arg == '-t' or arg == '--trace':
                trace = True
            index += 1

        inf = split[index]
        index += 1

        if inf in self.__pintools.keys():
            p = self.__pintools[inf]
            p.compile(False, debug, trace)
            # Before inference, check that the configuration is correct
            self.do_checkconfig("")
            if not self.config_ok:
                return
            # Parse command into binary + args
            args =  list()
            for i, arg in enumerate(split[index:]):
                if arg[0] == "\"":
                    arg = arg[1:]
                if arg[-1] == "\"":
                    arg = arg[:-1]
                if i == 0:
                    binary = arg
                else:
                    args.append(arg)
            # Check the binary (exists? is executable?)
            try:
                self.__check_path(binary, isdir=False, isexec=True)
            except ValueError:
                return
            # Run inference
            self.out("Launching {0} inference on {1}".format(p, binary))
            p.launch(binary, args)

    #========== ALLOC RETRIEVING ==========

    def do_memcomb(self, s):
        # Get log file from last block inference
        if "memblock" not in self.__pintools.keys():
            self.stderr("you must run memblock inference first")
            return
        logfile = self.__pintools["memblock"].get_logfile(s, prev=False)
        MemComb(logfile, self.out).run()

    #========== COUPLE FROM MEMBLOCK ==========

    def do_couple(self, s):
        # Get log file from last block inference
        if "memblock" not in self.__pintools.keys():
            self.stderr("you must run memblock inference first")
            return
        logfile = self.__pintools["memblock"].get_logfile(s, prev=False)
        Couple(logfile, self.out).run()

