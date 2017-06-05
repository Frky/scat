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
        self.fn_table = list()
        self._params = dict()
        with open(self.log_path, "r") as log:
            self.elapsed_time = log.readline()[:-1]

    def time(self):
        return self.elapsed_time

    @abstractmethod
    def get(self):
        raise NotImplemented

    def read_header(self, f):
        nb_fid = int(f.readline()[:-1])
        # First entry (fid = 0) corresponds to nothing
        self.fn_table.append(None)
        fid = 1
        while fid <= nb_fid:
            self.fn_table.append(f.readline()[:-1])
            fid += 1

    def count_lines(self):
        with open(self.log_path, 'rb') as f:
            lines = 0
            buf_size = 1024 * 1024
            read_f = f.read
            buf = read_f(buf_size)
            while buf:
                lines += buf.count(b'\n')
                buf = read_f(buf_size)
        return lines

    def get_params(self):
        return self._params

