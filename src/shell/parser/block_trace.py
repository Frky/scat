#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

class BlockTrace(object):

    IN = 0
    OUT = 1

    NUM = 0
    ADDR = 1

    def __init__(self, line):
        # io, typ, val, img, img_addr, name, counter = line[:-1].split(":")
        # yield io, typ, int(val), img, img_addr, name, int(counter)
        io, typ, value, img, img_addr, name, counter = line[:-1].split(":")
        if io == "in":
            self.__io = BlockTrace.IN
        else:
            self.__io = BlockTrace.OUT
        if typ == "addr":
            self.__type = BlockTrace.ADDR
        else:
            self.__type = BlockTrace.NUM
        self.__val = int(value)
        self.__date = int(counter)
        if name != "":
            self.__id = name
        else:
            self.__id = ":".join([img, img_addr])

    @property
    def io(self):
        return self.__io

    @property
    def type(self):
        return self.__type

    @property
    def val(self):
        return self.__val

    @property
    def date(self):
        return self.__date

    @property
    def id(self):
        return self.__id


class BlockTraceParser(ILogParser):

    # Size of the hash table
    DATA_SIZE = 10000000

    def __init__(self, log_file):
        super(BlockTraceParser, self).__init__(log_file)

    def __count_line(self):
        with open(self.log_path, 'rb') as f:
            lines = 0
            buf_size = 1024 * 1024
            read_f = f.read
            buf = read_f(buf_size)
            while buf:
                lines += buf.count(b'\n')
                buf = read_f(buf_size)
        return lines

    # def log_date(self):
    #     self.log("timestamp: {0}".format(datetime.now().strftime("%s.%f")))

    def get(self):
        LINES = self.__count_line()
        # PWIDTH = 78
        # progress = 0.0
        with open(self.log_path, 'r') as f:
            for i, line in enumerate(f.readlines()):
                # pg = round(((i * 100.0)/LINES)*10)/10
                # if pg != progress:
                #     progress = pg
                #     if verbose:
                #         # self.log("parsing data from log file ({2} lines): [{0}{1}]".format("#" * (int(progress*PWIDTH/100.0)), " " * (PWIDTH - int(progress*PWIDTH/100) - 1), LINES), False)
                yield BlockTrace(line)
        # if verbose:
        #     self.log("")

