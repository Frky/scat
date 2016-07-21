#-*- coding: utf-8 -*-

import os
from clang.cindex import Config, Index, TypeKind

class ClangExtractor(object):

    def __init__(self, libclang_path, srcdir):
        if Config.library_file != libclang_path:
            Config.set_library_file(libclang_path)
        self.srcdir = srcdir


    def extract(self):
        protos = dict()
        for dirpath, dirnames, filenames in os.walk(self.srcdir):
            for fname in filenames:
                fpath = dirpath + "/" + fname
                fext = fname.split(".")[-1]
                if fext == "c" or fext == "h":
                    index = Index.create()
                    tu = index.parse(fpath)
                    self.__clang_find_protos(tu.cursor, protos)

        return protos


    def __clang_find_protos(self, node, protos):
        if (node.type.kind == TypeKind.FUNCTIONPROTO): # or node.type.kind == TypeKind.FUNCTIONNOPROTO):
            if node.spelling not in protos.keys():
                protos[node.spelling] = list()
            if len(protos[node.spelling]) == 0:
                if (node.result_type.spelling == "Lisp_Object"):
                    protos[node.spelling].append("void *")
                else:
                    protos[node.spelling].append(node.result_type.get_canonical().spelling)
                for c in node.get_arguments():
                    if (c.type.spelling == "Lisp_Object"):
                        protos[node.spelling].append("void *")
                    else:
                        protos[node.spelling].append(c.type.get_canonical().spelling)
                if node.type.is_function_variadic():
                    protos[node.spelling].append("...")
        for c in node.get_children():
            self.__clang_find_protos(c, protos)
