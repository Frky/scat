#-*- coding: utf-8 -*-

from src.shell.command.i_command import ICommand
from src.shell.parser.couple import CoupleLogParser
from time import time

class Couple(ICommand):
    """
        Retrieve couples

    """

    def __init__(self, log_file, pgm, log):
        super(Couple, self).__init__()
        self.__parser = CoupleLogParser(log_file)
        self.log = log
        self.__pgm = pgm

    def run(self):
        inp = dict()
        out = dict()
        SIZE_LIMIT = 10000
        NROUND = 65536
        self.log("parsing memory blocks...")
        for block in self.__parser.get():
            if block.is_in():
                if block.id not in inp.keys():
                    inp[block.id] = list()
                if len(inp[block.id]) < SIZE_LIMIT:
                    inp[block.id].append(block)
            elif block.is_out():
                if block.id not in out.keys():
                    out[block.id] = list()
                    for i in xrange(NROUND):
                        out[block.id].append(list())
                out[block.id][block.val % NROUND].append(block)
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
                out_idx = 0
                pos_couple = dict()
                for param in param_in:
                    not_nb += 1
                    for out_p in param_out[param.val % NROUND]:
                        if out_p.val == param.val and out_p.date < param.date:
                            try:
                                pos_couple[param.pos] += 1
                            except KeyError:
                                pos_couple[param.pos] = 1
                            nb += 1
                            not_nb -= 1
                            break
                    if float(not_nb) / float(len(param_in)) > 1 - 0.5:
                        break
                rho = float(nb) / float(len(param_in))
                if rho > 0.5:
                    param_pos = max(pos_couple.items(), key=lambda x: x[1])[0]
                    couples.append((f, g, rho, param_pos))
        with open("log/{}_coupleres_{}.log".format(self.__pgm, int(time())), "w") as f:
            for c in couples:
                print "{} -- ({:.2f}) --> {} (param position : {})".format(c[0], c[2], c[1], c[3])
                f.write("{}:{}:{}:{}\n".format(*c))
        return

