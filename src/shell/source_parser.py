#-*- coding: utf-8 -*-

import clang.cindex
import pickle
import os

VERBOSE = False

class SourceParser(object):

    def __init__(self, libclang_path, pgm, dstdir, srcdir=None):
        if clang.cindex.Config.library_file != libclang_path:
            clang.cindex.Config.set_library_file(libclang_path)
        self.pgm = pgm
        self.srcdir = srcdir
        self.dstdir = dstdir
        self.protos = dict()


    def parse(self):
        self.__hardcoded_protos()
        self.__extract_protos()
        self.dump()


    def dump(self):
        pickle.dump(self.protos, open(self.dstdir + "/" + self.pgm + ".data", "wb"))


    def load(self):
        self.protos = pickle.load(open(self.dstdir + "/" + self.pgm + ".data", "rb"))
        return self.protos


    def __hardcoded_protos(self):
        self.protos["strlen"] = ["int", "char *"]
        self.protos["memchr"] = ["void *", "void *", "void *", "int"]
        self.protos["memcpy"] = ["void *", "void *", "void *", "int"]


    def __find_protos(self, node, **kwargs):
        if (node.type.kind == clang.cindex.TypeKind.FUNCTIONPROTO): # or node.type.kind == clang.cindex.TypeKind.FUNCTIONNOPROTO):
            if VERBOSE:
                print(node.spelling)
            if node.spelling not in self.protos.keys():
                self.protos[node.spelling] = list()
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
                if node.type.is_function_variadic():
                    self.protos[node.spelling].append("...")
        for c in node.get_children():
            self.__find_protos(c)
        return


    def __extract_protos(self):
        for dirpath, dirnames, filenames in os.walk(self.srcdir):
            for fname in filenames:
                fpath = dirpath + "/" + fname
                fext = fname.split(".")[-1]
                if fext == "c" or fext == "h":
                    index = clang.cindex.Index.create()
                    tu = index.parse(fpath)
                    self.__find_protos(tu.cursor)
        
