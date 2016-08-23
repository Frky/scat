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
        NROUND = 65536
        self.log("parsing memory blocks...")
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
                    for i in xrange(NROUND):
                        out[block.id].append(list())
                out[block.id][block.val % NROUND].append(block.val)
        self.log("#f: {0} | #g: {1} | #in: {2} | #out: {3}".format(
                                len(inp), 
                                len(out), 
                                reduce(lambda p, q: p + len(q), inp.values(), 0), 
                                reduce(lambda p, q: p + reduce(lambda x, y: x + len(y), q, 0), out.values(), 0),
                            ),
                        )
        self.log("computing couples...")
        couples = list()
        for g, param_in in inp.items():
            for f, param_out in out.items():
                nb = 0
                not_nb = 0
                for param in param_in:
                    if param in param_out[param % NROUND]:
                        nb += 1
                    else:
                        not_nb += 1
                    if float(not_nb) / float(len(param_in)) > 1 - 0.5:
                        break
                rho = float(nb) / float(len(param_in))
                if rho > 0.5:
                    couples.append((f, g, rho))
        for c in couples:
            print c
        return

