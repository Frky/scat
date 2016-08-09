#-*- coding: utf-8 -*-

from src.shell.parser.i_log_parser import ILogParser

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
                io, typ, val, name, counter = line[:-1].split(":")
                yield io, typ, int(val), name, int(counter)
        # if verbose:
        #     self.log("")

