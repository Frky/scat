#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

class Block(object):

    IN = 0
    OUT = 1

    NUM = 0
    ADDR = 1

    def __init__(self, line):
        # Lines from the log file are of the form:
        # io:type:value:file:offset:function_name:argument_position:timecounter
        io, typ, value, img, img_addr, name, pos, counter = line[:-1].split(":")
        if io == "in":
            self.__io = Block.IN
        else:
            self.__io = Block.OUT
        if typ == "addr":
            self.__type = Block.ADDR
        else:
            self.__type = Block.NUM
        self.__val = int(value)
        self.__date = int(counter)
        self.__pos = int(pos)
        # if name != "":
        #    self.__id = name
        # else:
        self.__id = ":".join([img, img_addr, name])

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

    def is_addr(self):
        return self.__type == Block.ADDR

    def is_num(self):
        return not self.is_addr()

    def is_in(self):
        return self.__io == Block.IN

    def is_out(self):
        return not self.is_in()


class MemblockParser(ILogParser):

    # Size of the hash table
    DATA_SIZE = 10000000

    def __init__(self, log_file):
        super(MemblockParser, self).__init__(log_file)

    def get(self):
        with open(self.log_path, 'r') as f:
            # Skip first line
            f.readline()
            for i, line in enumerate(f.readlines()):
                yield Block(line)

