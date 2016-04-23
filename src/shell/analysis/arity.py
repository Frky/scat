#-*- coding: utf-8 -*-

from datetime import datetime
import re

VERBOSE = True

class ArityAnalysis(object):

    def __init__(self, pgm, logfile, data=None):
        self.pgm = pgm
        self.logfile = logfile
        self.date = datetime.fromtimestamp(int(re.search("[0123456789]+", logfile).group()))
        self.data = data
        self.log = None
        self.parse_log()


    def parse_log(self):
        self.log = dict()
        with open(self.logfile, "r") as f:
            for line in f.readlines():
                addr, fn, ar, iar, ret, misc = line[:-1].split(":")
                self.log[int(addr)] = (fn, int(ar, 16), int(ret, 16))


    def check_one_arity(self, fname, ar):
        if ar == len(self.data[fname]) - 1:
            return True
        else:
            if VERBOSE:
                print("[Arity mismatch {}] Expected {}, got {}".format(fname, self.data[fname][1:], ar))
            return False


    def check_one_ret(self, fname, ret):
        if (ret == (self.data[fname][0] != "void")):
            return True
        else:
            if VERBOSE:
                print("[Return mismatch {}] Expected {}, got {}".format(fname, self.data[fname][0], ret))
            return False


    def print_general_info(self):
        print("Information about inference")
        print("| Last inference:           {0}".format(self.date))
        print("| Total functions infered:  {0}".format(len(self.log.keys())))


    def accuracy(self):
        self.print_general_info()
        print()

        total = 0
        ok_ar = 0
        ok_ret = 0
        overflow = 0
        not_found = 0
        for addr, (fn, ar, ret) in self.log.items():
            if fn == "":
                continue
            elif fn not in self.data.keys():
                not_found += 1
                continue

            total += 1
            if self.check_one_arity(fn, ar):
                ok_ar += 1
            elif ar > 6:
                overflow += 1

            if self.check_one_ret(fn, ret == 1):
                ok_ret += 1

        print("Accuracy of inference")
        print("| Arity  Ok/Total tested:   {0}/{1}".format(ok_ar, total))
        print("| Return Ok/Total tested:   {0}/{1}".format(ok_ret, total))
        if total != 0:
            print("| Ratio arity:              {0:.2f}%".format(float(ok_ar)*100./float(total)))
            print("| Ratio return:             {0:.2f}%".format(float(ok_ret)*100./float(total)))
        print("| Not found:                {0}".format(not_found))
        return


    def display(self):
        for addr, fn in self.log.items():
           print(hex(addr), fn)
        print()
        self.print_general_info()


