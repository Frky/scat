#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

class Block(object):

    IN = 0
    OUT = 1

    def __init__(self, line, fn_table):
        fid, pos, val, timecounter = line[:-1].split(":");
        self.__val = int(val)
        self.__date = int(timecounter)
        self.__pos = int(pos)
        self.__id = fn_table[int(fid)]

    @property
    def val(self):
        return self.__val

    @property
    def date(self):
        return self.__date

    @property
    def id(self):
        return self.__id

    @property
    def pos(self):
        return self.__pos

    def is_in(self):
        return self.__pos > 0

    def is_out(self):
        return not self.is_in()


class CoupleLogParser(ILogParser):

    # Size of the hash table
    DATA_SIZE = 10000000

    def __init__(self, log_file):
        super(CoupleLogParser, self).__init__(log_file)

    def get(self):
        with open(self.log_path, 'r') as f:
            # Read parameters
            line = f.readline()
            for p in line[:-1].split(":"):
                k, v = p.split("=")
                self._params[k] = v
            # Read Function Table
            self.read_header(f);
            line = f.readline()
            while line != "":
                yield Block(line, self.fn_table)
                line = f.readline()

