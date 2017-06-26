#-*- coding: utf-8 -*-

import sys
from abc import ABCMeta, abstractmethod

from src.shell.std import Std

class ICommand(Std):
    """
        Interface for a scat command

    """

    def __init__(self, verbose=2):
        self.__verbose = verbose
        return

    def stdout(self, msg, crlf=True):
        if self.__verbose > 1:
            sys.stdout.write("[*] " + msg)
            if crlf:
                sys.stdout.write("\n")

    def stderr(self, msg):
        """
            Print message on standard error, with formatting.

            @param msg  message to print

        """
        if self.__verbose > 0:
            sys.stderr.write("*** " + msg + "\n")

    @abstractmethod
    def run(self, *args, **kwargs):
        raise NotImplemented

    @abstractmethod
    def help(self, *args, **kwargs):
        print(self.__doc__.replace("\n"+8*" ","\n")[1:-5])

    @abstractmethod
    def complete(self, text, line, begidx, endidx):
        return
