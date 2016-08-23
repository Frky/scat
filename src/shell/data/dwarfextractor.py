#-*- coding: utf-8 -*-

import os

from elftools.elf.elffile import ELFFile

class DwarfExtractor(object):

    def __init__(self):
        self.types_cache = dict()

    def extract(self, binary):
        protos = dict()
        with open(binary, 'rb') as f:
            elf_file = ELFFile(f)

            if not elf_file.has_dwarf_info():
                print('    File has no debug info (DWARF format expected) !')
                return protos

            dwarf_info = elf_file.get_dwarf_info()
            for CU in dwarf_info.iter_CUs():
                for DIE in CU.iter_DIEs():
                    self.__extract_DIE(CU, DIE, protos)

        return protos


    def __extract_DIE(self, CU, DIE, protos):
        if DIE.tag == 'DW_TAG_subprogram':
            if ('DW_AT_name' not in DIE.attributes
                    or 'DW_AT_declaration' in DIE.attributes):
                return

            name = self.DIE_name(DIE)
            if name in protos:
                return

            protos[name] = list()
            protos[name].append(self.DIE_type(CU, DIE))
            variadic = False
            for child in DIE.iter_children():
                if child.tag == 'DW_TAG_formal_parameter':
                    protos[name].append(self.DIE_type(CU, child))
                elif child.tag == 'DW_TAG_unspecified_parameters':
                    variadic = True

            if variadic:
                protos[name].append('...')


    def DIE_name(self, DIE):
        if 'DW_AT_name' in DIE.attributes:
            return DIE.attributes['DW_AT_name'].value
        else:
            return "<???>"


    def DIE_type(self, CU, DIE):
        if 'DW_AT_type' not in DIE.attributes:
            return 'void'

        type = DIE.attributes['DW_AT_type']
        if type.form != 'DW_FORM_ref4':
            return '??{} {}??'.format(type.form, hex(type.value))

        addr = CU.cu_offset + type.value
        if addr in self.types_cache:
            return self.types_cache[addr]

        type = self.lookup_type(CU, addr)
        if 'Lisp_Object' in type:
            type = 'void *'
        self.types_cache[addr] = type
        return type


    def lookup_type(self, CU, addr):
        for DIE in CU.iter_DIEs():
            if DIE.offset != addr:
                continue

            if (DIE.tag == 'DW_TAG_base_type'):
                name = DIE.attributes['DW_AT_name']
                return name.value
            elif DIE.tag == 'DW_TAG_structure_type':
                if 'DW_AT_name' in DIE.attributes:
                    name = DIE.attributes['DW_AT_name']
                    return name.value
                else:
                    return 'anonymous struct'
            elif DIE.tag == 'DW_TAG_enumeration_type':
                if 'DW_AT_name' in DIE.attributes:
                    name = DIE.attributes['DW_AT_name']
                    return name.value
                else:
                    return 'anonymous enum'
            elif DIE.tag == 'DW_TAG_union_type':
                if 'DW_AT_name' in DIE.attributes:
                    name = DIE.attributes['DW_AT_name']
                    return name.value
                else:
                    return 'anonymous union'
            elif (DIE.tag == 'DW_TAG_subroutine_type'):
                return self.subroutine_type(CU, DIE)
            elif DIE.tag == 'DW_TAG_typedef':
                name = DIE.attributes['DW_AT_name']
                return "typedef " + self.DIE_type(CU, DIE) + " " + name.value
            elif DIE.tag == 'DW_TAG_const_type':
                return "const " + self.DIE_type(CU, DIE)
            elif DIE.tag == 'DW_TAG_volatile_type':
                return "volatile " + self.DIE_type(CU, DIE)
            elif DIE.tag == 'DW_TAG_restrict_type':
                return self.DIE_type(CU, DIE) + " restrict"
            elif DIE.tag == 'DW_TAG_pointer_type':
                return self.DIE_type(CU, DIE) + "*"
            elif DIE.tag == 'DW_TAG_array_type':
                return self.DIE_type(CU, DIE) + "[]"
            elif DIE.tag == 'DW_TAG_reference_type':
                return self.DIE_type(CU, DIE) + "&"
            else:
                print("  Dohh 3: {}".format(DIE))

        return "?{}?".format(hex(addr))


    def subroutine_type(self, CU, DIE):
        type = '(' + self.DIE_type(CU, DIE) + ' ('
        first = True
        for child in DIE.iter_children():
            if child.tag == 'DW_TAG_formal_parameter':
                if first:
                    first = False
                else:
                    type += ', '
                type += self.DIE_type(CU, child)
        type += '))'
        return type
