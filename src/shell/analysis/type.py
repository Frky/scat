#-*- coding: utf-8 -*-

from datetime import datetime
import re


class TypeAnalysis(object):

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
                addr, fn, args = line[:-1].split(":")
                args = args.split(",")
                if args[0] == '':
                    args = None
                self.log[int(addr)] = (fn, args)


    def print_general_info(self):
        print("Information about inference")
        print("| Last inference:           {0}".format(self.date))
        print("| Total functions infered:  {0}".format(len(self.log.keys())))


    def check_one(self, fname, args):
        if fname not in self.data.keys():
            return (0, 0)
        ar = min(len(args), len(self.data[fname]))
        ok, tot = 0, 0
        print("# " + fname)
        for ref, inf in zip(self.data[fname][:ar], args[:ar]):
            print("  " + ref + " / " + inf)
            if ref == "...":
                break
            tot += 1
            if (inf[:4] == "ADDR" and "*" not in ref and "[" not in ref) or (inf[:4] != "ADDR" and ("*" in ref or "[" in ref)):
                continue
            ok += 1
        return (ok, tot)


    def accuracy(self):
        self.print_general_info()
        print()

        tot_typ = 0
        ok_typ = 0
        for addr, (fn, args) in self.log.items():
            if fn != "":
                res = self.check_one(fn, args)
                ok_typ += res[0]
                tot_typ += res[1]
        print("Accuracy of inference")
        print("| Ok/Total tested:          {0}/{1}".format(ok_typ, tot_typ))
        if tot_typ != 0:
            print("- Ratio:                    {0:.2f}%".format(float(ok_typ)*100./float(tot_typ)))


    def display(self):
        for addr, (fn, args) in self.log.items():
            line = ""
            if len(args) == 0:
                line += "void "
            else:
                endidx = args[0].find("(")
                if endidx == -1:
                    line += args[0].lower()
                else:
                    line += args[0][0:endidx].lower()
            line += " "
            if fn == "":
                line += hex(addr)
            else:
                line += fn
            line += "("
            if len(args) == 1:
                line += "void"
            if args is None or len(args) == 1:
                line += ");"
                continue
            for i, arg in enumerate(args[1:]):
                endidx = arg.find("(")
                if endidx == -1:
                    line += arg.lower()
                else:
                    line += arg[0:endidx].lower()
                if i != len(args) - 2:
                    line += ", "
            line += ");"
            print(line)
        print()
        self.print_general_info()
