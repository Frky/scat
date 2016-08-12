#-*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.block_trace import BlockTraceParser, BlockTrace

class Couple(ICommand):
    """
        Retrieve couples

    """

    def __init__(self, log_file, log):
        super(Couple, self).__init__()
        self.__parser = BlockTraceParser(log_file)
        self.log = log

    def run(self):
        inp = dict()
        out = dict()
        SIZE_LIMIT = 10000
        for block in self.__parser.get():
            if block.type != BlockTrace.ADDR:
                continue
            if block.io == BlockTrace.IN:
                if block.id not in inp.keys():
                    inp[block.id] = list()
                if len(inp[block.id]) < SIZE_LIMIT:
                    inp[block.id].append(block.val)
            elif block.io == BlockTrace.OUT:
                if block.id not in out.keys():
                    out[block.id] = list()
                out[block.id].append(block.val)
        print len(inp), len(out)
        for g, param_in in out.items():
            for f, param_out in inp.items():
                nb = 0
                for param in param_in:
                    if param in param_out:
                        nb += 1
                if float(nb) / float(len(param_in)) > 0.5:
                    print f, g
        return

