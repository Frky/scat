#-*- coding: utf-8 -*-

import struct 

from src.shell.parser.i_log_parser import ILogParser

class Block(object):

    IN = 0
    OUT = 1

    NUM = 0
    ADDR = 1

    def __init__(self, buf, fmt, fn_table):
        # Lines from the log file are of the form:
        # io:type:value:file:offset:function_name:argument_position:timecounter
        value, fid, pos, counter, from_main = struct.unpack(fmt, buf)
        self.__type = Block.NUM if pos == 0 and value == 1 else Block.ADDR
        self.__val = value
        self.__date = counter
        self.__pos = pos
        self.__is_from_main = from_main
        self.__id = fn_table[fid]

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

    @property
    def img_name(self):
        return self.__id.split(":")[0]

    def is_from_main(self):
        return self.__is_from_main

    def is_addr(self):
        return self.__type == Block.ADDR

    def is_num(self):
        return not self.is_addr()

    def is_in(self):
        return self.__pos != 0

    def is_out(self):
        return not self.is_in()


class MemallocParser(ILogParser):

    # Size of the hash table
    DATA_SIZE = 100000

    def __init__(self, log_file, cli_ignore=None, cli_libmatch=None):
        super(MemallocParser, self).__init__(log_file)
        self.__ignore = cli_ignore
        self.__libmatch = cli_libmatch
        self.__time = None

    @property
    def time(self):
        return self.__time

    def get(self, ret_only=False, param_only=False, ALLOC=""):
        with open(self.log_path, 'r') as f:
            # Read time of execution
            self.__time = float(f.readline()[:-1])
            # Read Function Table
            self.read_header(f);
            field_size = [int(s) for s in f.readline()[:-1].split(":")]
            fmt = "<"
            idx = 0
            for size in field_size:
                if size == 4:
                    fmt += "L"
                elif size == 8:
                    fmt += "Q"
                elif size == 1:
                    fmt += "B"
                else:
                    raise NotImplemented
            while True:
                buf = f.read(sum(field_size))
                if not buf:
                    break
                block = Block(buf, fmt, self.fn_table)
                if self.__ignore is not None and any([i in block.id for i in self.__ignore]):
                    print "continue"
                    continue
                if self.__libmatch != None and not any([i in block.id.split(':')[0] for i in self.__libmatch]):
                    continue
                if ret_only and block.is_in():
                    continue
                if param_only and block.is_out() and block.id != ALLOC:
                    continue
                yield block

