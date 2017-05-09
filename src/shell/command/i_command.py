#-*- coding: utf-8 -*-

import sys
from abc import ABCMeta, abstractmethod

class ICommand(object):
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
        raise NotImplemented

    @abstractmethod
    def complete(self, text, line, begidx, endidx):
        return ""

