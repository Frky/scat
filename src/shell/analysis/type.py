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
                img, imgaddr, fn, args = line[:-1].split(":")
                args = args.split(",")
                if args[0] == '':
                    args = None
                self.log[(img, int(imgaddr))] = (fn, args)


    def is_variadic(self, proto):
        return proto[-1] == "..."


    def print_general_info(self):
        print("Information about inference")
        print("| Last inference:           {0}".format(self.date))
        print("| Total functions infered:  {0}".format(len(self.log.keys())))


    def check_one(self, fname, args, proto):
        ar = min(len(args), len(proto))
        ok, tot = 0, 0
        for ref, inf in zip(proto[:ar], args[:ar]):
            if ref == "...":
                break
            tot += 1
            if ((inf[:4] == "ADDR") != ("*" in ref or "[" in ref)):
                continue
            ok += 1
        return (ok, tot)


    def accuracy(self):
        self.print_general_info()
        print("")

        without_name = 0
        variadic = 0
        not_found = 0

        total = 0
        ok = 0

        for (img, imgaddr), (fn, args) in self.log.items():
            if fn == "":
                without_name += 1
                continue
            elif fn not in self.data.keys():
                not_found += 1
                continue

            proto = self.data[fn]
            if self.is_variadic(proto):
                variadic += 1
                continue

            res = self.check_one(fn, args, proto)
            ok += res[0]
            total += res[1]

        print("Ignored")
        print("| Without name:             {0}".format(without_name))
        print("| Variadic:                 {0}".format(variadic))
        print("- Not found (in source):    {0}".format(not_found))
        print("")

        print("Accuracy of inference")
        print("| Ok/Total tested:          {0}/{1}".format(ok, total))
        if total != 0:
            ratio = float(ok) * 100. / float(total)
            print("- Ratio:                    {0:.2f}%".format(ratio))


    def display(self):
        for (img, imgaddr), (fn, args) in self.log.items():
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
                line += img
                line += " "
                line += hex(imgaddr)
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
