#-*- coding: utf-8 -*-

from cmd2 import Cmd

import os
import subprocess

class ScatShell(Cmd):

    prompt = 'scat > '

    def __init__(self):
        self.log_dir = "./log/"
        Cmd.__init__(self, completekey='tab')
        self.do_help("")

    def emptyline(self):
        pass

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
            print "*** You must specify a path"
            raise ValueError
        # If executable, check if we can execute (exists + permission X)
        if isexec and subprocess.call("type " + fpath, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) != 0:
            print "*** Specified target ({0}) is not executable - check permissions".format(fpath)
            raise ValueError
        else:
            return
        # Check path existance
        if not os.path.exists(fpath):
            print "*** Specified target ({0}) does not exist".format(fpath)
            raise ValueError
        # If dir is mandatory, perform additional check
        if isdir and not os.path.isdir(fpath):
            print "*** Specified target ({0}) is not a directory".format(fpath)
            raise ValueError

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

    def do_arity(self, binary):
        """
            Launch arity inference on the binary specified as a parameter

        """
        try:
            self.__check_path(binary, isdir=False, isexec=True)
        except ValueError:
            return
        print "Launching arity inference on {0}".format(binary)

