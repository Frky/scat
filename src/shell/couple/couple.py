#-*- coding: utf-8 -*-

from src.shell.parser.couple import CoupleLogParser
from time import time

class Couple(object):
    """
        Retrieve couples

    """

    def __init__(self, log_file, pgm, verbose=True):
        super(Couple, self).__init__()
        self.__parser = CoupleLogParser(log_file)
        self.__pgm = pgm
        self.__verbose = verbose

    def stdout(self, m):
        if self.__verbose:
            print "[*] {}".format(m)

    def run(self, get=False, min_vals=50, min_rho=0.5, log=None):
        inp = dict()
        out = dict()
        SIZE_LIMIT = 10000
        NROUND = 65536
        self.stdout("parsing memory blocks...")
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
        for k, v in inp.items():
            if len(v) < min_vals:
                inp.pop(k)
        self.stdout("#f: {0} | #g: {1} | #in: {2} | #out: {3}".format(
                                len(out), 
                                len(inp), 
                                reduce(lambda p, q: p + len(q), inp.values(), 0), 
                                reduce(lambda p, q: p + reduce(lambda x, y: x + len(y), q, 0), out.values(), 0),
                            ),
                        )
        self.stdout("computing couples...")
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
                    if float(not_nb) / float(len(param_in)) > 1 - min_rho:
                        break
                rho = float(nb) / float(len(param_in))
                if rho >= min_rho:
                    param_pos = max(pos_couple.items(), key=lambda x: x[1])[0]
                    couples.append((f, g, rho, param_pos))
        with open("log/{}_coupleres_{}.log".format(self.__pgm, int(time())), "w") as f:
            for c in couples:
                self.stdout("{} -- ({:.2f}) --> {}[{}]".format(c[0], c[2], c[1], c[3]))
                f.write("{}:{}:{}:{}\n".format(*c))

        n_f = len(set(map(lambda a: a[0], couples)))
        n_g = len(set(map(lambda a: a[1], couples)))
        n = len(couples)
        tot = len(self.__parser.fn_table)

        if log is not None:
            params = self.__parser.get_params()
            with open(log, "a") as f:
                f.write("{}:{}:{}:{}:{}:{}:{}:{}\n".format(
                        self.__pgm,
                        int(min_vals), 
                        int(params["MAX_VALS"]),
                        min_rho,
                        tot, 
                        n_f,
                        n_g, 
                        n
                    ))

        if get:
            return tot, n_f, n_g, n

