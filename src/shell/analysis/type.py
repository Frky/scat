#-*- coding: utf-8 -*-

from datetime import datetime
import re
from analysis import Analysis

class TypeAnalysis(Analysis):

    def __init__(self, pgm, logfile, data=None):
        Analysis.__init__(self, pgm, logfile)

        self.data = data
        if data == None:
            self.protos = None
        else:
            self.protos = data.protos


    def parse_log(self):
        self.log = dict()
        with open(self.logfile, "r") as f:
            for line in f.readlines():
                img, imgaddr, fn, args = line[:-1].split(":")
                args = args.split(",")
                if args[0] == '':
                    args = None
                self.log[(img, int(imgaddr))] = (fn, args)


    def print_general_info(self):
        if self.data is None:
            Analysis.print_general_info(self)
        else:
            Analysis.print_general_info_with_data(self, self.data)


    def check_one(self, fname, args, proto, undef_as_int = False):
        ar = min(len(args), len(proto))
        ok, tot = 0, 0
        for ref, inf in zip(proto[:ar], args[:ar]):
            if ref == "...":
                break
            tot += 1
            if inf == "UNDEF":
                if undef_as_int:
                    inf = "INT"
                else:
                    continue
            if (inf[:4] == "ADDR") != ("*" in ref or "[" in ref):
                continue
            ok += 1
        return (ok, tot)


    def accuracy(self):
        self.print_general_info()
        print("")

        without_name = 0
        variadic = 0
        pseudo_functions = 0
        not_found = 0

        total = 0
        ok = 0

        for (img, imgaddr), (fn, args) in self.log.items():
            if fn == "":
                without_name += 1
                continue
            elif self.is_pseudo_function(fn):
                pseudo_functions += 1
                continue
            elif fn not in self.protos.keys():
                not_found += 1
                continue

            proto = self.protos[fn]
            if self.is_variadic(proto):
                variadic += 1
                continue

            res = self.check_one(fn, args, proto, undef_as_int = True)
            ok += res[0]
            total += res[1]

        if total == 0:
            ratio = float('nan')
        else:
            ratio = float(ok) * 100. / float(total)

        print("Ignored")
        print("| Without name:          {0}".format(without_name))
        print("| Variadic:              {0}".format(variadic))
        print("| Pseudo-Functions:      {0}".format(pseudo_functions))
        print("- Not in binary/source:  {0}".format(not_found))
        print("")

        print("Accuracy of inference")
        print("| Ok/Total tested:       {0}/{1}".format(ok, total))
        print("- Ratio:                 {0:.2f}%".format(ratio))


    def args_str(self, img, imgaddr, fn, args):
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
            line += '[{}@{}]'.format(img, hex(imgaddr))
        else:
            line += fn
        line += "("
        if len(args) == 1:
            line += "void"
        if not (args is None or len(args) == 1):
            for i, arg in enumerate(args[1:]):
                endidx = arg.find("(")
                if endidx == -1:
                    line += arg.lower()
                else:
                    line += arg[0:endidx].lower()
                if i != len(args) - 2:
                    line += ", "
        line += ");"
        return line


    def pp_data_type(self, type):
        if '*' in type or '[' in type:
            return 'addr '
        elif type == 'void':
            return 'void '
        elif type == 'float' or type == 'double':
            return 'float'
        else:
            return 'int  '


    def pp_inferred_type(self, type):
        idx = type.find("(")
        if idx == -1:
            return type.lower().ljust(5)
        else:
            return type[:idx].lower().ljust(5)


    def display(self):
        for (img, imgaddr), (fn, args) in self.log.items():
            print(self.args_str(img, imgaddr, fn, args))
        print("")
        self.print_general_info()


    def mismatch(self):
        self.print_general_info()
        print("")

        for (img, imgaddr), (fname, args) in self.log.items():
            if fname == "" or fname not in self.protos.keys():
                continue

            proto = self.protos[fname]
            if self.is_variadic(proto):
                continue

            res = self.check_one(fname, args, proto)

            if res[0] == res[1]:
                continue

            print("[{}@{}] {}".format(img, hex(imgaddr), fname))
            print("           {} -> {}".format(", ".join(proto[1:]), proto[0]))
            print("Expected:  {} -> {}".format(
                    ", ".join(map(self.pp_data_type, proto[1:])),
                    self.pp_data_type(proto[0])))
            print("Got:       {} -> {}".format(
                    ", ".join(map(self.pp_inferred_type, args[1:])),
                    self.pp_inferred_type(args[0])))
