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
        if fname in self.data.keys():
            if ar == len(self.data[fname]) - 1:
                return 1
            else:
                if VERBOSE:
                    print("[{}] Arity mismatch".format(fname))
                    print("   Expected {}, got {}".format(self.data[fname][1:], ar))
                return 0
        else:
            return -1


    def check_one_ret(self, fname, ret):
        if (ret and self.data[fname][0] == "void") or (not ret and self.data[fname][0] != "void"):
            if VERBOSE:
                print("[{}] Return mismatch".format(fname))
                print("   Expected {}, got {}".format(self.data[fname][0], ret))
            return 0
        else:
            return 1


    def print_general_info(self):
        print("Information about inference")
        print("| Last inference:           {0}".format(self.date))
        print("| Total functions infered:  {0}".format(len(self.log.keys())))


    def accuracy(self):
        self.print_general_info()
        print()
        tot_ar = 0
        ok_ar = 0
        ok_ret = 0
        overflow = 0
        not_found = 0
        for addr, (fn, ar, ret) in self.log.items():
            if fn != "":
                res = self.check_one_arity(fn, ar)
                if res == -1:
                    not_found += 1
                    continue
                elif res == 0 and ar > 6:
                    overflow += 1
                if self.check_one_ret(fn, ret == 1) > 0:
                    ok_ret += 1
                else:
                    pass
                tot_ar += 1
                ok_ar += res
        print("Accuracy of inference")
        print("| Ok/Total tested:          {0}/{1}".format(ok_ar, tot_ar))
        if tot_ar != 0:
            print("| Ratio arity:              {0:.2f}%".format(float(ok_ar)*100./float(tot_ar)))
            print("| Ratio return:             {0:.2f}%".format(float(ok_ret)*100./float(tot_ar)))
        print("| Not found:                {0}".format(not_found))
        return 


    def display(self):
        for addr, fn in self.log.items():
           print(hex(addr), fn)
        print()
        self.print_general_info()
            

