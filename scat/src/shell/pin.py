#-*- coding: utf-8 -*-

import subprocess

class Pin(object):
    """
        This class defines an interface between python and 
        our implementation using pin. 

    """

    def __init__(self, **kwargs):
        # Path to pin executable
        if "pinpath" in kwargs.keys():
            self.pinpath = kwargs["pinpath"]
        else:
            self.pinpath = ""
        # Path to pintool object files
        if "objdir" in kwargs.keys():
            self.objdir = kwargs["objdir"]
        else:
            self.objdir = ""
        # Dictionary for pintool names
        self.pintool = dict()
        # Name of pintool for arity, type and couple
        for pt in ["arity", "type", "couple"]:
            if pt in kwargs.keys():
                self.pintool[pt] = kwargs[pt]
            else:
                self.pintool[pt] = pt + ".so"

    def __cmd(self, pintool, binary):
        return "{0} -t {1} -- {2}".format(self.pinpath, self.objdir + "/" + pintool, binary)

    def arity(self, binary, logdir):
        """
            Launch arity inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable path
            @param logdir   path to the log directory where arity information
                            is stored (must be a valid path)

        """
        print self.__cmd(self.pintool["arity"], binary)
        # subprocess.call(self.__cmd("arity", binary))

    def type(self, binary, logdir):
        """
            Launch type inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable path
            @param logdir   path to the log directory where arity information
                            is stored (must be a valid path)

        """
        print self.__cmd(self.pintool["type"], binary)
        # subprocess.call(self.__cmd("type", binary))

    def couple(self, binary, logdir):
        """
            Launch couple inference on binary given in parameter

            @param binary   the binary file to analyse (must be a valid path to
                            an executable path
            @param logdir   path to the log directory where arity information
                            is stored (must be a valid path)

        """
        print self.__cmd(self.pintool["couple"], binary)
        # subprocess.call(self.__cmd("type", binary))

