#-*- coding: utf-8 -*-

from abc import ABCMeta, abstractmethod 

class ILogParser(object):
    """
        Parser for log files

        This is an abstract class that will be concretised by
        parsers for each type of log file (arity log, type log, etc.)

    """

    __metaclass__ = ABCMeta

    def __init__(self, log_path):
        # Path to log file
        self.log_path = log_path

    @abstractmethod
    def get(self):
        raise NotImplemented

