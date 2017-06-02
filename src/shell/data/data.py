#-*- coding: utf-8 -*-

import pickle
import os
import os.path

from elftools.elf.elffile import ELFFile

from src.shell.data.dwarfextractor import DwarfExtractor
from src.shell.data.clangextractor import ClangExtractor

class Data(object):

    def __init__(self, dstdir, pgm):
        self.dstdir = dstdir
        self.pgm = pgm
        self.deps = []
        self.protos_without_libs = dict()
        self.protos = dict()


    def __path(self, ext):
        return self.dstdir + "/" + self.pgm + ext


    def load(self, add_hardcoded=True, verbose=True, main_pgm=True):
        self.deps = []
        self.protos = dict()

        deps_path = self.__path(".deps")
        if os.path.exists(deps_path):
            self.deps = pickle.load(open(deps_path, "rb"))
            for dep in self.deps:
                dep_data = Data(self.dstdir, dep)
                dep_data.load(False, verbose=verbose, main_pgm=False)
                self.protos.update(dep_data.protos)

        data_path = self.__path(".data")
        if os.path.exists(data_path):
            self.protos_without_libs = pickle.load(open(data_path, "rb"))
        elif main_pgm:
            raise IOError
        else:
            self.protos_without_libs = dict()
            if verbose:
                print("!! Missing data for {}".format(self.pgm))

        self.protos.update(self.protos_without_libs)

        if add_hardcoded:
            self.__hardcoded_protos()


    def dump(self):
        if not(os.path.exists(self.dstdir)):
            os.makedirs(self.dstdir)
        pickle.dump(self.deps, open(self.__path(".deps"), "wb"))
        pickle.dump(self.protos_without_libs, open(self.__path(".data"), "wb"))


    def parse(self, binary, libclang_path, srcdir = None, force=False, verbose=True):
        if not force and os.path.exists(self.__path(".data")):
            if verbose:
                print "Already infered -- aborting"
            return False
        if verbose:
            print(" * Checking dependencies")
        self.deps = []
        self.protos = dict()

        with open(binary, 'rb') as f:
            elf_file = ELFFile(f)

            dynamic = elf_file.get_section_by_name('.dynamic')
            for tag in dynamic.iter_tags('DT_NEEDED'):
                if verbose:
                    print("     Found dependency {}".format(tag.needed))
                self.deps.append(tag.needed)

        for dep in self.deps:
            dep_data = Data(self.dstdir, dep)
            dep_data.load(False, verbose=verbose, main_pgm=False)
            self.protos.update(dep_data.protos)

        self.protos_without_libs = dict()

        if srcdir != None:
            if verbose:
                print(" * Extracting data from source code")
            extractor = ClangExtractor(libclang_path, srcdir)
            self.protos_without_libs.update(extractor.extract())

        if verbose:
            print(" * Extracting data from binary debug informations")
        extractor = DwarfExtractor()
        # self.protos_without_libs.update(extractor.extract(binary))

        self.protos.update(self.protos_without_libs)

        return True

    def __hardcoded_protos(self):
        """
            These are functions which are hard to detect
            because of glibc magics
        """
        self.__hardcoded_glibc_proto('bzero', ['void', 'void*', 'int'])
        self.__hardcoded_glibc_proto('write', ['int', 'int', 'void*', 'int'])

        self.__hardcoded_glibc_proto('malloc', ['void*', 'int'])
        self.__hardcoded_glibc_proto('calloc', ['void*', 'int', 'int'])
        self.__hardcoded_glibc_proto('memchr', ['void*', 'void*', 'int', 'int'])
        self.__hardcoded_glibc_proto('memrchr', ['void*', 'void*', 'int', 'int'])
        self.__hardcoded_glibc_proto('rawmemchr', ['void*', 'void*', 'int', 'int'])
        self.__hardcoded_glibc_proto('memcmp', ['int', 'void*', 'void*', 'int'])

        self.__hardcoded_glibc_proto('memset', ['void*', 'void*', 'int', 'int'])
        self.__hardcoded_glibc_proto('memmove', ['void*', 'void*', 'void*', 'int'])
        self.__hardcoded_glibc_proto('memcpy', ['void*', 'void*', 'void*', 'int'])
        self.__hardcoded_glibc_proto('mempcpy', ['void*', 'void*', 'void*', 'int'])

        self.__hardcoded_glibc_proto('strcat', ['char*', 'char*', 'char*'])
        self.__hardcoded_glibc_proto('strchr', ['char*', 'char*', 'int'])
        self.__hardcoded_glibc_proto('strchrnul', ['char*', 'char*', 'int'])
        self.__hardcoded_glibc_proto('strcmp', ['int', 'char*', 'char*'])
        self.__hardcoded_glibc_proto('strncmp', ['int', 'char*', 'char*', 'int'])
        self.__hardcoded_glibc_proto('strcpy', ['char*', 'char*', 'char*'])
        self.__hardcoded_glibc_proto('stpcpy', ['char*', 'char*', 'char*'])
        self.__hardcoded_glibc_proto('strlen', ['int', 'char*'])

        self.__hardcoded_glibc_proto('vsnprintf', ['int', 'char*', 'char*', '...'])


    def __hardcoded_glibc_proto(self, name, proto):
        self.protos[name] = proto
        self.protos['__' + name] = proto
        self.protos['__GI_' + name] = proto
        self.protos['__GI___' + name] = proto
        if 'sse2' not in name:
            self.__hardcoded_glibc_proto(name + '_sse2', proto)
            self.__hardcoded_glibc_proto(name + '_sse2_unaligned', proto)

