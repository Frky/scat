#-*- coding: utf-8 -*-

from datetime import datetime
import re

class Analysis(object):

    def __init__(self, pgm, logfile):
        self.pgm = pgm
        self.logfile = logfile
        date_str = re.search("[0123456789]+", logfile.split('_')[-1]).group()
        self.date = datetime.fromtimestamp(int(date_str))
        self.log = None
        self.parse_log()


    def is_variadic(self, proto):
        return proto[-1] == "..."

    def is_pseudo_function(self, fname):
        return ('.part.' in fname
                or '.isra.' in fname
                or '.constprop.' in fname
                or '.plt' in fname)


    def ratio(self, ok, total):
        if total == 0:
            return float('nan')
        else:
            return float(ok) * 100. / float(total)


    def compute_nb_inf_pgm(self):
        nb = 0
        for (img, imgaddr), fn_log in self.log.items():
            fname = fn_log[0]
            if self.pgm in img and len(fname) > 0:
                nb += 1
        return nb


    def print_general_info(self):
        nb_inf = len(self.log.keys())
        nb_inf_pgm = self.compute_nb_inf_pgm()

        print("Inference")
        print("| Date:                        {}".format(self.date))
        print("| Total functions inferred:    {}".format(nb_inf))
        print("- Program functions inferred:  {}".format(nb_inf_pgm))


    def print_general_info_with_data(self, data):
        nb_inf = len(self.log.keys())
        nb_inf_pgm = self.compute_nb_inf_pgm()
        nb_src = len(data.protos.keys())
        nb_src_pgm = len(data.protos_without_libs.keys())
        if nb_src == 0:
            coverage = float('nan')
        else:
            coverage = float(nb_inf) * 100 / float(nb_src)
        if nb_src_pgm == 0:
            coverage_pgm = float('nan')
        else:
            coverage_pgm = float(nb_inf_pgm) * 100 / float(nb_src_pgm)

        print("Inference")
        print("| Date:                 {}".format(self.date))

        print("| Totals :")
        print("  | Functions inferred:   {}".format(nb_inf))
        print("  | Functions in source:  {}".format(nb_src))
        print("  - Coverage :            {:.2f}%".format(coverage))

        print("| Program (Excluding libraries) :")
        print("  | Functions inferred:   {}".format(nb_inf_pgm))
        print("  | Functions in source:  {}".format(nb_src_pgm))
        print("  - Coverage :            {:.2f}%".format(coverage_pgm))

    def display(self):
        print("Not implemented")


    def accuracy(self):
        print("Not implemented")


    def mismatch(self):
        print("Not implemented")
