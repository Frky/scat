#-*- coding: utf-8 -*-

from abc import ABCMeta, abstractmethod

class ICommand(object):
    """
        Interface for a scat command

    """

    def __init__(self, log=None):
        self.__log = log
        return

    def log(self, msg, crlf=True):
        if self.__log is not None:
            self.log(msg, crlf=crlf)

    @abstractmethod
    def run(self, *args, **kwargs):
        raise NotImplemented


    @abstractmethod
    def help(self, *args, **kwargs):
        raise NotImplemented

