#!/usr/bin/python

""" 
    Usage: call with three arguments:
        1) path to src files (e.g. src/midori/)
        2) type of analysis (-a for arity, -t for type)
        3) path to log file (e.g. log/arity/_midori/130715.log)

"""

import sys
import os
import clang.cindex
import pickle

class Function(object):

    def __init__(self, fpath):
        self.path = fpath
        self.protos = dict()
        self.rprotos = dict()
        self.loaded = False
        if fpath[-4:] == ".dat":
            self.loaded = True
            self.protos = pickle.load(open(fpath, "rb"))

    def find_protos(self, node, **kwargs):
        verbose = False
        if "verbose" in kwargs.keys():
            verbose = kwargs["verbose"]
        if self.loaded:
            return
        if verbose:
            print node.spelling, node.kind
        if (node.type.kind == clang.cindex.TypeKind.FUNCTIONPROTO): # or node.type.kind == clang.cindex.TypeKind.FUNCTIONNOPROTO):
            if verbose:
                print node.spelling
            if node.spelling not in self.protos.keys():
                self.protos[node.spelling] = list()
                self.rprotos[node.spelling] = list()
            if len(self.protos[node.spelling]) == 0:
                if (node.result_type.spelling == "Lisp_Object"):
                    self.protos[node.spelling].append("void *")
                else:
                    self.protos[node.spelling].append(node.result_type.get_canonical().spelling)
                for c in node.get_arguments():
                    if (c.type.spelling == "Lisp_Object"):
                        self.protos[node.spelling].append("void *")
                    else:
                        self.protos[node.spelling].append(c.type.get_canonical().spelling)
                    self.rprotos[node.spelling].append(c.type.spelling)
        for c in node.get_children():
            self.find_protos(c, verbose=verbose)
        return


    def save(self):
        if self.loaded:
            return
        pickle.dump(self.protos, open(raw_input("Where to save ? "), "wb"))


    def extract_protos(self):
        for dirpath, dirnames, filenames in os.walk(self.path):
            for fname in filenames:
                fpath = dirpath + "/" + fname
                fext = fname.split(".")[-1]
                if fext == "c" or fext == "h":
                    index = clang.cindex.Index.create()
                    tu = index.parse(fpath)
                    if fname == "gvariant.h":
                        print "YOLO"
                        self.find_protos(tu.cursor, verbose=True)
                    else:
                        self.find_protos(tu.cursor)
        

    def comp(self, fname, args):
        if len(args) != len(self.protos[fname]):
            # print fname, args, self.protos[fname]
            return (0, 0)
        else:
            for ref, inf in zip(self.protos[fname], args):
                if (inf[:4] == "ADDR" and "*" not in ref and "[" not in ref) or (inf[:4] != "ADDR" and ("*" in ref or "[" in ref)):
                    print fname, self.protos[fname], args
                    return (1, 0)
            return (1, 1)


    def comp_ar(self, fname, ar):
        if ar == len(self.protos[fname]) - 1:
            return 1
        else: 
            print fname, self.protos[fname], len(self.protos[fname]), ar
            return 0


    def comp_ret(self, fname, ret):
        if (ret and self.protos[fname][0] == "void") or (not ret and self.protos[fname][0] != "void"):
            return 0
        else:
            return 1


class PrototypeCheck(object):

    def __init__(self, src_path, log):
        self.path = src_path
        self.log = log
        self.f = Function(self.path)
        self.f.extract_protos()
        self.f.save()


    def check_one(self, fname, args):
        if fname in self.f.protos.keys():
            return self.f.comp(fname, args)
        else:
            # print fname
            return (-1, -1)


    def check_one_arity(self, fname, ar):
        if fname in self.f.protos.keys():
            return self.f.comp_ar(fname, ar)
        else:
            return -1


    def check_one_ret(self, fname, ret):
        return self.f.comp_ret(fname, ret)

    def check_arity(self):
        tot_ar = 0
        ok_ar = 0
        ok_ret = 0
        overflow = 0
        not_found = 0
        with open(self.log, "r") as f:
            for line in f.readlines():
                try:
                    addr, fn, ar, iar, ret, misc = line[:-1].split(":")
                except Exception:
                    print "EXCEPTION"
                    print line[:-1]
                    exit()
                if fn != "":
                    ar = int(ar, 16)
                    res = self.check_one_arity(fn, ar)
                    if res == -1:
                        not_found += 1
                        continue
                    elif res == 0 and ar > 6:
                        overflow += 1
                    if self.check_one_ret(fn, ret == "1") > 0:
                        ok_ret += 1
                    else:
                        print fn, ret, self.f.protos[fn][0]
                    tot_ar += 1
                    ok_ar += res
        print ok_ar, tot_ar
        if tot_ar != 0:
            print "Ratio AR : {0}".format(float(ok_ar)*100./float(tot_ar))
            print "Ratio RET: {0}".format(float(ok_ret)*100./float(tot_ar))
        print "Overflow: " + str(overflow)
        print "Not found: {0}".format(not_found)
        print "Total functions in source: {0}".format(len(self.f.protos))


    def check_types(self):
        tot_ar = 0
        tot_typ = 0
        ok_ar = 0
        ok_typ = 0
        with open(self.log, "r") as f:
            for line in f.readlines():
                try:
                    addr, fn, args = line[:-1].split(":")
                except Exception:
                    print "EXCEPTION"
                    print line[:-1]
                    exit()
                if fn != "":
                    #                    print "#" + fn + "#" + " - " + str(fn in self.f.protos.keys()) 
                    if args != "":
                        args = args.split(",")
                    else:
                        args = list()
                    res = self.check_one(fn, args)
                    if res[0] == -1:
                        continue
                    tot_ar += 1
                    ok_ar += res[0]
                    ok_typ += res[1]
                    if res[0] == 1:
                        tot_typ += 1
        print ok_ar, tot_ar, 
        if tot_ar != 0:
            print "Ratio: {0}".format(float(ok_ar)*100./float(tot_ar))
        print ok_typ, tot_typ, 
        if tot_typ != 0:
            print "Ratio: {0}".format(float(ok_typ)*100./float(tot_typ))


src_path = sys.argv[1]
log_path = sys.argv[3]
check = PrototypeCheck(src_path, log_path)

if sys.argv[2] == "-t":
    check.check_types()
elif sys.argv[2] == "-a":
    check.check_arity()
else:
    raise IndexError
