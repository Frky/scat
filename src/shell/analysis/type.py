#-*- coding: utf-8 -*-

from datetime import datetime
import re
from analysis import Analysis
from src.shell.parser.type import TypeLogParser

class TypeAnalysis(Analysis):

    def __init__(self, pgm, logfile, data=None):
        Analysis.__init__(self, pgm, logfile)

        self.data = data
        if data == None:
            self.protos = None
        else:
            self.protos = data.protos

    def parse_log(self):
        self.log = TypeLogParser(self.logfile)

    def print_general_info(self):
        if self.data is None:
            Analysis.print_general_info(self)
        else:
            Analysis.print_general_info_with_data(self, self.data)

    def check_function(self, fname, args, proto, undef_as_int = False):
        ret_fp, ret_fn = 0, 0
        param_fp, param_fn = 0, 0
        ret_tot, param_tot = 1, 0

        real_ret = self.get_one(proto[0])

        # Eliminate problems due to arity detection
        # on retval
        if (real_ret == "VOID" and args[0] != "VOID") or \
                (real_ret != "VOID" and args[0] == "VOID"):
            return (0,0,0,0,0,0)
        if self.check_one(proto[0], args[0], undef_as_int):
            return_ok = 1
        elif real_ret == 'ADDR':
            ret_fn += 1
        else:
            ret_fp += 1

        ar = min(len(args), len(proto))
        for ref, inf in zip(proto[1:ar], args[1:ar]):
            if ref == "...":
                break

            param_tot += 1
            real_param = self.get_one(ref)
            if self.check_one(ref, inf, undef_as_int):
                pass
            elif real_param == 'ADDR':
                param_fn += 1
            else:
                param_fp += 1

        return (param_fp, param_fn, param_tot, ret_fp, ret_fn, ret_tot)

    def get_one(self, ref):
        if '*' in ref or '[' in ref:
            return 'ADDR'
        elif ref == 'float' or ref == 'double':
            return 'FLOAT'
        elif ref == 'void':
            return 'VOID'
        else:
            return 'INT'

    def check_one(self, ref, inf, undef_as_int):
        if inf == 'UNDEF':
            if undef_as_int:
                inf = 'INT'
            else:
                return False

        if '*' in ref or '[' in ref:
            return inf.startswith('ADDR')
        elif ref == 'float' or ref == 'double':
            return inf.startswith('FLOAT')
        elif ref == 'void':
            return inf.startswith('VOID')
        else:
            return inf.startswith('INT')

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
        for function, args in self.log.get():
            img, imgaddr, fn = function.split(":")
            imgaddr = int(imgaddr)
            print(self.args_str(img, imgaddr, fn, args))
        print("")
        self.print_general_info()

    def accuracy(self, get=False, verbose=True, log=None, empty_time=0.0, nopin_time=0.0):
        if verbose:
            self.print_general_info()
            print("")

        without_name = 0
        variadic = 0
        pseudo_functions = 0
        not_found = 0

        return_ok = 0
        return_total = 0
        return_fp = 0
        return_fn = 0
        params_ok = 0
        params_total = 0
        param_fp = 0
        param_fn = 0

        for function, args in self.log.get():
            fn = function.split(":")[-1]
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
            
            pfp, pfn, ptot, rfp, rfn, rtot = self.check_function(fn, args, proto, undef_as_int = True)
            params_ok += (ptot - pfn - pfp)
            params_total += ptot
            return_ok += (rtot - rfn - rfp)
            return_total += rtot
            return_fp += rfp
            return_fn += rfn
            param_fp += pfp
            param_fn += pfn

        if verbose:
            print("Ignored")
            print("| Without name:          {0}".format(without_name))
            print("| Variadic:              {0}".format(variadic))
            print("| Pseudo-Functions:      {0}".format(pseudo_functions))
            print("- Not in binary/source:  {0}".format(not_found))
            print("")

            print("Accuracy of inference")
            print("| Params Ok/Total tested:  {0}/{1}".format(params_ok, params_total))
            print("| Return Ok/Total tested:  {0}/{1}".format(return_ok, return_total))
            print("| Ratio params:            {0:.2f}%".format(self.ratio(params_ok, params_total)))
            print("- Ratio return:            {0:.2f}%".format(self.ratio(return_ok, return_total)))

        if log is not None:
            params = self.log.get_params()
            with open(log, "a") as f:
                f.write("{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}\n".format(
                        self.pgm,
                        params["MIN_VALS"],
                        params["MAX_VALS"],
                        params["ADDR_THRESHOLD"],
                        param_fp, 
                        param_fn, 
                        params_total, 
                        return_fp, 
                        return_fn, 
                        return_total,
                        self.log.time(),
                        empty_time, 
                        nopin_time, 
                    ))
        if get:
            return (params_ok, return_ok, param_fp, param_fn, return_fp, return_fn, params_total, return_total)

    def mismatch(self):
        self.print_general_info()
        print("")

        for function, args in self.log.get():
            img, imgaddr, fname = function.split(":")
            imgaddr = int(imgaddr)
            if fname == "" or fname not in self.protos.keys():
                continue

            proto = self.protos[fname]
            if self.is_variadic(proto):
                continue

            res = self.check_function(fname, args, proto, True)

            if res[0] + res[1] == 0 and res[3] + res[4] == 0:
                continue

            print("[{}@{}] {}".format(img, hex(imgaddr), fname))
            print("           {} -> {}".format(", ".join(proto[1:]), proto[0]))
            print("Expected:  {} -> {}".format(
                    ", ".join(map(self.pp_data_type, proto[1:])),
                    self.pp_data_type(proto[0])))
            print("Got:       {} -> {}".format(
                    ", ".join(map(self.pp_inferred_type, args[1:])),
                    self.pp_inferred_type(args[0])))

